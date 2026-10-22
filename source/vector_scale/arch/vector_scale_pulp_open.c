#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

static int vector_scale_pulp_open_cluster(const float *src, const float val, float *dst, const int len)
{
    int id;
    int i;

    id = pi_core_id();

    for (i = 2 * id; (i + 1) < len; i += 2 * NUM_CORES) {
        int idx1, idx2;
        float src1, src2;

        idx1 = i;
        idx2 = i + 1;

        src1 = src[idx1];
        src2 = src[idx2];

        dst[idx1] = src1 * val;
        dst[idx2] = src2 * val;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        dst[len - 1] = src[len - 1] * val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int vector_scale_pulp_open_fc(const float *src, const float val, float *dst, const int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src[i] * val;

    return 0;
}

#endif  /* CLUSTER */

int vector_scale_impl(const float *src, const float val, float *dst, const int len)
{
    int ret;

#ifdef CLUSTER
    ret = vector_scale_pulp_open_cluster(src, val, dst, len);
#else   /* CLUSTER */
    ret = vector_scale_pulp_open_fc(src, val, dst, len);
#endif  /* CLUSTER */

    return ret;
}
