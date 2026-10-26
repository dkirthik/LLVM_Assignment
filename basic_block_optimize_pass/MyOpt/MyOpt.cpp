#include "llvm/Transforms/Utils/MyOpt.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>

using namespace llvm;

/*
 * Passes done here (basic block-local):
 *   1. constant propagation + copy propagation  (store -> load forwarding)
 *   2. redundant assignment elimination          (a = 3; a = 3;)
 *   3. inst combine     (constant folding, identities, x * 2^k -> shl)
 *   4. strength reduction (x * 5 -> (x << 2) + x, x % 8 -> x & 7)
 *   5. common subexpression elimination
 */

/* ===================== my data structures ===================== */

#define TABLE_SIZE 64 // number of hash buckets

// one chained hash entry: an LLVM value and the value remembered for it
struct Node {
  Value *key;
  Value *val;
  Node *next;
};

// hash table from Value* to Value*, used to remember what each alloca holds
struct Table {
  Node *bucket[TABLE_SIZE];

  Table() { // start with every bucket empty
    for (int i = 0; i < TABLE_SIZE; i++)
      bucket[i] = nullptr;
  }

  ~Table() { clear(); } // free every node

  int hash(Value *k) { // bucket from the pointer; low bits are 0 because of alignment
    uintptr_t p = (uintptr_t)k;
    return (int)((p >> 4) % TABLE_SIZE);
  }

  Value *get(Value *k) { // stored value for this key, or nullptr if it is absent
    for (Node *n = bucket[hash(k)]; n != nullptr; n = n->next) {
      if (n->key == k)
        return n->val;
    }
    return nullptr;
  }

  void put(Value *k, Value *v) { // insert this key, or overwrite it if it is already there
    int h = hash(k);
    for (Node *n = bucket[h]; n != nullptr; n = n->next) {
      if (n->key == k) {
        n->val = v;
        return;
      }
    }
    Node *n = new Node;
    n->key = k;
    n->val = v;
    n->next = bucket[h];
    bucket[h] = n;
  }

  void clear() { // delete every node and empty every bucket
    for (int i = 0; i < TABLE_SIZE; i++) {
      while (bucket[i] != nullptr) {
        Node *tmp = bucket[i];
        bucket[i] = tmp->next;
        delete tmp;
      }
    }
  }
};

// one binary expression already computed in this block
struct Expr {
  unsigned op;
  Value *lhs;
  Value *rhs;
  Instruction *inst; // instruction that computed this expression
  Expr *next;
};

// linked list of expressions, searched by local CSE
struct ExprList {
  Expr *head;

  ExprList() { head = nullptr; } // empty list
  ~ExprList() { clear(); }       // free every expression node

  Instruction *find(unsigned op, Value *l, Value *r) { // earlier instruction with the same op and operands
    for (Expr *e = head; e != nullptr; e = e->next) {
      if (e->op == op && e->lhs == l && e->rhs == r)
        return e->inst;
    }
    return nullptr;
  }

  void add(unsigned op, Value *l, Value *r, Instruction *I) { // remember this instruction at the front
    Expr *e = new Expr;
    e->op = op;
    e->lhs = l;
    e->rhs = r;
    e->inst = I;
    e->next = head;
    head = e;
  }

  void clear() { // delete every expression node
    while (head != nullptr) {
      Expr *tmp = head;
      head = head->next;
      delete tmp;
    }
  }
};

/* ===================== helpers ===================== */

// returns k if x == 2^k, otherwise -1
static int log2u(uint64_t x) {
  if (x == 0 || (x & (x - 1)) != 0)
    return -1;
  int k = 0;
  while (x > 1) {
    x = x >> 1;
    k++;
  }
  return k;
}

// same as log2u for a constant, but refuse integers wider than 64 bits
static int log2Const(ConstantInt *C) {
  if (C->getBitWidth() > 64)
    return -1;
  return log2u(C->getZExtValue());
}

// my own constant folder. returns nullptr if it cant / shouldnt fold
static Constant *foldBinary(unsigned op, ConstantInt *A, ConstantInt *B) {
  unsigned w = A->getBitWidth();
  if (w > 64)
    return nullptr;

  uint64_t a = A->getZExtValue();
  uint64_t b = B->getZExtValue();
  int64_t sa = A->getSExtValue();
  int64_t sb = B->getSExtValue();
  uint64_t r = 0;

  switch (op) {
  case Instruction::Add: r = a + b; break; // fold a + b
  case Instruction::Sub: r = a - b; break; // fold a - b
  case Instruction::Mul: r = a * b; break; // fold a * b
  case Instruction::And: r = a & b; break; // fold a & b
  case Instruction::Or:  r = a | b; break; // fold a | b
  case Instruction::Xor: r = a ^ b; break; // fold a ^ b

  case Instruction::Shl: // fold a << b, but a shift of the full width or more is poison
    if (b >= w) return nullptr;
    r = a << b;
    break;
  case Instruction::LShr: // fold logical a >> b, same poison rule as shl
    if (b >= w) return nullptr;
    r = a >> b;
    break;
  case Instruction::AShr: // fold arithmetic a >> b using the signed value
    if (b >= w) return nullptr;
    r = (uint64_t)(sa >> b);
    break;

  case Instruction::UDiv: // fold unsigned a / b, division by zero is undefined
    if (b == 0) return nullptr;
    r = a / b;
    break;
  case Instruction::URem: // fold unsigned a % b, remainder by zero is undefined
    if (b == 0) return nullptr;
    r = a % b;
    break;
  case Instruction::SDiv: // fold signed a / b, skip 0 and INT_MIN / -1
  case Instruction::SRem: // fold signed a % b, same two cases that overflow or are undefined
    if (sb == 0) return nullptr;
    if (sb == -1 && A->getValue().isMinSignedValue()) return nullptr;
    if (op == Instruction::SDiv)
      r = (uint64_t)(sa / sb);
    else
      r = (uint64_t)(sa % sb);
    break;

  default: // opcode this folder does not handle
    return nullptr;
  }

  // chop result back down to the real bit width
  if (w < 64)
    r = r & ((1ULL << w) - 1);

  return ConstantInt::get(A->getType(), r);
}

// x+0, x*1, x-x, x/x ... returns the simpler value or nullptr
static Value *simplifyIdentity(BinaryOperator *BO) {
  Value *X = BO->getOperand(0);
  Value *Y = BO->getOperand(1);
  ConstantInt *CY = dyn_cast<ConstantInt>(Y);
  Type *T = BO->getType();

  switch (BO->getOpcode()) {
  case Instruction::Add: // x + 0 is x
    if (CY && CY->isZero()) return X;
    break;
  case Instruction::Sub: // x - 0 is x, and x - x is 0
    if (CY && CY->isZero()) return X;
    if (X == Y) return ConstantInt::get(T, 0);
    break;
  case Instruction::Mul: // x * 1 is x, and x * 0 is 0
    if (CY && CY->isOne()) return X;
    if (CY && CY->isZero()) return CY;
    break;
  case Instruction::SDiv: // x / 1 is x, and x / x is 1
  case Instruction::UDiv: // same identities for unsigned division
    if (CY && CY->isOne()) return X;
    if (X == Y) return ConstantInt::get(T, 1); // x == 0 is undefined in C
    break;
  case Instruction::SRem: // x % 1 is 0, and x % x is 0
  case Instruction::URem: // same identities for unsigned remainder
    if (CY && CY->isOne()) return ConstantInt::get(T, 0);
    if (X == Y) return ConstantInt::get(T, 0);
    break;
  case Instruction::And: // x & x is x, x & 0 is 0, x & -1 is x
    if (X == Y) return X;
    if (CY && CY->isZero()) return CY;
    if (CY && CY->isMinusOne()) return X;
    break;
  case Instruction::Or: // x | x is x, x | 0 is x, x | -1 is -1
    if (X == Y) return X;
    if (CY && CY->isZero()) return X;
    if (CY && CY->isMinusOne()) return CY;
    break;
  case Instruction::Xor: // x ^ x is 0, and x ^ 0 is x
    if (X == Y) return ConstantInt::get(T, 0);
    if (CY && CY->isZero()) return X;
    break;
  case Instruction::Shl:  // any shift by 0 leaves x unchanged
  case Instruction::LShr: // logical right shift by 0 is x
  case Instruction::AShr: // arithmetic right shift by 0 is x
    if (CY && CY->isZero()) return X;
    break;
  default: // no identity for this opcode
    break;
  }
  return nullptr;
}

// print the rewrite, point every use of I at V, then delete I
static void replaceAndErase(Instruction *I, Value *V, const char *why) {
  errs() << "  [" << why << "] " << *I << "  ==>  " << *V << "\n";
  I->replaceAllUsesWith(V);
  I->eraseFromParent();
}

/* ===================== the passes ===================== */

// 1. constant + copy propagation
// at -O0 every variable lives in an alloca. remember what was last stored
// in each one, and when it gets loaded back use that value directly.
//   store 3, %a  ...  load %a   -> 3        (constant propagation)
//   store %d, %c ...  load %c   -> %d       (copy propagation)
static bool propagateStores(BasicBlock &BB) {
  bool changed = false;
  Table mem; // alloca -> value currently inside it

  for (auto it = BB.begin(); it != BB.end();) {
    Instruction *I = &*it++;

    if (StoreInst *S = dyn_cast<StoreInst>(I)) {
      if (!S->isVolatile() && isa<AllocaInst>(S->getPointerOperand())) {
        mem.put(S->getPointerOperand(), S->getValueOperand());
        continue;
      }
    }

    if (LoadInst *L = dyn_cast<LoadInst>(I)) {
      Value *ptr = L->getPointerOperand();
      if (!L->isVolatile() && isa<AllocaInst>(ptr)) {
        Value *v = mem.get(ptr);
        if (v != nullptr && v->getType() == L->getType()) {
          if (isa<Constant>(v))
            replaceAndErase(L, v, "const prop");
          else
            replaceAndErase(L, v, "copy prop");
          changed = true;
        } else {
          mem.put(ptr, L); // next load of ptr can reuse this one
        }
        continue;
      }
    }

    // call / store through some pointer / etc, could change any local
    if (I->mayWriteToMemory())
      mem.clear();
  }
  return changed;
}

// 2. redundant assignment elimination
// a = 3; a = 3;  -> second store does nothing
static bool removeRedundantStores(BasicBlock &BB) {
  bool changed = false;
  Table mem;

  for (auto it = BB.begin(); it != BB.end();) {
    Instruction *I = &*it++;

    if (StoreInst *S = dyn_cast<StoreInst>(I)) {
      Value *ptr = S->getPointerOperand();
      Value *val = S->getValueOperand();
      if (!S->isVolatile() && isa<AllocaInst>(ptr)) {
        if (mem.get(ptr) == val) {
          errs() << "  [redundant store] " << *S << "\n";
          S->eraseFromParent();
          changed = true;
        } else {
          mem.put(ptr, val);
        }
        continue;
      }
    }

    if (LoadInst *L = dyn_cast<LoadInst>(I)) {
      Value *ptr = L->getPointerOperand();
      if (!L->isVolatile() && isa<AllocaInst>(ptr)) {
        if (mem.get(ptr) == nullptr)
          mem.put(ptr, L); // so "x = x" is caught too
        continue;
      }
    }

    if (I->mayWriteToMemory())
      mem.clear();
  }
  return changed;
}

// 3. inst combine
//   - constant folding        2 * 3   -> 6
//   - algebraic identities    a / a   -> 1,  b - b -> 0,  x + 0 -> x
//   - constant goes on right  4 * x   -> x * 4
//   - combine constants       (x + 2) + 3 -> x + 5
//   - power of 2              x * 8   -> x << 3,  x udiv 4 -> x >> 2
static bool instCombine(BasicBlock &BB) {
  bool changed = false;

  for (auto it = BB.begin(); it != BB.end();) {
    Instruction *I = &*it++;
    BinaryOperator *BO = dyn_cast<BinaryOperator>(I);
    if (BO == nullptr || !BO->getType()->isIntegerTy())
      continue;

    if (BO->isCommutative() && isa<Constant>(BO->getOperand(0)) &&
        !isa<Constant>(BO->getOperand(1))) {
      BO->swapOperands();
      changed = true;
    }

    Value *X = BO->getOperand(0);
    ConstantInt *CX = dyn_cast<ConstantInt>(X);
    ConstantInt *CY = dyn_cast<ConstantInt>(BO->getOperand(1));
    unsigned op = BO->getOpcode();

    if (CX && CY) {
      Constant *C = foldBinary(op, CX, CY);
      if (C) {
        replaceAndErase(BO, C, "const fold");
        changed = true;
      }
      continue;
    }

    Value *simple = simplifyIdentity(BO);
    if (simple) {
      replaceAndErase(BO, simple, "identity");
      changed = true;
      continue;
    }

    if (CY == nullptr)
      continue;

    if (op == Instruction::Add || op == Instruction::Mul) {
      BinaryOperator *inner = dyn_cast<BinaryOperator>(X);
      if (inner && inner->getOpcode() == op && inner->hasOneUse()) {
        ConstantInt *C1 = dyn_cast<ConstantInt>(inner->getOperand(1));
        if (C1) {
          Constant *C = foldBinary(op, C1, CY);
          if (C) {
            errs() << "  [combine] " << *inner << " and " << *BO << "\n";
            BO->setOperand(0, inner->getOperand(0));
            BO->setOperand(1, C);
            BO->dropPoisonGeneratingFlags(); // nsw might not hold anymore
            changed = true;
            continue;
          }
        }
      }
    }

    int k = log2Const(CY);
    if (k > 0) {
      IRBuilder<> B(BO);
      Value *nv = nullptr;
      if (op == Instruction::Mul)
        nv = B.CreateShl(X, k, "shl");
      else if (op == Instruction::UDiv)
        nv = B.CreateLShr(X, k, "lshr");
      // sdiv by 2^k is NOT a plain ashr for negative numbers, so skip it

      if (nv) {
        replaceAndErase(BO, nv, "pow2 shift");
        changed = true;
      }
    }
  }
  return changed;
}

// 4. strength reduction
// multiply by a constant next to a power of 2 -> shift and add/sub
//   x * 5 -> (x << 2) + x      x * 7 -> (x << 3) - x
//   x urem 8 -> x & 7
static bool strengthReduce(BasicBlock &BB) {
  bool changed = false;

  for (auto it = BB.begin(); it != BB.end();) {
    Instruction *I = &*it++;
    BinaryOperator *BO = dyn_cast<BinaryOperator>(I);
    if (BO == nullptr || !BO->getType()->isIntegerTy())
      continue;

    Value *X = BO->getOperand(0);
    ConstantInt *CY = dyn_cast<ConstantInt>(BO->getOperand(1));
    if (CY == nullptr || isa<Constant>(X))
      continue;

    unsigned w = CY->getBitWidth();
    if (w > 64)
      continue;
    uint64_t c = CY->getZExtValue();

    IRBuilder<> B(BO);
    Value *nv = nullptr;

    if (BO->getOpcode() == Instruction::Mul && c > 2) {
      int k = log2u(c - 1);
      if (k >= 1 && k < (int)w) {
        Value *t = B.CreateShl(X, k, "sr.shl");
        nv = B.CreateAdd(t, X, "sr.add");
      } else {
        k = log2u(c + 1);
        if (k >= 1 && k < (int)w) {
          Value *t = B.CreateShl(X, k, "sr.shl");
          nv = B.CreateSub(t, X, "sr.sub");
        }
      }
    } else if (BO->getOpcode() == Instruction::URem) {
      int k = log2u(c);
      if (k >= 1)
        nv = B.CreateAnd(X, ConstantInt::get(BO->getType(), c - 1), "sr.and");
    }

    if (nv) {
      replaceAndErase(BO, nv, "strength reduce");
      changed = true;
    }
  }
  return changed;
}

// 5. local CSE
// t1 = a + b ... t2 = b + a  -> t2 is the same as t1
static bool localCSE(BasicBlock &BB) {
  bool changed = false;
  ExprList seen;

  for (auto it = BB.begin(); it != BB.end();) {
    Instruction *I = &*it++;
    BinaryOperator *BO = dyn_cast<BinaryOperator>(I);
    if (BO == nullptr)
      continue;

    unsigned op = BO->getOpcode();
    Value *L = BO->getOperand(0);
    Value *R = BO->getOperand(1);

    // a+b and b+a should look the same, so order by pointer
    if (BO->isCommutative() && (uintptr_t)L > (uintptr_t)R) {
      Value *tmp = L;
      L = R;
      R = tmp;
    }

    Instruction *old = seen.find(op, L, R);
    if (old) {
      old->andIRFlags(BO); // keep only flags that both had
      replaceAndErase(BO, old, "cse");
      changed = true;
    } else {
      seen.add(op, L, R, BO);
    }
  }
  return changed;
}

// Run the local opts until a block stops changing. Later folds only become
// visible to store forwarding on the next iteration (result = result + 2).
PreservedAnalyses MyOptPass::run(Function &F, FunctionAnalysisManager &AM) {
  (void)AM;
  bool changed = false;

  errs() << "MyOpt: " << F.getName() << "\n";
  for (BasicBlock &BB : F) {
    bool again = true;
    unsigned guard = 0;
    while (again && guard < 32) {
      again = false;
      again |= propagateStores(BB);
      again |= removeRedundantStores(BB);
      again |= instCombine(BB);
      again |= strengthReduce(BB);
      again |= localCSE(BB);
      changed |= again;
      guard++;
    }
  }

  if (!changed)
    return PreservedAnalyses::all();

  // CFG is untouched; only instructions inside each block are rewritten.
  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}
