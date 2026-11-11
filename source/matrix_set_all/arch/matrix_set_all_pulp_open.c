#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

#ifdef PLAY_PACKED

static int matrix_set_all_pulp_open_cluster(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N)
{
    play_fmt_v2_t *m2;
    play_fmt_v2_t val2;
    int row_start;
    int row_end;
    int block;
    int left;
    int pairs;
    int id;
    int i, j;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    val2[0] = val;
    val2[1] = val;

    i = row_start * dim_N;
    if ((i & 1) && (i < row_end * dim_N)) {
        mat[i] = val;
        i++;
    }

    m2 = (play_fmt_v2_t *) &mat[i];
    pairs = (row_end * dim_N - i) / 2;

    for (j = 0; (j + 1) < pairs; j += 2) {
        m2[j] = val2;
        m2[j + 1] = val2;
    }
    if (j < pairs)
        m2[j] = val2;

    i += 2 * pairs;
    if (i < row_end * dim_N)
        mat[i] = val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int matrix_set_all_pulp_open_cluster(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N)
{
    int row_start;
    int row_end;
    int num_ops;
    int rem_ops;
    int block;
    int left;
    int m, n;
    int ops;
    int id;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);
    num_ops = dim_N / 2;
    rem_ops = dim_N % 2;

    m = row_start;
    do {
        n = 0;
        ops = 0;
        do {
            mat[m * dim_N + n] = val;
            mat[m * dim_N + (n + 1)] = val;

            n += 2;
            ops++;
        } while (ops < num_ops);
        m++;
    } while (m < row_end);

    if (rem_ops) {
        m = row_start;
        do {
            mat[m * dim_N + (dim_N - 1)] = val;
            m++;
        } while (m < row_end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_set_all_pulp_open_fc(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N)
{
    for (int m = 0; m < dim_M; m++)
        for (int n = 0; n < dim_N; n++)
            mat[m * dim_N + n] = val;

    return 0;
}

#endif  /* CLUSTER */

int matrix_set_all_impl(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_set_all_pulp_open_cluster(mat, val, dim_M, dim_N);
#else
    ret = matrix_set_all_pulp_open_fc(mat, val, dim_M, dim_N);
#endif

    return ret;
}
