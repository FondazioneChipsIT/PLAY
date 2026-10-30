#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int matrix_mul_pulp_open_cluster(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int row_start;
    int row_end;
    int rows_pair;
    int cols_pair;
    int m, n, p;
    int block;
    int left;
    int id;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    if (dim_N < 1) {
        for (m = row_start; m < row_end; m++)
            for (p = 0; p < dim_P; p++)
                dst[m * dim_P + p] = 0;
        goto exit;
    }

    /* 2x2 blocks of dst: every step over n loads two elements of A and two of B
     * and feeds four independent FMAs (one load per FMA instead of two) */
    rows_pair = row_start + ((row_end - row_start) & ~1);
    cols_pair = dim_P & ~1;

    for (m = row_start; m < rows_pair; m += 2) {    /* rows m, m+1 of dst */
        const float *a0 = src_a + m * dim_N;        /* row m of A */
        const float *a1 = a0 + dim_N;               /* row m+1 of A */

        for (p = 0; p < cols_pair; p += 2) {        /* columns p, p+1 of dst */
            const float *b = src_b + p;             /* B[0][p] */
            float s00, s01, s10, s11;               /* dst[m][p], dst[m][p+1], dst[m+1][p], dst[m+1][p+1] */

            s00 = 0;
            s01 = 0;
            s10 = 0;
            s11 = 0;
            n = 0;
            do {
                float a0n, a1n;
                float bn0, bn1;

                a0n = a0[n];                        /* A[m][n] */
                a1n = a1[n];                        /* A[m+1][n] */
                bn0 = b[n * dim_P];                 /* B[n][p] */
                bn1 = b[n * dim_P + 1];             /* B[n][p+1] */

                s00 += a0n * bn0;                   /* dst[m][p] */
                s01 += a0n * bn1;                   /* dst[m][p+1] */
                s10 += a1n * bn0;                   /* dst[m+1][p] */
                s11 += a1n * bn1;                   /* dst[m+1][p+1] */
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s00;
            dst[m * dim_P + (p + 1)] = s01;
            dst[(m + 1) * dim_P + p] = s10;
            dst[(m + 1) * dim_P + (p + 1)] = s11;
        }

        if (p < dim_P) {
            /* odd dim_P: last column, 2x1 block */
            const float *b = src_b + p;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            n = 0;
            do {
                float bn;

                bn = b[n * dim_P];
                s0 += a0[n] * bn;
                s1 += a1[n] * bn;
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s0;
            dst[(m + 1) * dim_P + p] = s1;
        }
    }

    if (m < row_end) {
        /* odd number of rows for this core: last row, 1x2 blocks then 1x1 */
        const float *a0 = src_a + m * dim_N;

        for (p = 0; p < cols_pair; p += 2) {
            const float *b = src_b + p;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            n = 0;
            do {
                float a0n;

                a0n = a0[n];
                s0 += a0n * b[n * dim_P];
                s1 += a0n * b[n * dim_P + 1];
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s0;
            dst[m * dim_P + (p + 1)] = s1;
        }

        if (p < dim_P) {
            float s0;

            s0 = 0;
            n = 0;
            do {
                s0 += a0[n] * src_b[n * dim_P + p];
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s0;
        }
    }

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int matrix_mul_pulp_open_fc(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    float sum;

    for (int m = 0; m < dim_M; m++) {
        for (int p = 0; p < dim_P; p++) {
            sum = 0;
            for (int n = 0; n < dim_N; n++)
                sum += src_a[m * dim_N + n] * src_b[n * dim_P + p];
            dst[m * dim_P + p] = sum;
        }
    }

    return 0;
}

#endif  /* CLUSTER */


int matrix_mul_impl(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_mul_pulp_open_cluster(src_a, src_b, dst, dim_M, dim_N, dim_P);
#else
    ret = matrix_mul_pulp_open_fc(src_a, src_b, dst, dim_M, dim_N, dim_P);
#endif

    return ret;
}
