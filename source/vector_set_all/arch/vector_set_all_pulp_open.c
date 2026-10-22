#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int vector_set_all_pulp_open_cluster(float *vec, const float val, const int len)
{
    int id;
    int i;

    id = pi_core_id();

    for (i = 2 * id; (i + 1) < len; i += 2 * NUM_CORES) {
        int idx1, idx2;

        idx1 = i;
        idx2 = i + 1;

        vec[idx1] = val;
        vec[idx2] = val;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        vec[len - 1] = val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int vector_set_all_pulp_open_fc(float *vec, const float val, const int len)
{
    for (int i = 0; i < len; i++)
        vec[i] = val;

    return 0;
}

#endif  /* CLUSTER */

int vector_set_all_impl(float *vec, const float val, const int len)
{
    int ret;

#if CLUSTER
    ret = vector_set_all_pulp_open_cluster(vec, val, len);
#else
    ret = vector_set_all_pulp_open_fc(vec, val, len);
#endif

    return ret;
}
