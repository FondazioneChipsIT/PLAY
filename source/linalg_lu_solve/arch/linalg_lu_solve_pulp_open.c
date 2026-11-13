#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

PI_L1 play_fmt_t *y;

#ifdef  CLUSTER

PI_L1 play_fmt_t local_sum[NUM_CORES];

static inline void barrier()
{
#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif
}
#ifdef PLAY_PACKED

static inline play_fmt_t row_dot(const play_fmt_t *row, const play_fmt_t *vec, const int start, const int end)
{
    const play_fmt_v2_t *a2;
    const play_fmt_v2_t *v2;
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_t sum;
    int tot_ops;
    int op;
    int k;

    sum2_1[0] = 0;
    sum2_1[1] = 0;
    sum2_2[0] = 0;
    sum2_2[1] = 0;
    sum = 0;

    k = start;
    if ((k & 1) && (k < end)) {
        sum += row[k] * vec[k];
        k++;
    }

    a2 = (const play_fmt_v2_t *) &row[k];
    v2 = (const play_fmt_v2_t *) &vec[k];
    tot_ops = (end - k) / 4;
    op = 0;

    if (tot_ops > 0) {
        do {
            sum2_1 += a2[0] * v2[0];
            sum2_2 += a2[1] * v2[1];

            a2 += 2;
            v2 += 2;
            op++;
        } while (op < tot_ops);
        k += 4 * tot_ops;
    }

    for (; k < end; k++)
        sum += row[k] * vec[k];

    sum2_1 += sum2_2;
    return sum + sum2_1[0] + sum2_1[1];
}

int linalg_lu_solve_pulp_open_cluster(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N)
{
    int perm_idx;
    play_fmt_t sum;
    int start;
    int block;
    int left;
    int end;
    int id;

    id = pi_core_id();
    if (id == 0)
        y = pmsis_l1_malloc(dim_M * sizeof(play_fmt_t));

    barrier();

    for (int m = 0; m < dim_M; m++) {
        sum = 0;
        perm_idx = perm[m];

        block = m / NUM_CORES;
        left = m % NUM_CORES;
        start = id * block + (id < left ? id : left);
        end = start + block + (id < left ? 1 : 0);

        local_sum[id] = row_dot(&mat[m * dim_N], y, start, end);

        barrier();
        if (id == 0) {
            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum[i];
            y[m] = vec[perm_idx] - sum;
        }
        barrier();
    }

    for (int m = (dim_M - 1); m >= 0; m--) {
        sum = 0;

        block = (dim_N - (m + 1)) / NUM_CORES;
        left = (dim_N - (m + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (m + 1);
        end = start + block + (id < left ? 1 : 0);

        local_sum[id] = row_dot(&mat[m * dim_N], result, start, end);

        barrier();
        if (id == 0) {
            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum[i];
            result[m] = (y[m] - sum) / mat[m * dim_N + m];
        }
        barrier();
    }

    if (id == 0)
        pmsis_l1_malloc_free(y, dim_M * sizeof(play_fmt_t));

    barrier();

    return 0;
}

#else   /* PLAY_PACKED */


int linalg_lu_solve_pulp_open_cluster(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N)
{
    int perm_idx;
    int tot_ops;
    int rem_ops;
    play_fmt_t sum;
    int start;
    int block;
    int left;
    int end;
    int id;
    int op;
    int k;

    id = pi_core_id();
    if (id == 0)
        y = pmsis_l1_malloc(dim_M * sizeof(play_fmt_t));

    barrier();

    /* Forward substitution (L * y = perm * v) */
    for (int m = 0; m < dim_M; m++) {
        sum = 0;
        local_sum[id] = 0;
        perm_idx = perm[m];

        block = m / NUM_CORES;
        left = m % NUM_CORES;
        start = id * block + (id < left ? id : left);
        end = start + block + (id < left ? 1 : 0);
        tot_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        k = start;
        op = 0;

        if (tot_ops != 0) {
            do {
                play_fmt_t y1, y2;
                play_fmt_t mat1, mat2;

                y1 = y[k];
                y2 = y[k + 1];
                mat1 = mat[m * dim_N + k];
                mat2 = mat[m * dim_N + (k + 1)];

                local_sum[id] += mat1 * y1;
                local_sum[id] += mat2 * y2;

                k += 2;
                op++;
            } while (op < tot_ops);
        }

        if (rem_ops)
            local_sum[id] += mat[m * dim_N + (end - 1)] * y[end - 1];

        barrier();
        if (id == 0) {
            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum[i];
            y[m] = vec[perm_idx] - sum;
        }
        barrier();
    }

    /* Backward substitution (U * x = y) */
    for (int m = (dim_M - 1); m >= 0; m--) {
        sum = 0;
        local_sum[id] = 0;

        block = (dim_N - (m + 1)) / NUM_CORES;
        left = (dim_N - (m + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (m + 1);
        end = start + block + (id < left ? 1 : 0);
        tot_ops = (end - start) / 2;
        rem_ops = (end - start) % 2;
        k = start;
        op = 0;

        if (tot_ops != 0) {
            do {
                play_fmt_t mat1, mat2;
                play_fmt_t result1, result2;

                result1 = result[k];
                result2 = result[k + 1];
                mat1 = mat[m * dim_N + k];
                mat2 = mat[m * dim_N + (k + 1)];

                local_sum[id] += mat1 * result1;
                local_sum[id] += mat2 * result2;

                k += 2;
                op++;
            } while (op < tot_ops);
        }

        if (rem_ops)
            local_sum[id] += mat[m * dim_N + (end - 1)] * result[end - 1];

        barrier();
        if (id == 0) {
            for (int i = 0; i < NUM_CORES; i++)
                sum += local_sum[i];
            result[m] = (y[m] - sum) / mat[m * dim_N + m];
        }
        barrier();
    }

    if (id == 0)
        pmsis_l1_malloc_free(y, dim_M * sizeof(play_fmt_t));

    barrier();

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int linalg_lu_solve_pulp_open_fc(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N)
{
    int perm_idx;
    play_fmt_t sum;

    y = pmsis_l1_malloc(dim_M * sizeof(play_fmt_t));

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

int linalg_lu_solve_impl(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_lu_solve_pulp_open_cluster(mat, vec, perm, result, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_lu_solve_pulp_open_fc(mat, vec, perm, result, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
