#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

PI_L1 float *y;

#ifdef  CLUSTER

/* partial sums, double-buffered by row so that one barrier per row is enough */
PI_L1 float local_sum[2][NUM_CORES];

static inline void barrier()
{
#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif
}

int linalg_lu_solve_pulp_open_cluster(const float *mat, const float *vec, const int *perm, float *result, const int dim_M, const int dim_N)
{
    float *y_c;
    float *x_c;
    int tot_ops;
    int rem_ops;
    float sum;
    int start;
    int block;
    int left;
    int end;
    int id;
    int op;
    int k;
    int r;

    id = pi_core_id();
    if (id == 0)
        y = pmsis_l1_malloc(2 * NUM_CORES * dim_M * sizeof(float));

    barrier();

    /* Every core computes each y[m] and x[m] itself from the partial sums and keeps them in
     * its own copy: nothing is shared after the barrier, and core 0 also writes the result */
    y_c = y + id * dim_M;
    x_c = y + (NUM_CORES + id) * dim_M;
    r = 0;

    /* Forward substitution (L * y = perm * v) */
    for (int m = 0; m < dim_M; m++) {
        float sum1, sum2;

        block = m / NUM_CORES;
        left = m % NUM_CORES;
        start = id * block + (id < left ? id : left);
        end = start + block + (id < left ? 1 : 0);
        tot_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        sum1 = 0;
        sum2 = 0;
        k = start;
        op = 0;

        if (tot_ops != 0) {
            do {
                sum1 += mat[m * dim_N + k] * y_c[k];
                sum2 += mat[m * dim_N + (k + 1)] * y_c[k + 1];

                k += 2;
                op++;
            } while (op < tot_ops);
        }

        if (rem_ops)
            sum1 += mat[m * dim_N + (end - 1)] * y_c[end - 1];

        local_sum[r & 1][id] = sum1 + sum2;
        barrier();

        sum = 0;
        for (int i = 0; i < NUM_CORES; i++)
            sum += local_sum[r & 1][i];
        y_c[m] = vec[perm[m]] - sum;
        r++;
    }

    /* Backward substitution (U * x = y) */
    for (int m = (dim_M - 1); m >= 0; m--) {
        float sum1, sum2;

        block = (dim_N - (m + 1)) / NUM_CORES;
        left = (dim_N - (m + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (m + 1);
        end = start + block + (id < left ? 1 : 0);
        tot_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        sum1 = 0;
        sum2 = 0;
        k = start;
        op = 0;

        if (tot_ops != 0) {
            do {
                sum1 += mat[m * dim_N + k] * x_c[k];
                sum2 += mat[m * dim_N + (k + 1)] * x_c[k + 1];

                k += 2;
                op++;
            } while (op < tot_ops);
        }

        if (rem_ops)
            sum1 += mat[m * dim_N + (end - 1)] * x_c[end - 1];

        local_sum[r & 1][id] = sum1 + sum2;
        barrier();

        sum = 0;
        for (int i = 0; i < NUM_CORES; i++)
            sum += local_sum[r & 1][i];
        x_c[m] = (y_c[m] - sum) / mat[m * dim_N + m];
        if (id == 0)
            result[m] = x_c[m];
        r++;
    }

    barrier();

    if (id == 0)
        pmsis_l1_malloc_free(y, 2 * NUM_CORES * dim_M * sizeof(float));

    barrier();

    return 0;
}

#else   /* CLUSTER */

static int linalg_lu_solve_pulp_open_fc(const float *mat, const float *vec, const int *perm, float *result, const int dim_M, const int dim_N)
{
    int perm_idx;
    float sum;

    y = pmsis_l1_malloc(dim_M * sizeof(float));

    /* Forward substitution (L * y = perm * v) */
    for (int m = 0; m < dim_M; m++) {
        sum = 0;
        perm_idx = perm[m];

        for (int k = 0; k < m; k++)
            sum += mat[m * dim_N + k] * y[k];
        y[m] = vec[perm_idx] - sum;
    }

    /* Backward substitution (U * x = y) */
    for (int m = (dim_M - 1); m >= 0; m--) {
        sum = 0;

        for (int k = (m + 1); k < dim_N; k++)
            sum += mat[m * dim_N + k] * result[k];
        result[m] = (y[m] - sum) / mat[m * dim_N + m];
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_lu_solve_impl(const float *mat, const float *vec, const int *perm, float *result, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_lu_solve_pulp_open_cluster(mat, vec, perm, result, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_lu_solve_pulp_open_fc(mat, vec, perm, result, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
