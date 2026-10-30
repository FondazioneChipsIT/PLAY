#include "internal/arch_interface.h"
#include "play.h"

#include <math.h>

#include "pmsis.h"


#ifdef  CLUSTER

PI_L1 float ONE_f = 1.0f;

static inline void barrier()
{
#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif
}

static int linalg_cholesky_decomp_pulp_open_cluster(const float *src, float *dst, const int dim)
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

        /* Every core computes the diagonal element on its own: row m up to column m - 1
         * is complete since the previous column, so there are no partial sums, no
         * reduction on core 0 and no barriers around it. Core 0 stores it. */
        float dst_mm_inv;
        {
            const float *rm = dst + m * dim;
            float sq1 = 0;
            float sq2 = 0;
            float dst_mm;

            num_ops = m / 2;
            n = 0;
            if (num_ops > 0) {
                do {
                    sq1 += rm[n] * rm[n];
                    sq2 += rm[n + 1] * rm[n + 1];
                    n += 2;
                    num_ops--;
                } while (num_ops > 0);
            }
            if (m % 2)
                sq1 += rm[m - 1] * rm[m - 1];

            dst_mm = sqrtf(src[idx_mm] - (sq1 + sq2));
            dst_mm_inv = ONE_f / dst_mm;
            if (id == 0)
                dst[idx_mm] = dst_mm;
        }

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

#else   /* CLUSTER */

float ONE_f = 1.0f;

static int linalg_cholesky_decomp_pulp_open_fc(const float *src, float *dst,  const int dim)
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

int linalg_cholesky_decomp_impl(const float *src, float *dst, const int dim)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_cholesky_decomp_pulp_open_cluster(src, dst, dim);
#else   /* CLUSTER */
    ret = linalg_cholesky_decomp_pulp_open_fc(src, dst, dim);
#endif  /* CLUSTER */

    return ret;
}
