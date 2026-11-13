#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static PI_L1 float ONE_f = 1;
static PI_L1 float EPSILON = 1e-12;

#ifdef PLAY_PACKED

static inline void lsv_col(const play_fmt_t *src, const play_fmt_t *mat_V, const play_fmt_t s, play_fmt_t *dst, const int k, const int dim_M, const int dim_N)
{
    play_fmt_t s_inv;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int m, n;

    if (s < EPSILON) {
        for (m = 0; m < dim_M; m++)
            dst[m * dim_N + k] = 0;
        return;
    }
    s_inv = ONE_f / s;

    for (m = 0; m < dim_M; m++) {
        sum1 = 0;
        sum2 = 0;
        for (n = 0; (n + 1) < dim_N; n += 2) {
            sum1 += src[m * dim_N + n] * mat_V[n * dim_N + k];
            sum2 += src[m * dim_N + (n + 1)] * mat_V[(n + 1) * dim_N + k];
        }
        if (n < dim_N)
            sum1 += src[m * dim_N + n] * mat_V[n * dim_N + k];
        dst[m * dim_N + k] = (sum1 + sum2) * s_inv;
    }
}

static int linalg_svd_lsv_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_v2_t x2_1;
    play_fmt_v2_t x2_2;
    play_fmt_v2_t s_inv2;
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

    k = start;
    if ((k & 1) && (k < end)) {
        lsv_col(src, mat_V, vec_S[k], dst, k, dim_M, dim_N);
        k++;
    }

    for (; (k + 1) < end; k += 2) {
        s_inv2[0] = (vec_S[k] < EPSILON) ? 0 : ONE_f / vec_S[k];
        s_inv2[1] = (vec_S[k + 1] < EPSILON) ? 0 : ONE_f / vec_S[k + 1];

        for (m = 0; m < dim_M; m++) {
            const play_fmt_t *s;
            const play_fmt_t *v;

            s = &src[m * dim_N];
            v = &mat_V[k];
            sum2_1[0] = 0;
            sum2_1[1] = 0;
            sum2_2[0] = 0;
            sum2_2[1] = 0;
            for (n = 0; (n + 1) < dim_N; n += 2) {
                x2_1[0] = s[n];
                x2_1[1] = x2_1[0];
                x2_2[0] = s[n + 1];
                x2_2[1] = x2_2[0];

                sum2_1 += x2_1 * *(const play_fmt_v2_t *) &v[0];
                sum2_2 += x2_2 * *(const play_fmt_v2_t *) &v[dim_N];
                v += 2 * dim_N;
            }
            if (n < dim_N) {
                x2_1[0] = s[n];
                x2_1[1] = x2_1[0];
                sum2_1 += x2_1 * *(const play_fmt_v2_t *) &v[0];
            }
            *(play_fmt_v2_t *) &dst[m * dim_N + k] = (sum2_1 + sum2_2) * s_inv2;
        }
    }

    if (k < end)
        lsv_col(src, mat_V, vec_S[k], dst, k, dim_M, dim_N);

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int linalg_svd_lsv_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N)
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

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

/* these variable needs to be re-defined for FC since it does not have access to L1 memory */
static float ONE_f = 1;
static float EPSILON = 1e-12;

int linalg_svd_lsv_pulp_open_fc(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N)
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

int linalg_svd_lsv_impl(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_svd_lsv_pulp_open_cluster(src, mat_V, vec_S, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_svd_lsv_pulp_open_fc(src, mat_V, vec_S, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
