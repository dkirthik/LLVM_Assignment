#ifndef LLVM_TRANSFORMS_UTILS_MYOPT_H
#define LLVM_TRANSFORMS_UTILS_MYOPT_H

#include "llvm/IR/PassManager.h"

namespace llvm {

// function pass that runs the local optimizations in MyOpt.cpp
class MyOptPass : public RequiredPassInfoMixin<MyOptPass> {
public:
  // optimize one function; returns which analyses are still valid
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif
