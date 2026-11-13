#include "internal/arch_interface.h"
#include "play.h"

#include <math.h>

#include "pmsis.h"


#ifdef  CLUSTER

PI_L1 float ONE_f = 1.0f;

PI_L1 float local_sum1[NUM_CORES];
PI_L1 float local_sum2[NUM_CORES];

static inline void barrier()
{
#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif
}

#ifdef PLAY_PACKED

static int linalg_cholesky_decomp_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim)
{
    const play_fmt_v2_t *a2;
    const play_fmt_v2_t *b2;
    const play_fmt_v2_t *m2;
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int m, n, k, j;
    int num_ops;
    int rem_ops;
    int tot_ops;
    int pairs;
    int block;
    int start;
    int left;
    int end;
    int id;
    int op;

    id = pi_core_id();

    for (m = 0; m < dim; m++) {
        int idx_mm;

        idx_mm = m * dim + m;
        sum1 = 0;
        sum2_1[0] = 0;
        sum2_1[1] = 0;
        sum2_2[0] = 0;
        sum2_2[1] = 0;

        block = m / NUM_CORES;
        left = m % NUM_CORES;
        start = id * block + (id < left ? id : left);
        end = start + block + (id < left ? 1 : 0);

        n = start;
        if ((n & 1) && (n < end)) {
            sum1 += dst[m * dim + n] * dst[m * dim + n];
            n++;
        }

        a2 = (const play_fmt_v2_t *) &dst[m * dim + n];
        tot_ops = (end - n) / 4;
        op = 0;

        if (tot_ops > 0) {
            do {
                sum2_1 += a2[0] * a2[0];
                sum2_2 += a2[1] * a2[1];

                a2 += 2;
                op++;
            } while (op < tot_ops);
            n += 4 * tot_ops;
        }

        for (; n < end; n++)
            sum1 += dst[m * dim + n] * dst[m * dim + n];

        local_sum1[id] = sum1 + sum2_1[0] + sum2_1[1];
        local_sum2[id] = sum2_2[0] + sum2_2[1];

        barrier();

        if (id == 0) {
            play_fmt_t sum = 0;

            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum1[i] + local_sum2[i];

            dst[idx_mm] = sqrtf(src[idx_mm] - sum);
        }   /* id 0 */

        barrier();

        play_fmt_t dst_mm_inv = ONE_f / dst[idx_mm];
        block = (dim - (m + 1)) / NUM_CORES;
        left = (dim - (m + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (m + 1);
        end = start + block + (id < left ? 1 : 0);
        num_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        pairs = m / 2;
        m2 = (const play_fmt_v2_t *) &dst[m * dim];
        n = start;

        for (op = 0; op < num_ops; op++) {
            a2 = (const play_fmt_v2_t *) &dst[n * dim];
            b2 = (const play_fmt_v2_t *) &dst[(n + 1) * dim];
            sum2_1[0] = 0;
            sum2_1[1] = 0;
            sum2_2[0] = 0;
            sum2_2[1] = 0;

            for (j = 0; j < pairs; j++) {
                play_fmt_v2_t dm;

                dm = m2[j];
                sum2_1 += a2[j] * dm;
                sum2_2 += b2[j] * dm;
            }

            sum1 = sum2_1[0] + sum2_1[1];
            sum2 = sum2_2[0] + sum2_2[1];
            if (m % 2) {
                k = m - 1;
                sum1 += dst[n * dim + k] * dst[m * dim + k];
                sum2 += dst[(n + 1) * dim + k] * dst[m * dim + k];
            }

            dst[n * dim + m] = dst_mm_inv * (src[n * dim + m] - sum1);
            dst[(n + 1) * dim + m] = dst_mm_inv * (src[(n + 1) * dim + m] - sum2);

            n += 2;
        }

        if (rem_ops) {
            a2 = (const play_fmt_v2_t *) &dst[(end - 1) * dim];
            sum2_1[0] = 0;
            sum2_1[1] = 0;

            for (j = 0; j < pairs; j++)
                sum2_1 += a2[j] * m2[j];

            sum1 = sum2_1[0] + sum2_1[1];
            if (m % 2)
                sum1 += dst[(end - 1) * dim + (m - 1)] * dst[m * dim + (m - 1)];

            dst[(end - 1) * dim + m] = dst_mm_inv * (src[(end - 1) * dim + m] - sum1);
        }

        barrier();
    }

    return 0;
}

#else   /* PLAY_PACKED */

static int linalg_cholesky_decomp_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim)
{
    int m, n, k;
    int num_ops;
    int rem_ops;
    int block;
    int start;
    int left;
    int end;
    int id;
    int op;

    id = pi_core_id();

    for (m = 0; m < dim; m++) {
        int idx_mm;

        idx_mm = m * dim + m;
        local_sum1[id] = 0;
        local_sum2[id] = 0;

        block = m / NUM_CORES;
        left = m % NUM_CORES;
        start = id * block + (id < left ? id : left);
        end = start + block + (id < left ? 1 : 0);
        num_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        n = start;

        for (op = 0; op < num_ops; op++) {
            int idx1, idx2;
            float dst1, dst2;

            idx1 = m * dim + n;
            idx2 = m * dim + (n + 1);
            dst1 = dst[idx1];
            dst2 = dst[idx2];

            local_sum1[id] += dst1 * dst1;
            local_sum2[id] += dst2 * dst2;
            n += 2;
        }

        if (rem_ops)
            local_sum1[id] += dst[m * dim + (end - 1)] * dst[m * dim + (end - 1)];

        barrier();

        if (id == 0) {
            float sum = 0;

            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum1[i] + local_sum2[i];

            dst[idx_mm] = sqrtf(src[idx_mm] - sum);
        }   /* id 0 */

        barrier();

        float dst_mm_inv = ONE_f / dst[idx_mm];
        block = (dim - (m + 1)) / NUM_CORES;
        left = (dim - (m + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (m + 1);
        end = start + block + (id < left ? 1 : 0);
        num_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        n = start;

        for (op = 0; op < num_ops; op++) {
            float sum1 = 0;
            float sum2 = 0;

            for (k = 0; k < m; k++) {
                int idx_nk1, idx_nk2, idx_mk;
                float dst1, dst2, dst_mk;

                idx_mk = m * dim + k;
                idx_nk1 = n * dim + k;
                idx_nk2 = (n + 1) * dim + k;

                dst_mk = dst[idx_mk];
                dst1 = dst[idx_nk1];
                dst2 = dst[idx_nk2];

                sum1 += dst1 * dst_mk;
                sum2 += dst2 * dst_mk;
            }

            float src_nm1, src_nm2;
            int idx_nm1, idx_nm2;

            idx_nm1 = n * dim + m;
            idx_nm2 = (n + 1) * dim + m;
            src_nm1 = src[idx_nm1];
            src_nm2 = src[idx_nm2];

            dst[idx_nm1] = dst_mm_inv * (src_nm1 - sum1);
            dst[idx_nm2] = dst_mm_inv * (src_nm2 - sum2);

            n += 2;
        }

        if (rem_ops) {
            float sum = 0;
            for (k = 0; k < m; k++) {
                sum += dst[(end - 1) * dim + k] * dst[m * dim + k];
            }
            dst[(end - 1) * dim + m] = dst_mm_inv * (src[(end - 1) * dim + m] - sum);
        }

        barrier();
    }

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

float ONE_f = 1.0f;

static int linalg_cholesky_decomp_pulp_open_fc(const play_fmt_t *src, play_fmt_t *dst,  const int dim)
{
    for (int m = 0; m < dim; m++) {
        float sum = 0;
        for (int n = 0; n < m; n++)
            sum += dst[m * dim + n] * dst[m * dim + n];
        dst[m * dim + m] = sqrtf(src[m * dim + m] - sum);

        for (int n = (m + 1); n < dim; n++) {
            float sum = 0;
            for (int k = 0; k < m; k++) {
                sum += dst[n * dim + k] * dst[m * dim + k];
            }
            dst[n * dim + m] = (ONE_f / dst[m * dim + m]) * (src[n * dim + m] - sum);
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_cholesky_decomp_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_cholesky_decomp_pulp_open_cluster(src, dst, dim);
#else   /* CLUSTER */
    ret = linalg_cholesky_decomp_pulp_open_fc(src, dst, dim);
#endif  /* CLUSTER */

    return ret;
}
