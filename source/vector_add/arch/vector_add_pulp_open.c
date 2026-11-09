#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#if CLUSTER

#ifdef PLAY_PACKED

static int vector_add_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    const play_fmt_v2_t *a2;
    const play_fmt_v2_t *b2;
    play_fmt_v2_t *d2;
    int pairs;
    int full;
    int id;
    int i;

    id = pi_core_id();

    a2 = (const play_fmt_v2_t *) src_a;
    b2 = (const play_fmt_v2_t *) src_b;
    d2 = (play_fmt_v2_t *) dst;
    pairs = len / 2;
    full = pairs - pairs % (2 * NUM_CORES);

    for (i = 2 * id; i < full; i += 2 * NUM_CORES) {
        play_fmt_v2_t a1, a3;
        play_fmt_v2_t b1, b3;

        a1 = a2[i];
        b1 = b2[i];
        a3 = a2[i + 1];
        b3 = b2[i + 1];

        d2[i] = a1 + b1;
        d2[i + 1] = a3 + b3;
    }

    i = full + id;
    if (i < pairs)
        d2[i] = a2[i] + b2[i];

    if ((len % 2) && (id == (len / 2) % NUM_CORES))
        dst[len - 1] = src_a[len - 1] + src_b[len - 1];

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int vector_add_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    int rem_ops;
    int tot_ops;
    int block;
    int start;
    int left;
    int end;
    int op;
    int id;
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
        play_fmt_t a1, a2;
        play_fmt_t b1, b2;

        idx1 = i;
        idx2 = i + 1;
        a1 = src_a[idx1];
        b1 = src_b[idx1];
        a2 = src_a[idx2];
        b2 = src_b[idx2];

        dst[idx1] = a1 + b1;
        dst[idx2] = a2 + b2;

        i += 2;
        op++;
    } while (op < tot_ops);

    if (rem_ops)
        dst[end - 1] = src_a[end - 1] + src_b[end - 1];

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int vector_add_pulp_open_fc(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src_a[i] + src_b[i];

    return 0;
}

#endif  /* CLUSTER */

int vector_add_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    int ret;

#if CLUSTER
    ret = vector_add_pulp_open_cluster(src_a, src_b, dst, len);
#else
    ret = vector_add_pulp_open_fc(src_a, src_b, dst, len);
#endif

    return ret;
}
