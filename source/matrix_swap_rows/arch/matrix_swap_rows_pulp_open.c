#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

#ifdef PLAY_PACKED

static int matrix_swap_rows_pulp_open_cluster(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N)
{
    play_fmt_v2_t *a2;
    play_fmt_v2_t *b2;
    play_fmt_t tmp;
    int start;
    int block;
    int left;
    int pairs;
    int end;
    int id;
    int n;

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    n = start;
    if (((row_a * dim_N + n) & 1) && (n < end)) {
        tmp = mat[row_a * dim_N + n];
        mat[row_a * dim_N + n] = mat[row_b * dim_N + n];
        mat[row_b * dim_N + n] = tmp;
        n++;
    }

    a2 = (play_fmt_v2_t *) &mat[row_a * dim_N + n];
    b2 = (play_fmt_v2_t *) &mat[row_b * dim_N + n];
    pairs = (end - n) / 2;

    for (int j = 0; j < pairs; j++) {
        play_fmt_v2_t tmp2;

        tmp2 = a2[j];
        a2[j] = b2[j];
        b2[j] = tmp2;
    }

    n += 2 * pairs;
    if (n < end) {
        tmp = mat[row_a * dim_N + n];
        mat[row_a * dim_N + n] = mat[row_b * dim_N + n];
        mat[row_b * dim_N + n] = tmp;
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int matrix_swap_rows_pulp_open_cluster(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N)
{
    play_fmt_t tmp;
    int start;
    int block;
    int left;
    int end;
    int id;

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    for (int n = start; n < end; n++) {
        tmp = mat[row_a * dim_N + n];
        mat[row_a * dim_N + n] = mat[row_b * dim_N + n];
        mat[row_b * dim_N + n] = tmp;
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_swap_rows_pulp_open_fc(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N)
{
    play_fmt_t tmp;

    for (int n = 0; n < dim_N; n++) {
        tmp = mat[row_a * dim_N + n];
        mat[row_a * dim_N + n] = mat[row_b * dim_N + n];
        mat[row_b * dim_N + n] = tmp;
    }

    return 0;
}

#endif  /* CLUSTER */

int matrix_swap_rows_impl(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_swap_rows_pulp_open_cluster(mat, row_a, row_b, dim_N);
#else
    ret = matrix_swap_rows_pulp_open_fc(mat, row_a, row_b, dim_N);
#endif

    return ret;
}
