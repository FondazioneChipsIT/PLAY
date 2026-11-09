#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

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
