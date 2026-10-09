// Times the four matrix multiply functions in kernels.c.
// Each function is called 5 times and the average time is printed.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

void matmul_naive(const double *A, const double *B, double *C, int n);
void matmul_interchange(const double *A, const double *B, double *C, int n);
void matmul_tiling(const double *A, const double *B, double *C, int n);
void matmul_unroll(const double *A, const double *B, double *C, int n);

typedef void (*matmul_fn)(const double *A, const double *B, double *C, int n);

// time one call, repeat 5 times, return the average in seconds.
// N = 64 is shorter than one clock tick, so each sample times 1000 calls.
double average_time(matmul_fn fn, const double *A, const double *B, double *C, int n)
{
    int r, c;
    int calls = (n <= 64) ? 1000 : 1;
    clock_t start, end;
    double total = 0.0;

    for (r = 0; r < 5; r++) {
        start = clock();
        for (c = 0; c < calls; c++) {
            fn(A, B, C, n);
        }
        end = clock();
        total = total + (double)(end - start) / CLOCKS_PER_SEC / calls;
    }
    return total / 5.0;
}

// fill matrix with random numbers between 0 and 1
void fill_matrix(double *M, int n)
{
    int i;
    for (i = 0; i < n * n; i++) {
        M[i] = (double)rand() / RAND_MAX;
    }
}

// compare the optimized matrix against naive, one element at a time
void check_elements(const double *C, const double *ref, int n)
{
    int i, bad = 0;
    int total = n * n;
    double worst = 0.0;

    for (i = 0; i < total; i++) {
        double d = fabs(C[i] - ref[i]);
        if (d > worst) {
            worst = d;
        }
        if (d > 1e-6) {
            bad++;
        }
    }

    if (bad == 0) {
        printf("    accuracy: all %d elements match naive (max |diff| %.3e)\n",
               total, worst);
    } else {
        printf("    accuracy: %d of %d elements differ from naive (max |diff| %.3e)\n",
               bad, total, worst);
    }
}

int main(void)
{
    int sizes[] = {64, 256, 512, 1024};
    int num_sizes = 4;
    int s, n;
    double *A, *B, *C, *ref;
    double t_naive, t;

    for (s = 0; s < num_sizes; s++) {
        n = sizes[s];

        A = (double *)malloc(n * n * sizeof(double));
        B = (double *)malloc(n * n * sizeof(double));
        C = (double *)malloc(n * n * sizeof(double));
        ref = (double *)malloc(n * n * sizeof(double));

        fill_matrix(A, n);
        fill_matrix(B, n);

        printf("N = %d\n", n);

        t_naive = average_time(matmul_naive, A, B, ref, n);
        printf("  naive       : %8.3f ms  (avg of 5)\n", t_naive * 1000.0);

        t = average_time(matmul_interchange, A, B, C, n);
        printf("  interchange : %8.3f ms  uplift %.2fx\n", t * 1000.0, t_naive / t);
        check_elements(C, ref, n);

        t = average_time(matmul_tiling, A, B, C, n);
        printf("  tiling      : %8.3f ms  uplift %.2fx\n", t * 1000.0, t_naive / t);
        check_elements(C, ref, n);

        t = average_time(matmul_unroll, A, B, C, n);
        printf("  unroll      : %8.3f ms  uplift %.2fx\n", t * 1000.0, t_naive / t);
        check_elements(C, ref, n);

        printf("\n");

        free(A);
        free(B);
        free(C);
        free(ref);
    }

    return 0;
}
