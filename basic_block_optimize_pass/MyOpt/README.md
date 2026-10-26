# MyOpt

Local LLVM function pass (`myopt`) for the tests in `opt_tests.c`.

It is compiled into the LLVM `opt` tool. The implementation in this folder is pulled in by the LLVM tree:

- `C:\Users\dkirthik\llvm-project\llvm\lib\Transforms\Utils\MyOpt.cpp` includes `MyOpt.cpp` from this folder
- `C:\Users\dkirthik\llvm-project\llvm\include\llvm\Transforms\Utils\MyOpt.h` declares `MyOptPass`
- the pass is registered as `myopt` in `llvm\lib\Passes\PassRegistry.def`

The existing build tree is `C:\Users\dkirthik\llvm-build` (Ninja, Release, X86, MinGW GCC).

## Build `opt.exe`

`C:\Users\dkirthik\llvm-build\bin\opt.exe` is built from the configured Ninja tree (Release, X86, MinGW GCC) with:

```powershell
cd C:\Users\dkirthik\llvm-build
C:\Users\dkirthik\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\ninja.exe opt
```

The same build as one command:

```powershell
cmake --build C:\Users\dkirthik\llvm-build --target opt
```

Run that again after changing `MyOpt.cpp`. It rebuilds the pass and relinks `opt.exe`.

To configure that build tree from scratch:

```powershell
cmake -G Ninja `
  -S C:\Users\dkirthik\llvm-project\llvm `
  -B C:\Users\dkirthik\llvm-build `
  -DCMAKE_BUILD_TYPE=Release `
  -DLLVM_TARGETS_TO_BUILD=X86 `
  -DCMAKE_C_COMPILER=C:\Users\dkirthik\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe `
  -DCMAKE_CXX_COMPILER=C:\Users\dkirthik\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe

cmake --build C:\Users\dkirthik\llvm-build --target opt
```

A full configure builds a large part of LLVM. After the first configure, `ninja opt` only rebuilds what changed.

## Build and run on Linux

Packages on Debian or Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build clang zlib1g-dev
```

Copy this folder to the Linux machine, for example `~/MyOpt`, and use an LLVM source tree that already contains the pass (`llvm/include/llvm/Transforms/Utils/MyOpt.h`, `MyOpt.cpp` in `llvm/lib/Transforms/Utils/CMakeLists.txt`, `#include "llvm/Transforms/Utils/MyOpt.h"` in `llvm/lib/Passes/PassBuilder.cpp`, and `FUNCTION_PASS("myopt", MyOptPass())` in `llvm/lib/Passes/PassRegistry.def`).

The in-tree wrapper currently includes the Windows path. Point it at the Linux copy before building. `llvm/lib/Transforms/Utils/MyOpt.cpp` should be:

```cpp
#include "/home/you/MyOpt/MyOpt.cpp"
```

Use the real path of `MyOpt.cpp`.

Configure and build `opt` (Release, X86, the system GCC):

```bash
cmake -G Ninja \
  -S ~/llvm-project/llvm \
  -B ~/llvm-build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++

cmake --build ~/llvm-build --target opt
```

After the first configure, rebuild with:

```bash
cd ~/llvm-build
ninja opt
```

The binary is `~/llvm-build/bin/opt`.

Generate the IR and run the pass:

```bash
cd ~/MyOpt

clang -S -emit-llvm -O0 -Xclang -disable-O0-optnone -fno-discard-value-names \
  opt_tests.c -o unoptimized.ll

~/llvm-build/bin/opt -S -passes=myopt \
  unoptimized.ll -o optimized.ll
```

On a fresh LLVM checkout, add the same four hooks before the `cmake` command, and add `MyOpt.cpp` to the source list in `llvm/lib/Transforms/Utils/CMakeLists.txt` next to the other `.cpp` files. Then run the configure and `ninja opt` commands above.

## Run on Windows

`-disable-O0-optnone` keeps the `-O0` allocas (the pass forwards through those) and does not mark the functions `optnone`, which would otherwise skip a non-required pass.

```powershell
cd C:\Users\dkirthik\Downloads\MyOpt

clang -S -emit-llvm -O0 -Xclang -disable-O0-optnone -fno-discard-value-names `
  opt_tests.c -o unoptimized.ll

C:\Users\dkirthik\llvm-build\bin\opt.exe -S -passes=myopt `
  unoptimized.ll -o optimized.ll
```

`clang` here is `C:\Users\dkirthik\AppData\Local\Programs\LLVM\bin\clang.exe`. Rewrite notes from the pass go to stderr; the IR is written to `optimized.ll`.

`unoptimized.ll` is the `-O0` IR. `optimized.ll` is that IR after `myopt`.
