#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static PI_L1 float ONE_f = 1;
static PI_L1 float EPSILON = 1e-12;

/* one column k of dst: dst[m][k] = (src[m][:] . mat_V[:][k]) * s_inv, or 0 when s is below EPSILON */
static inline void lsv_column(const float *src, const float *mat_V, const float *vec_S, float *dst, const int dim_M, const int dim_N, const int k)
{
    float s, s_inv;
    int m, n;

    s = vec_S[k];
    if (s < EPSILON) {
        for (m = 0; m < dim_M; m++)
            dst[m * dim_N + k] = 0;
        return;
    }
    s_inv = ONE_f / s;

    for (m = 0; m < dim_M; m++) {
        const float *r = src + m * dim_N;
        float sum1, sum2;

        sum1 = 0;
        sum2 = 0;
        for (n = 0; n + 1 < dim_N; n += 2) {
            sum1 += r[n] * mat_V[n * dim_N + k];
            sum2 += r[n + 1] * mat_V[(n + 1) * dim_N + k];
        }
        if (n < dim_N)
            sum1 += r[n] * mat_V[n * dim_N + k];

        dst[m * dim_N + k] = (sum1 + sum2) * s_inv;
    }
}

static int linalg_svd_lsv_pulp_open_cluster(const float *src, float *mat_V, float *vec_S, float *dst, const int dim_M, const int dim_N)
{
    int cols_pair;
    int k, m, n;
    int block;
    int start;
    int left;
    int end;
    int id;

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    /* 2x2 blocks of dst (rows m, m+1 x columns k, k+1): every step over n loads two
     * elements of src and two of mat_V and feeds four independent FMAs */
    cols_pair = start + ((end - start) & ~1);

    for (k = start; k < cols_pair; k += 2) {        /* columns k, k+1 of dst */
        const float *v = mat_V + k;                 /* mat_V[0][k] */
        float s0, s1;
        float s0_inv, s1_inv;

        s0 = vec_S[k];
        s1 = vec_S[k + 1];
        if (s0 < EPSILON || s1 < EPSILON) {
            lsv_column(src, mat_V, vec_S, dst, dim_M, dim_N, k);
            lsv_column(src, mat_V, vec_S, dst, dim_M, dim_N, k + 1);
            continue;
        }
        s0_inv = ONE_f / s0;
        s1_inv = ONE_f / s1;

        for (m = 0; m + 1 < dim_M; m += 2) {        /* rows m, m+1 of dst */
            const float *r0 = src + m * dim_N;      /* row m of src */
            const float *r1 = r0 + dim_N;           /* row m+1 of src */
            float s00, s01, s10, s11;               /* dst[m][k], dst[m][k+1], dst[m+1][k], dst[m+1][k+1], before the 1/s scaling */

            s00 = 0;
            s01 = 0;
            s10 = 0;
            s11 = 0;
            n = 0;
            do {
                float r0n, r1n;
                float v0n, v1n;

                r0n = r0[n];                        /* src[m][n] */
                r1n = r1[n];                        /* src[m+1][n] */
                v0n = v[n * dim_N];                 /* mat_V[n][k] */
                v1n = v[n * dim_N + 1];             /* mat_V[n][k+1] */

                s00 += r0n * v0n;                   /* dst[m][k] */
                s01 += r0n * v1n;                   /* dst[m][k+1] */
                s10 += r1n * v0n;                   /* dst[m+1][k] */
                s11 += r1n * v1n;                   /* dst[m+1][k+1] */
                n++;
            } while (n < dim_N);

            dst[m * dim_N + k] = s00 * s0_inv;
            dst[m * dim_N + (k + 1)] = s01 * s1_inv;
            dst[(m + 1) * dim_N + k] = s10 * s0_inv;
            dst[(m + 1) * dim_N + (k + 1)] = s11 * s1_inv;
        }

        if (m < dim_M) {
            /* odd dim_M: last row, 1x2 block */
            const float *r0 = src + m * dim_N;
            float s00, s01;

            s00 = 0;
            s01 = 0;
            n = 0;
            do {
                float r0n;

                r0n = r0[n];
                s00 += r0n * v[n * dim_N];
                s01 += r0n * v[n * dim_N + 1];
                n++;
            } while (n < dim_N);

            dst[m * dim_N + k] = s00 * s0_inv;
            dst[m * dim_N + (k + 1)] = s01 * s1_inv;
        }
    }

    if (k < end)
        /* odd number of columns for this core */
        lsv_column(src, mat_V, vec_S, dst, dim_M, dim_N, k);

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

/* these variable needs to be re-defined for FC since it does not have access to L1 memory */
static float ONE_f = 1;
static float EPSILON = 1e-12;

int linalg_svd_lsv_pulp_open_fc(const float *src, float *mat_V, float *vec_S, float *dst, const int dim_M, const int dim_N)
{
    float sum;

    for (int k = 0; k < dim_N; k++) {
        /* Optimization */
        if (vec_S[k] < EPSILON) {
            printf("setting %d-th col to 0\n");
            /* Set k-th column of dst to 0 */
            for (int m = 0; m < dim_M; m++)
                dst[m * dim_N + k] = 0;
            continue;
        }

        for (int m = 0; m < dim_M; m++) {
            sum = 0;
            for (int n = 0; n < dim_N; n++)
                sum += src[m * dim_N + n] * mat_V[n * dim_N + k];
            dst[m * dim_N + k] = sum / vec_S[k];
        }
    }
}

#endif  /* CLUSTER */

int linalg_svd_lsv_impl(const float *src, float *mat_V, float *vec_S, float *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_svd_lsv_pulp_open_cluster(src, mat_V, vec_S, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_svd_lsv_pulp_open_fc(src, mat_V, vec_S, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
