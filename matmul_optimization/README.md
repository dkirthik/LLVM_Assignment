# Matrix Multiplication - Loop Optimizations

For this assignment I wrote a basic matrix multiplication in C and then tried three loop optimizations on it: loop interchange, loop tiling, and loop unrolling. I timed each one against the basic version to see how much faster it got.

All matrices are square, stored row-major, and use `double`.

## Files

- `kernels.c` - the four matrix multiply functions
- `bench.c` - main program that fills the matrices, times each function, and checks the answer

## The four versions

1. **naive** - the normal triple loop in `i, j, k` order. The sum is kept in a local variable and written to `C` once.
2. **interchange** - same thing but the loops are in `i, k, j` order. Now the inner loop goes along a row of B and C, so memory is accessed one after another instead of jumping by N each time. Each element of `C` is updated on every `k`, so `C` is cleared with `memset` first.
3. **tiling** - keeps the `i, j, k` order but splits the matrices into 32x32 blocks. Three 32x32 blocks of doubles are 24 KB, which fits in the 32 KB L1 cache. `C` is also cleared with `memset`, because each block adds onto what is already there.
4. **unroll** - the `k` loop is unrolled 4 times and uses 4 separate sums, so the CPU doesn't have to wait for one add to finish before starting the next one.

Each optimized version only changes one thing compared to naive, so I can see what each optimization does by itself. I didn't combine them.

I also turned off GCC's own loop optimizations (auto-vectorization, auto-unrolling, auto-interchange). Otherwise the compiler would apply some of these changes to the naive version too and the comparison wouldn't be fair.

The compile command does this in two steps. `-O3` turns on all of GCC's optimizations, including the loop ones. The `-fno-...` flags after it then switch off just the loop transformations.

## How to run

`bench.c` is the main program. The four multiply functions live in `kernels.c`, so both files have to be compiled and then linked together.

```command to run
gcc -O3 -std=c11 -march=native -ffp-contract=on -fno-fast-math -fno-unroll-loops -fno-peel-loops -fno-loop-unroll-and-jam -fno-loop-interchange -fno-tree-vectorize -fno-tree-loop-vectorize -fno-tree-slp-vectorize -fno-tree-loop-distribution -fno-version-loops-for-strides -fno-split-loops -Wall -Wextra -Wno-unknown-pragmas -o matmul.exe bench.c kernels.c -lm
.\matmul.exe
```

`bench.c` has no command-line arguments. For N = 64, 256, 512, and 1024 it:

1. Fills A and B with random values.
2. Calls each function 5 times, with `clock()` immediately before and after each call, and keeps the average. One call at N = 64 is shorter than a clock tick, so each of those 5 samples times 1000 calls and divides by 1000.
3. Prints that average time in milliseconds and the uplift against naive.
4. Compares every element of the result with the naive result, and prints how many differ and the largest difference.



## Machine config

- CPU: AMD Ryzen 7 PRO 6850U (8 cores, 16 threads, 2.7 GHz base)
- Cache: 32 KB L1 data per core, 512 KB L2 per core, 16 MB L3 shared
- RAM: 32 GB DDR5-4800
- OS: Windows 11 Enterprise 64-bit
- Compiler: GCC 16.2.0 with `-O3 -march=native`


## Results

Uplift is the naive time divided by that version's time.
Accuracy: every element of each optimized result is compared with naive; interchange and tiling matched exactly, and unroll stayed within about 1e-12.

Unroll is faster at N = 64, where the three matrices (96 KB) fit in the 512 KB L2 cache. At the larger sizes it is about the same as naive or slower.

### N = 64

| Version | Time (ms) | Uplift |
|---|---:|---:|
| naive | 0.196 | 1.00x |
| interchange | 0.143 | 1.37x |
| tiling | 0.166 | 1.19x |
| unroll | 0.090 | 2.19x |

### N = 256

| Version | Time (ms) | Uplift |
|---|---:|---:|
| naive | 32.200 | 1.00x |
| interchange | 7.800 | 4.13x |
| tiling | 23.800 | 1.35x |
| unroll | 35.400 | 0.91x |

### N = 512

| Version | Time (ms) | Uplift |
|---|---:|---:|
| naive | 964.800 | 1.00x |
| interchange | 62.600 | 15.41x |
| tiling | 346.400 | 2.79x |
| unroll | 981.600 | 0.98x |

### N = 1024

| Version | Time (ms) | Uplift |
|---|---:|---:|
| naive | 7981.000 | 1.00x |
| interchange | 565.200 | 14.12x |
| tiling | 2930.800 | 2.72x |
| unroll | 9507.200 | 0.84x |
