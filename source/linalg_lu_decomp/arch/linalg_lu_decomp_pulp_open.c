#include "internal/arch_interface.h"
#include "play.h"

#include <math.h>

#include "pmsis.h"

static PI_L1 float ONE_f = 1;

static inline int swap_elems(int *vec, const int idx_a, const int idx_b)
{
    int tmp;

    tmp = vec[idx_a];
    vec[idx_a] = vec[idx_b];
    vec[idx_b] = tmp;

    return 0;
}

static inline int swap_rows(float *mat, const int row_a, const int row_b, const int dim_M, const int dim_N)
{
    for (int n = 0; n < dim_N; n++) {
        int idx_a, idx_b;
        float tmp;

        idx_a = row_a * dim_N + n;
        idx_b = row_b * dim_N + n;

        tmp = mat[idx_a];
        mat[idx_a] = mat[idx_b];
        mat[idx_b] = tmp;
    }

    return 0;
}

#ifdef  CLUSTER

/* pivot candidate of each core: largest |value| in the column among its rows, with value and row */
PI_L1 float local_max[NUM_CORES];
PI_L1 float local_val[NUM_CORES];
PI_L1 int local_row[NUM_CORES];

static inline int set_permutation_identity(int *vec, const int len)
{
    int num_ops;
    int rem_ops;
    int start;
    int block;
    int left;
    int end;
    int id;
    int op;
    int i;

    id = pi_core_id();
    block = len / NUM_CORES;
    left = len % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);
    num_ops = (end - start) / 2;
    rem_ops = (end - start) % 2;
    i = start;
    op = 0;

    if (num_ops != 0) {
        do {
            vec[i] = i;
            vec[i + 1] = (i + 1);

            i += 2;
            op++;
        } while (op < num_ops);
    }

    if (rem_ops)
        vec[end - 1] = (end - 1);

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

static int linalg_lu_decomp_pulp_open_cluster(float *mat, int *perm, const int dim_M, const int dim_N)
{
    float max_val;
    float val;
    int row_max;
    int dim_min;
    int start;
    int block;
    int left;
    int end;
    int id;

    set_permutation_identity(perm, dim_M);

    id = pi_core_id();
    dim_min = dim_M < dim_N ? dim_M : dim_N;

    /* Partial pivoting is distributed: every core keeps the first largest |value| of the
     * column among its own rows (here for column 0, then as a by-product of the elimination),
     * so each step reduces NUM_CORES candidates instead of scanning the column on core 0 */
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);
    max_val = -1;
    val = 0;
    row_max = -1;

    for (int m = start; m < end; m++) {
        float v, a;

        v = mat[m * dim_N];
        a = fabsf(v);
        if (a > max_val) {
            max_val = a;
            val = v;
            row_max = m;
        }
    }

    local_max[id] = max_val;
    local_val[id] = val;
    local_row[id] = row_max;

    for (int k = 0; k < dim_min; k++) {
        float p_inv;

#if NUM_CORES > 1
        pi_cl_team_barrier();
#endif

        /* same reduction on every core, in core order: the first largest |value| wins */
        max_val = -1;
        for (int i = 0; i < NUM_CORES; i++) {
            if (local_max[i] > max_val) {
                max_val = local_max[i];
                val = local_val[i];
                row_max = local_row[i];
            }
        }

        if (val == 0) {
#ifdef  ENABLE_LOGGING
            printf("ERROR | Zero pivot found at position %d during LU decomposition - Matrix is singular\n", k);
#endif  /* ENABLE_LOGGING */
            if (id == 0)
                pmsis_exit(-1);
        }
        p_inv = ONE_f / val;

        if (row_max != k) {
            /* rows k and row_max are swapped by columns, one slice per core */
            block = dim_N / NUM_CORES;
            left = dim_N % NUM_CORES;
            start = id * block + (id < left ? id : left);
            end = start + block + (id < left ? 1 : 0);

            for (int n = start; n < end; n++) {
                float tmp;

                tmp = mat[k * dim_N + n];
                mat[k * dim_N + n] = mat[row_max * dim_N + n];
                mat[row_max * dim_N + n] = tmp;
            }

            if (id == 0)
                swap_elems(perm, k, row_max);
        }

#if NUM_CORES > 1
        pi_cl_team_barrier();
#endif

        /* Gaussian Elimination; the updated rows are the pivot candidates of column k+1 */
        block = (dim_M - (k + 1)) / NUM_CORES;
        left = (dim_M - (k + 1)) % NUM_CORES;
        start = id * block + (id < left ? id : left) + (k + 1);
        end = start + block + (id < left ? 1 : 0);
        max_val = -1;
        val = 0;
        row_max = -1;

        for (int m = start; m < end; m++) {
            float factor;
            float v, a;

            factor = mat[m * dim_N + k] * p_inv;
            mat[m * dim_N + k] = factor;

            for (int n = k + 1; n < dim_N; n++)
                mat[m * dim_N + n] -= factor * mat[k * dim_N + n];

            if (k + 1 == dim_N)
                continue;
            v = mat[m * dim_N + (k + 1)];
            a = fabsf(v);
            if (a > max_val) {
                max_val = a;
                val = v;
                row_max = m;
            }
        }

        local_max[id] = max_val;
        local_val[id] = val;
        local_row[id] = row_max;
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int set_permutation_identity(int *vec, const int len)
{
    for (int i = 0; i < len; i++)
        vec[i] = i;

    return 0;
}

static int linalg_lu_decomp_pulp_open_fc(float *mat, int *perm, const int dim_M, const int dim_N)
{
    float factor;
    float pivot;
    int dim_min;
    int row_max;
    float max;
    float val;

    set_permutation_identity(perm, dim_M);

    dim_min = dim_M < dim_N ? dim_M : dim_N;
    for (int k = 0; k < dim_min; k++) {

        /* Partial Pivoting */
        row_max = k;
        max = fabs(mat[k * dim_N + k]);
        for (int m = k + 1; m < dim_M; m++) {
            val = fabs(mat[m * dim_N + k]);
            if (val > max) {
                row_max = m;
                max = val;
            }
        }

        if (row_max != k) {
            swap_rows(mat, k, row_max, dim_M, dim_N);
            swap_elems(perm, k, row_max);
        }

        /* Gaussian Elimination */
        pivot = mat[k * dim_N + k];
        if (pivot == 0) {
#ifdef  ENABLE_LOGGING
            printf("ERROR | Zero pivot found at position %d during LU decomposition - Matrix is singular\n", k);
#endif  /* ENABLE_LOGGING */
            pmsis_exit(-1);
        }

        for (int m = k + 1; m < dim_M; m++) {
            factor = mat[m * dim_N + k] / pivot;
            mat[m * dim_N + k] = factor;

            for (int n = k + 1; n < dim_N; n++)
                mat[m * dim_N + n] -= factor * mat[k * dim_N + n];
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_lu_decomp_impl(float *mat, int *perm, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_lu_decomp_pulp_open_cluster(mat, perm, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_lu_decomp_pulp_open_fc(mat, perm, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
