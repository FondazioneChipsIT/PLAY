#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

PI_L1 play_fmt_t local_min[NUM_CORES];

static int vector_min_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *min, const int len)
{
    play_fmt_t min_tmp;
    int block;
    int start;
    int left;
    int end;
    int id;
    int i;

    id = pi_core_id();
    block = len / NUM_CORES;
    left = len % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);
    min_tmp = src[start];

    for (i = (start + 1); (i + 3) < end; i += 4) {
        int idx1, idx2, idx3, idx4;
        play_fmt_t src1, src2, src3, src4;

        idx1 = i;
        idx2 = i + 1;
        idx3 = i + 2;
        idx4 = i + 3;

        src1 = src[idx1];
        src2 = src[idx2];
        src3 = src[idx3];
        src4 = src[idx4];

        min_tmp = src1 < min_tmp ? src1 : min_tmp;
        min_tmp = src2 < min_tmp ? src2 : min_tmp;
        min_tmp = src3 < min_tmp ? src3 : min_tmp;
        min_tmp = src4 < min_tmp ? src4 : min_tmp;
    }

    for(; i < end; i++) {
        if (src[i] < min_tmp)
            min_tmp = src[i];
    }

    local_min[id] = min_tmp;

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

static int vector_min_pulp_open_fc(const play_fmt_t *src, play_fmt_t *min, const int len)
{
    *min = src[0];
    for (int i = 1; i < len; i++) {
        if (src[i] < *min)
            *min = src[i];
    }

    return 0;
}

#endif  /* CLUSTER */

int vector_min_impl(const play_fmt_t *src, play_fmt_t *min, const int len)
{
    int ret;

#ifdef  CLUSTER
    ret = vector_min_pulp_open_cluster(src, min, len);
#else   /* CLUSTER */
    ret = vector_min_pulp_open_fc(src, min, len);
#endif  /* CLUSTER */

    return ret;
}
