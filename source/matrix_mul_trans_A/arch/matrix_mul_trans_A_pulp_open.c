#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int matrix_mul_trans_A_pulp_open_cluster(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int cols_pair;
    int m, n, p;
    int start;
    int block;
    int left;
    int end;
    int id;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    if (dim_M < 1) {
        for (p = start; p < end; p++)
            for (n = 0; n < dim_N; n++)
                dst[n * dim_P + p] = 0;
        goto exit;
    }

    /* 2x2 blocks of dst: every step over m loads two elements of A (columns n, n+1)
     * and two of B (columns p, p+1) and feeds four independent FMAs */
    cols_pair = start + ((end - start) & ~1);

    for (p = start; p < cols_pair; p += 2) {        /* columns p, p+1 of dst */
        const float *b = src_b + p;                 /* B[0][p] */

        for (n = 0; n + 1 < dim_N; n += 2) {        /* rows n, n+1 of dst */
            const float *a = src_a + n;             /* A[0][n] */
            float s00, s01, s10, s11;               /* dst[n][p], dst[n][p+1], dst[n+1][p], dst[n+1][p+1] */

            s00 = 0;
            s01 = 0;
            s10 = 0;
            s11 = 0;
            m = 0;
            do {
                float a0, a1;
                float b0, b1;

                a0 = a[m * dim_N];                  /* A[m][n] */
                a1 = a[m * dim_N + 1];              /* A[m][n+1] */
                b0 = b[m * dim_P];                  /* B[m][p] */
                b1 = b[m * dim_P + 1];              /* B[m][p+1] */

                s00 += a0 * b0;                     /* dst[n][p] */
                s01 += a0 * b1;                     /* dst[n][p+1] */
                s10 += a1 * b0;                     /* dst[n+1][p] */
                s11 += a1 * b1;                     /* dst[n+1][p+1] */
                m++;
            } while (m < dim_M);

            dst[n * dim_P + p] = s00;
            dst[n * dim_P + (p + 1)] = s01;
            dst[(n + 1) * dim_P + p] = s10;
            dst[(n + 1) * dim_P + (p + 1)] = s11;
        }

        if (n < dim_N) {
            /* odd dim_N: last column of A, 1x2 block */
            const float *a = src_a + n;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            m = 0;
            do {
                float a0;

                a0 = a[m * dim_N];
                s0 += a0 * b[m * dim_P];
                s1 += a0 * b[m * dim_P + 1];
                m++;
            } while (m < dim_M);

            dst[n * dim_P + p] = s0;
            dst[n * dim_P + (p + 1)] = s1;
        }
    }

    if (p < end) {
        /* odd number of columns for this core: last column of B, 2x1 blocks then 1x1 */
        const float *b = src_b + p;

        for (n = 0; n + 1 < dim_N; n += 2) {
            const float *a = src_a + n;
            float s0, s1;

            s0 = 0;
            s1 = 0;
            m = 0;
            do {
                float b0;

                b0 = b[m * dim_P];
                s0 += a[m * dim_N] * b0;
                s1 += a[m * dim_N + 1] * b0;
                m++;
            } while (m < dim_M);

            dst[n * dim_P + p] = s0;
            dst[(n + 1) * dim_P + p] = s1;
        }

        if (n < dim_N) {
            float s0;

            s0 = 0;
            m = 0;
            do {
                s0 += src_a[m * dim_N + n] * b[m * dim_P];
                m++;
            } while (m < dim_M);

            dst[n * dim_P + p] = s0;
        }
    }

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int matrix_mul_trans_A_pulp_open_fc(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    float sum;

    for (int n = 0; n < dim_N; n++) {
        for (int p = 0; p < dim_P; p++) {
            sum = 0;
            for (int m = 0; m < dim_M; m++)
                sum += src_a[m * dim_N + n] * src_b[m * dim_P + p];
            dst[n * dim_P + p] = sum;
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int matrix_mul_trans_A_impl(const float *src_a, const float *src_b, float *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_mul_trans_A_pulp_open_cluster(src_a, src_b, dst, dim_M, dim_N, dim_P);
#else
    ret = matrix_mul_trans_A_pulp_open_fc(src_a, src_b, dst, dim_M, dim_N, dim_P);
#endif

    return ret;
}
