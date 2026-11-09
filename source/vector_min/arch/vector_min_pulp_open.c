#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef CLUSTER

PI_L1 play_fmt_t local_min[NUM_CORES];

#ifdef PLAY_PACKED

#define MIN2(a, b)  __builtin_elementwise_minnum((a), (b))

static int vector_min_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *min, const int len)
{
    const play_fmt_v2_t *src2;
    play_fmt_v2_t min2;
    play_fmt_t min_tmp;
    int pairs;
    int full;
    int id;
    int i;

    id = pi_core_id();

    min_tmp = src[0];
    min2[0] = min_tmp;
    min2[1] = min_tmp;

    src2 = (const play_fmt_v2_t *) src;
    pairs = len / 2;
    full = pairs - pairs % (2 * NUM_CORES);

    for (i = 2 * id; i < full; i += 2 * NUM_CORES) {
        play_fmt_v2_t src_a, src_b;

        src_a = src2[i];
        src_b = src2[i + 1];

        min2 = MIN2(min2, src_a);
        min2 = MIN2(min2, src_b);
    }

    i = full + id;
    if (i < pairs)
        min2 = MIN2(min2, src2[i]);

    min_tmp = min2[0] < min2[1] ? min2[0] : min2[1];

    if ((len % 2) && (id == (len / 2) % NUM_CORES) && src[len - 1] < min_tmp)
        min_tmp = src[len - 1];

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

#else   /* PLAY_PACKED */

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

#endif  /* PLAY_PACKED */

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
