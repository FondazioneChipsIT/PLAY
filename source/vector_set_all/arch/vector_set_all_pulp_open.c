#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int vector_set_all_pulp_open_cluster(play_fmt_t *vec, const play_fmt_t val, const int len)
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

        idx1 = i;
        idx2 = i + 1;

        vec[idx1] = val;
        vec[idx2] = val;

        i += 2;
        op++;
    } while (op < tot_ops);

    if (rem_ops)
        vec[end - 1] = val;

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int vector_set_all_pulp_open_fc(play_fmt_t *vec, const play_fmt_t val, const int len)
{
    for (int i = 0; i < len; i++)
        vec[i] = val;

    return 0;
}

#endif  /* CLUSTER */

int vector_set_all_impl(play_fmt_t *vec, const play_fmt_t val, const int len)
{
    int ret;

#if CLUSTER
    ret = vector_set_all_pulp_open_cluster(vec, val, len);
#else
    ret = vector_set_all_pulp_open_fc(vec, val, len);
#endif

    return ret;
}
