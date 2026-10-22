#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#if CLUSTER

static int vector_sub_pulp_open_cluster(const float *src_a, const float *src_b, float *dst, const int len)
{
    int id;
    int i;

    id = pi_core_id();

    for (i = 2 * id; (i + 1) < len; i += 2 * NUM_CORES) {
        int idx1, idx2;
        float a1, a2;
        float b1, b2;

        idx1 = i;
        idx2 = i + 1;
        a1 = src_a[idx1];
        b1 = src_b[idx1];
        a2 = src_a[idx2];
        b2 = src_b[idx2];

        dst[idx1] = a1 - b1;
        dst[idx2] = a2 - b2;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        dst[len - 1] = src_a[len - 1] - src_b[len - 1];

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int vector_sub_pulp_open_fc(const float *src_a, const float *src_b, float *dst, const int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src_a[i] - src_b[i];

    return 0;
}

#endif  /* CLUSTER */

int vector_sub_impl(const float *src_a, const float *src_b, float *dst, const int len)
{
    int ret;

#if CLUSTER
    ret = vector_sub_pulp_open_cluster(src_a, src_b, dst, len);
#else
    ret = vector_sub_pulp_open_fc(src_a, src_b, dst, len);
#endif

    return ret;
}
