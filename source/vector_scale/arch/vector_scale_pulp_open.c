#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

#ifdef PLAY_PACKED

static int vector_scale_pulp_open_cluster(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len)
{
    const play_fmt_v2_t *s2;
    play_fmt_v2_t *d2;
    play_fmt_v2_t val2;
    int pairs;
    int full;
    int id;
    int i;

    id = pi_core_id();

    val2[0] = val;
    val2[1] = val;

    s2 = (const play_fmt_v2_t *) src;
    d2 = (play_fmt_v2_t *) dst;
    pairs = len / 2;
    full = pairs - pairs % (2 * NUM_CORES);

    for (i = 2 * id; i < full; i += 2 * NUM_CORES) {
        play_fmt_v2_t s1, s3;

        s1 = s2[i];
        s3 = s2[i + 1];

        d2[i] = s1 * val2;
        d2[i + 1] = s3 * val2;
    }

    i = full + id;
    if (i < pairs)
        d2[i] = s2[i] * val2;

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        dst[len - 1] = src[len - 1] * val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int vector_scale_pulp_open_cluster(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len)
{
    int tot_ops;
    int rem_ops;
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
    i = start;
    op = 0;

    do {
        int idx1, idx2;
        play_fmt_t src1, src2;

        idx1 = i;
        idx2 = i + 1;

        src1 = src[idx1];
        src2 = src[idx2];

        dst[idx1] = src1 * val;
        dst[idx2] = src2 * val;

        i += 2;
        op++;
    } while (op < tot_ops);

    if (rem_ops)
        dst[end - 1] = src[end - 1] * val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int vector_scale_pulp_open_fc(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src[i] * val;

    return 0;
}

#endif  /* CLUSTER */

int vector_scale_impl(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len)
{
    int ret;

#ifdef CLUSTER
    ret = vector_scale_pulp_open_cluster(src, val, dst, len);
#else   /* CLUSTER */
    ret = vector_scale_pulp_open_fc(src, val, dst, len);
#endif  /* CLUSTER */

    return ret;
}
