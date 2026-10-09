// Matrix multiplication C = A * B
// all matrices are n x n and stored row by row in a 1D array
// so element (i, j) is at index i*n + j

#include <string.h>

#define TILE 32

// basic version, loop order i j k
void matmul_naive(const double *A, const double *B, double *C, int n)
{
    int i, j, k;
    double sum;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            sum = 0;
            for (k = 0; k < n; k++) {
                sum = sum + A[i*n + k] * B[k*n + j];
            }
            C[i*n + j] = sum;
        }
    }
}

// loop interchange: swapped j and k so the order is i k j
// now the inner loop goes along a row of B and C
void matmul_interchange(const double *A, const double *B, double *C, int n)
{
    int i, j, k;
    double a;

    // C has to start at 0 because we keep adding to it
    memset(C, 0, (size_t)n * n * sizeof(double));

    for (i = 0; i < n; i++) {
        for (k = 0; k < n; k++) {
            a = A[i*n + k];
            for (j = 0; j < n; j++) {
                C[i*n + j] = C[i*n + j] + a * B[k*n + j];
            }
        }
    }
}

// loop tiling: same i j k order but done in TILE x TILE blocks
// 3 blocks of 32x32
void matmul_tiling(const double *A, const double *B, double *C, int n)
{
    int i, j, k;
    int ii, jj, kk;
    int i_end, j_end, k_end;
    double sum;

    // C has to start at 0 because each block adds its part to it
    memset(C, 0, (size_t)n * n * sizeof(double));

    for (ii = 0; ii < n; ii = ii + TILE) {
        for (jj = 0; jj < n; jj = jj + TILE) {
            for (kk = 0; kk < n; kk = kk + TILE) {

                // last block can be smaller if n is not a multiple of TILE
                i_end = ii + TILE;
                if (i_end > n) i_end = n;
                j_end = jj + TILE;
                if (j_end > n) j_end = n;
                k_end = kk + TILE;
                if (k_end > n) k_end = n;

                for (i = ii; i < i_end; i++) {
                    for (j = jj; j < j_end; j++) {
                        sum = C[i*n + j];
                        for (k = kk; k < k_end; k++) {
                            sum = sum + A[i*n + k] * B[k*n + j];
                        }
                        C[i*n + j] = sum;
                    }
                }
            }
        }
    }
}

// loop unrolling: k loop unrolled 4 times
// using 4 different sums so the adds don't have to wait for each other
void matmul_unroll(const double *A, const double *B, double *C, int n)
{
    int i, j, k;
    double sum0, sum1, sum2, sum3, sum;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            sum0 = 0;
            sum1 = 0;
            sum2 = 0;
            sum3 = 0;

            for (k = 0; k + 3 < n; k = k + 4) {
                sum0 = sum0 + A[i*n + k]     * B[k*n + j];
                sum1 = sum1 + A[i*n + k + 1] * B[(k+1)*n + j];
                sum2 = sum2 + A[i*n + k + 2] * B[(k+2)*n + j];
                sum3 = sum3 + A[i*n + k + 3] * B[(k+3)*n + j];
            }

            sum = sum0 + sum1 + sum2 + sum3;

            // leftover elements if n is not a multiple of 4
            for (; k < n; k++) {
                sum = sum + A[i*n + k] * B[k*n + j];
            }

            C[i*n + j] = sum;
        }
    }
}
