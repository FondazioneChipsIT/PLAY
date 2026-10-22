#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static PI_L1 float ONE_f = 1;
static PI_L1 float EPSILON = 1e-12;

static int linalg_svd_lsv_pulp_open_cluster(const float *src, float *mat_V, float *vec_S, float *dst, const int dim_M, const int dim_N)
{
    int tot_ops_M;
    int rem_ops_M;
    int tot_ops_N;
    int rem_ops_N;
    int k, m, n;
    float sum1;
    float sum2;
    int block;
    int start;
    int left;
    int end;
    int id;
    int op;

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    tot_ops_M = dim_M / 2;
    rem_ops_M = dim_M % 2;
    tot_ops_N = dim_N / 2;
    rem_ops_N = dim_N % 2;

    if (start >= end || dim_M < 2 || dim_N < 2) {
        for (k = start; k < end; k++) {
            float s;

            s = vec_S[k];
            for (m = 0; m < dim_M; m++) {
                sum1 = 0;
                for (n = 0; n < dim_N; n++)
                    sum1 += src[m * dim_N + n] * mat_V[n * dim_N + k];
                dst[m * dim_N + k] = (s < EPSILON ? 0 : sum1 / s);
            }
        }
        goto exit;
    }

    k = start;
    do {
        float s, s_inv;
        s = vec_S[k];

        if (s < EPSILON) {
            op = 0;
            m = 0;
            do {
                dst[m * dim_N + k] = 0;
                dst[(m + 1) * dim_N + k] = 0;

                m +=  2;
                op++;
            } while (op < tot_ops_M);

            if (rem_ops_M)
                dst[(dim_M - 1) * dim_N + k] = 0;

            k++;
            continue;
        } else {
            s_inv = ONE_f / s;
        }

        m = 0;
        do {
            sum1 = 0;
            sum2 = 0;

            n = 0;
            op = 0;
            do {
                float src1, src2;
                float v1, v2;

                src1 = src[m * dim_N + n];
                src2 = src[m * dim_N + (n + 1)];
                v1 = mat_V[n * dim_N + k];
                v2 = mat_V[(n + 1) * dim_N + k];

                sum1 += src1 * v1;
                sum2 += src2 * v2;

                n += 2;
                op++;
            } while (op < tot_ops_N);

            if (rem_ops_N)
                sum1 += src[m * dim_N + (dim_N - 1)] * mat_V[(dim_N - 1) * dim_N + k];

            dst[m * dim_N + k] = (sum1 + sum2) * s_inv;
            m++;
        } while (m < dim_M);

        k++;
    } while (k < end);

exit:

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
