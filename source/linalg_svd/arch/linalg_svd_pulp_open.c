#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

PI_L1 play_fmt_t *tmp;

#ifdef  CLUSTER

static int linalg_svd_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N)
{
    if (pi_core_id() == 0)
        tmp = pmsis_l1_malloc(dim_N * dim_N * sizeof(play_fmt_t));

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    matrix_mul_trans_A(src, src, tmp, dim_M, dim_N, dim_N);
    linalg_svd_jacobi(tmp, mat_V, vec_S, dim_N);
    linalg_svd_lsv(src, mat_V, vec_S, dst, dim_M, dim_N);

    if (pi_core_id() == 0)
        pmsis_l1_malloc_free(tmp, dim_N * dim_N * sizeof(play_fmt_t));

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */


static int linalg_svd_pulp_open_fc(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N)
{
    tmp = pmsis_l1_malloc(dim_N * dim_N * sizeof(play_fmt_t));
    matrix_mul_trans_A(src, src, tmp, dim_M, dim_N, dim_N);
    linalg_svd_jacobi(tmp, mat_V, vec_S, dim_N);
    linalg_svd_lsv(src, mat_V, vec_S, dst, dim_M, dim_N);
    pmsis_l1_malloc_free(tmp, dim_N * dim_N * sizeof(play_fmt_t));
    return 0;
}

#endif  /* CLUSTER */

int linalg_svd_impl(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_svd_pulp_open_cluster(src, dst, mat_V, vec_S, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_svd_pulp_open_fc(src, dst, mat_V, vec_S, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
