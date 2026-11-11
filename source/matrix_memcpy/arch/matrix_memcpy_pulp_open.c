#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

#ifdef PLAY_PACKED

int matrix_memcpy_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    const play_fmt_v2_t *s2;
    play_fmt_v2_t *d2;
    int row_start;
    int row_end;
    int block;
    int left;
    int pairs;
    int id;
    int i, j;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    i = row_start * dim_N;
    if ((i & 1) && (i < row_end * dim_N)) {
        dst[i] = src[i];
        i++;
    }

    s2 = (const play_fmt_v2_t *) &src[i];
    d2 = (play_fmt_v2_t *) &dst[i];
    pairs = (row_end * dim_N - i) / 2;

    for (j = 0; (j + 1) < pairs; j += 2) {
        play_fmt_v2_t s1, s3;

        s1 = s2[j];
        s3 = s2[j + 1];

        d2[j] = s1;
        d2[j + 1] = s3;
    }
    if (j < pairs)
        d2[j] = s2[j];

    i += 2 * pairs;
    if (i < row_end * dim_N)
        dst[i] = src[i];

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

int matrix_memcpy_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int row_start;
    int row_end;
    int num_ops;
    int rem_ops;
    int block;
    int left;
    int m, n;
    int ops;
    int id;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);
    num_ops = dim_N / 2;
    rem_ops = dim_N % 2;

    m = row_start;
    do {
        n = 0;
        ops = 0;
        do {
            int idx1, idx2;
            play_fmt_t src1, src2;

            idx1 = m * dim_N + n;
            idx2 = m * dim_N + (n + 1);
            src1 = src[idx1];
            src2 = src[idx2];

            dst[idx1] = src1;
            dst[idx2] = src2;
            n += 2;
            ops++;
        } while (ops < num_ops);
        m++;
    } while (m < row_end);

    if (rem_ops) {
        m = row_start;
        do {
            dst[m * dim_N + (dim_N - 1)] = src[m * dim_N + (dim_N - 1)];
            m++;
        } while (m < row_end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_memcpy_pulp_open_fc(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    for (int m = 0; m < dim_M; m++)
        for (int n = 0; n < dim_N; n++)
            dst[m * dim_N + n] = src[m * dim_N + n];

    return 0;
}

#endif  /* CLUSTER */

int matrix_memcpy_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = matrix_memcpy_pulp_open_cluster(src, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = matrix_memcpy_pulp_open_fc(src, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
