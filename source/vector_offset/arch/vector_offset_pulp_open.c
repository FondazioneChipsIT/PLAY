#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

#ifdef PLAY_PACKED

static int vector_offset_pulp_open_cluster(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len)
{
    const play_fmt_v2_t *s2;
    play_fmt_v2_t *d2;
    play_fmt_v2_t offset2;
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

    offset2[0] = offset;
    offset2[1] = offset;

    i = start;
    if ((i & 1) && (i < end)) {
        dst[i] = src[i] + offset;
        i++;
    }

    s2 = (const play_fmt_v2_t *) &src[i];
    d2 = (play_fmt_v2_t *) &dst[i];
    tot_ops = (end - i) / 4;
    op = 0;

    if (tot_ops > 0) {
        do {
            play_fmt_v2_t x1, x2;

            x1 = s2[0];
            x2 = s2[1];

            d2[0] = x1 + offset2;
            d2[1] = x2 + offset2;

            s2 += 2;
            d2 += 2;
            op++;
        } while (op < tot_ops);
        i += 4 * tot_ops;
    }

    for (; i < end; i++)
        dst[i] = src[i] + offset;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int vector_offset_pulp_open_cluster(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len)
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

        dst[idx1] = src1 + offset;
        dst[idx2] = src2 + offset;

        i += 2;
        op++;
    } while (op < tot_ops);

    if (rem_ops)
        dst[end - 1] = src[end - 1] + offset;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int vector_offset_pulp_open_fc(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src[i] + offset;

    return 0;
}

#endif  /* CLUSTER */

int vector_offset_impl(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len)
{
    int ret;

#ifdef CLUSTER
    ret = vector_offset_pulp_open_cluster(src, offset, dst, len);
#else /* CLUSTER */
    ret = vector_offset_pulp_open_fc(src, offset, dst, len);
#endif /* CLUSTER */

    return ret;
}
