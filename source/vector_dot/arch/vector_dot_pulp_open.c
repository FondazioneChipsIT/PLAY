#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

PI_L1 float ZERO_f = 0.0f;
PI_L1 float local_dot[NUM_CORES];

static int vector_dot_pulp_open_cluster(const float *src_a, const float *src_b, float *dst, const int len)
{
    float dot_tmp1;
    float dot_tmp2;
    int id;
    int i;

    id = pi_core_id();
    dot_tmp1 = 0;
    dot_tmp2 = 0;

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

        dot_tmp1 += a1 * b1;
        dot_tmp2 += a2 * b2;
    }

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        dot_tmp1 += src_a[len - 1] * src_b[len - 1];

    /* Only master core computes final dot prod */
    local_dot[id] = dot_tmp1 + dot_tmp2;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    if (id != 0)
        goto exit;

    *dst = ZERO_f;
    for (i = 0; i < NUM_CORES; i++)
        *dst += local_dot[i];

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

float ZERO_f = 0.0f;

static int vector_dot_pulp_open_fc(const float *src_a, const float *src_b, float *dst, const int len)
{
    *dst = ZERO_f;
    for (int i = 0; i < len; i++)
        *dst += src_a[i] * src_b[i];

    return 0;
}

#endif  /* CLUSTER */

int vector_dot_impl(const float *src_a, const float *src_b, float *dst, const int len)
{
    int ret;

#ifdef CLUSTER
    ret = vector_dot_pulp_open_cluster(src_a, src_b, dst, len);
#else
    ret = vector_dot_pulp_open_fc(src_a, src_b, dst, len);
#endif

    return ret;
}
