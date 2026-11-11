#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

#ifdef PLAY_PACKED

static int matrix_trans_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int col_pairs;
    int pair_start;
    int pair_end;
    int block;
    int left;
    int id;
    int m, n, j;

    id = pi_core_id();
    col_pairs = dim_N / 2;
    block = col_pairs / NUM_CORES;
    left = col_pairs % NUM_CORES;
    pair_start = id * block + (id < left ? id : left);
    pair_end = pair_start + block + (id < left ? 1 : 0);

    for (j = pair_start; j < pair_end; j++) {
        const play_fmt_t *s0;
        const play_fmt_t *s1;
        play_fmt_t *d0;
        play_fmt_t *d1;

        n = 2 * j;
        s0 = &src[n];                   /* src[m][n], src[m][n + 1] */
        s1 = &src[dim_N + n];           /* src[m + 1][n], src[m + 1][n + 1] */
        d0 = &dst[n * dim_M];           /* dst[n][m] */
        d1 = &dst[(n + 1) * dim_M];     /* dst[n + 1][m] */
        for (m = 0; (m + 1) < dim_M; m += 2) {
            play_fmt_v2_t r0, r1;

            r0 = *(const play_fmt_v2_t *) s0;
            r1 = *(const play_fmt_v2_t *) s1;
            s0 += 2 * dim_N;
            s1 += 2 * dim_N;

            *(play_fmt_v2_t *) &d0[m] = __builtin_shufflevector(r0, r1, 0, 2);
            *(play_fmt_v2_t *) &d1[m] = __builtin_shufflevector(r0, r1, 1, 3);
        }
        if (m < dim_M) {
            d0[m] = s0[0];
            d1[m] = s0[1];
        }
    }

    if ((dim_N % 2) && (id == col_pairs % NUM_CORES)) {
        n = dim_N - 1;
        for (m = 0; m < dim_M; m++)
            dst[n * dim_M + m] = src[m * dim_N + n];
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int matrix_trans_pulp_open_cluster(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int col_start;
    int col_end;
    int num_ops;
    int rem_ops;
    int block;
    int left;
    int m, n;
    int ops;
    int id;

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    col_start = id * block + (id < left ? id : left);
    col_end = col_start + block + (id < left ? 1 : 0);
    num_ops = dim_M / 2;
    rem_ops = dim_M % 2;

    n = col_start;
    do {
        m = 0;
        ops = 0;
        do {
            play_fmt_t src1, src2;

            src1 = src[m * dim_N + n];
            src2 = src[(m + 1) * dim_N + n];
            dst[n * dim_M + m] = src1;
            dst[n * dim_M + (m + 1)] = src2;

            m += 2;
            ops ++;
        } while (ops < num_ops);
        n++;
    } while (n < col_end);

    if (rem_ops) {
        n = col_start;
        do {
            dst[n * dim_M + (dim_M - 1)] = src[(dim_M - 1) * dim_N + n];
            m++;
        } while (n < col_end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_trans_pulp_open_fc(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    for (int m = 0; m < dim_M; m++)
        for (int n = 0; n < dim_N; n++)
            dst[n * dim_M + m] = src[m * dim_N + n];

    return 0;
}

#endif  /* CLUSTER */

int matrix_trans_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = matrix_trans_pulp_open_cluster(src, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = matrix_trans_pulp_open_fc(src, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
