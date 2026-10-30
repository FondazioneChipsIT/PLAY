#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int matrix_mul_trans_B_pulp_open_cluster(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int row_start;
    int row_end;
    int rows_pair;
    int m, n, p;
    int block;
    int left;
    int id;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    if (dim_N < 1) {
        for (p = row_start; p < row_end; p++)
            for (m = 0; m < dim_M; m++)
                dst[m * dim_P + p] = 0;
        goto exit;
    }

    /* 2x2 blocks of dst: every step over n loads two elements of A (rows m, m+1)
     * and two of B (rows p, p+1) and feeds four independent FMAs */
    rows_pair = row_start + ((row_end - row_start) & ~1);

    for (p = row_start; p < rows_pair; p += 2) {    /* columns p, p+1 of dst */
        const float *b0 = src_b + p * dim_N;        /* row p of B */
        const float *b1 = b0 + dim_N;               /* row p+1 of B */

        for (m = 0; m + 1 < dim_M; m += 2) {        /* rows m, m+1 of dst */
            const float *a0 = src_a + m * dim_N;    /* row m of A */
            const float *a1 = a0 + dim_N;           /* row m+1 of A */
            float s00, s01, s10, s11;               /* dst[m][p], dst[m][p+1], dst[m+1][p], dst[m+1][p+1] */

            s00 = 0;
            s01 = 0;
            s10 = 0;
            s11 = 0;
            n = 0;
            do {
                float a0n, a1n;
                float b0n, b1n;

                a0n = a0[n];                        /* A[m][n] */
                a1n = a1[n];                        /* A[m+1][n] */
                b0n = b0[n];                        /* B[p][n] */
                b1n = b1[n];                        /* B[p+1][n] */

                s00 += a0n * b0n;                   /* dst[m][p] */
                s01 += a0n * b1n;                   /* dst[m][p+1] */
                s10 += a1n * b0n;                   /* dst[m+1][p] */
                s11 += a1n * b1n;                   /* dst[m+1][p+1] */
                n++;
            } while (n < dim_N);

            /* each result is written once */
            dst[m * dim_P + p] = s00;
            dst[m * dim_P + (p + 1)] = s01;
            dst[(m + 1) * dim_P + p] = s10;
            dst[(m + 1) * dim_P + (p + 1)] = s11;
        }

        if (m < dim_M) {
            /* odd dim_M: last row of A, 1x2 block */
            const float *a0 = src_a + m * dim_N;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            n = 0;
            do {
                float a0n;

                a0n = a0[n];
                s0 += a0n * b0[n];
                s1 += a0n * b1[n];
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s0;
            dst[m * dim_P + (p + 1)] = s1;
        }
    }

    if (p < row_end) {
        /* odd number of rows of B for this core: last row, 2x1 blocks then 1x1 */
        const float *b0 = src_b + p * dim_N;

        for (m = 0; m + 1 < dim_M; m += 2) {
            const float *a0 = src_a + m * dim_N;
            const float *a1 = a0 + dim_N;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            n = 0;
            do {
                float b0n;

                b0n = b0[n];
                s0 += a0[n] * b0n;
                s1 += a1[n] * b0n;
                n++;
            } while (n < dim_N);

            dst[m * dim_P + p] = s0;
            dst[(m + 1) * dim_P + p] = s1;
        }

        if (m < dim_M) {
            const float *a0 = src_a + m * dim_N;
            float s0;

            s0 = 0;
            n = 0;
            do {
                s0 += a0[n] * b0[n];
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

static int matrix_mul_trans_B_pulp_open_fc(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    float sum;

    for (int m = 0; m < dim_M; m++) {
        for (int p = 0; p < dim_P; p++) {
            sum = 0;
            for (int n = 0; n < dim_N; n++)
                sum += src_a[m * dim_N + n] * src_b[p * dim_N + n];
            dst[m * dim_P + p] = sum;
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int matrix_mul_trans_B_impl(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_mul_trans_B_pulp_open_cluster(src_a, src_b, dst, dim_M, dim_N, dim_P);
#else
    ret = matrix_mul_trans_B_pulp_open_fc(src_a, src_b, dst, dim_M, dim_N, dim_P);
#endif

    return ret;
}
