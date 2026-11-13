#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

PI_L1 float ZERO_f = 0.0f;
PI_L1 float local_dot[NUM_CORES];

#ifdef PLAY_PACKED

static int vector_dot_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    const play_fmt_v2_t *a2;
    const play_fmt_v2_t *b2;
    play_fmt_v2_t dot2_1;
    play_fmt_v2_t dot2_2;
    float dot_tmp;
    int tot_ops;
    int block;
    int start;
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

    dot2_1[0] = 0;
    dot2_1[1] = 0;
    dot2_2[0] = 0;
    dot2_2[1] = 0;
    dot_tmp = 0;

    i = start;
    if ((i & 1) && (i < end)) {
        dot_tmp += src_a[i] * src_b[i];
        i++;
    }

    a2 = (const play_fmt_v2_t *) &src_a[i];
    b2 = (const play_fmt_v2_t *) &src_b[i];
    tot_ops = (end - i) / 4;
    op = 0;

    if (tot_ops > 0) {
        do {
            play_fmt_v2_t a1, a3;
            play_fmt_v2_t b1, b3;

            a1 = a2[0];
            b1 = b2[0];
            a3 = a2[1];
            b3 = b2[1];

            dot2_1 += a1 * b1;
            dot2_2 += a3 * b3;

            a2 += 2;
            b2 += 2;
            op++;
        } while (op < tot_ops);
        i += 4 * tot_ops;
    }

    for (; i < end; i++)
        dot_tmp += src_a[i] * src_b[i];

    dot2_1 += dot2_2;
    local_dot[id] = dot_tmp + dot2_1[0] + dot2_1[1];

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

#else   /* PLAY_PACKED */

static int vector_dot_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    float dot_tmp1;
    float dot_tmp2;
    int rem_ops;
    int tot_ops;
    int block;
    int start;
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
    tot_ops = (end - start) / 2;
    rem_ops = tot_ops % 2;
    dot_tmp1 = 0;
    dot_tmp2 = 0;
    i = start;
    op = 0;

    do {
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
        i += 2;
        op++;
    } while (op < tot_ops);

    if (rem_ops)
        dot_tmp1 += src_a[end - 1] * src_b[end - 1];

    /* Only master core computes final dot prod */
    local_dot[id] = dot_tmp1 + dot_tmp2;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    if (id != 0)
        goto exit;

    dot_tmp1 = ZERO_f;
    for (i = 0; i < NUM_CORES; i++)
        dot_tmp1 += local_dot[i];
    *dst = dot_tmp1;

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

float ZERO_f = 0.0f;

static int vector_dot_pulp_open_fc(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    *dst = ZERO_f;
    for (int i = 0; i < len; i++)
        *dst += src_a[i] * src_b[i];

    return 0;
}

#endif  /* CLUSTER */

int vector_dot_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    int ret;

#ifdef CLUSTER
    ret = vector_dot_pulp_open_cluster(src_a, src_b, dst, len);
#else
    ret = vector_dot_pulp_open_fc(src_a, src_b, dst, len);
#endif

    return ret;
}
