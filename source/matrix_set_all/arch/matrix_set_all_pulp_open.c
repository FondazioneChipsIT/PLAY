#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int matrix_set_all_pulp_open_cluster(float *mat, const float val, const int dim_M, const int dim_N)
{
    int len;
    int id;
    int i;

    id = pi_core_id();
    len = dim_M * dim_N;

    for (i = 2 * id; (i + 1) < len; i += 2 * NUM_CORES) {
        int idx1, idx2;

        idx1 = i;
        idx2 = i + 1;

        mat[idx1] = val;
        mat[idx2] = val;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        mat[len - 1] = val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int matrix_set_all_pulp_open_fc(float *mat, const float val, const int dim_M, const int dim_N)
{
    for (int m = 0; m < dim_M; m++)
        for (int n = 0; n < dim_N; n++)
            mat[m * dim_N + n] = val;

    return 0;
}

#endif  /* CLUSTER */

int matrix_set_all_impl(float *mat, const float val, const int dim_M, const int dim_N)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_set_all_pulp_open_cluster(mat, val, dim_M, dim_N);
#else
    ret = matrix_set_all_pulp_open_fc(mat, val, dim_M, dim_N);
#endif

    return ret;
}
