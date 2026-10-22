#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

PI_L1 float local_min[NUM_CORES];

static int vector_min_pulp_open_cluster(const float *src, float *min, const int len)
{
    float min_tmp1;
    float min_tmp2;
    int id;
    int i;

    id = pi_core_id();
    min_tmp1 = src[0];
    min_tmp2 = src[0];

    for (i = 2 * id; (i + 1) < len; i += 2 * NUM_CORES) {
        int idx1, idx2;
        float src1, src2;

        idx1 = i;
        idx2 = i + 1;

        src1 = src[idx1];
        src2 = src[idx2];

        min_tmp1 = src1 < min_tmp1 ? src1 : min_tmp1;
        min_tmp2 = src2 < min_tmp2 ? src2 : min_tmp2;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        min_tmp1 = src[len - 1] < min_tmp1 ? src[len - 1] : min_tmp1;

    local_min[id] = min_tmp1 < min_tmp2 ? min_tmp1 : min_tmp2;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    if (id != 0) {
        /* Only master core finds absolute minimum */
        goto exit;
    }

    *min = local_min[0];
    for (int i = 1; i < NUM_CORES; i++) {
        if (local_min[i] < *min)
            *min = local_min[i];
    }

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int vector_min_pulp_open_fc(const float *src, float *min, const int len)
{
    *min = src[0];
    for (int i = 1; i < len; i++) {
        if (src[i] < *min)
            *min = src[i];
    }

    return 0;
}

#endif  /* CLUSTER */

int vector_min_impl(const float *src, float *min, const int len)
{
    int ret;

#ifdef  CLUSTER
    ret = vector_min_pulp_open_cluster(src, min, len);
#else   /* CLUSTER */
    ret = vector_min_pulp_open_fc(src, min, len);
#endif  /* CLUSTER */

    return ret;
}
