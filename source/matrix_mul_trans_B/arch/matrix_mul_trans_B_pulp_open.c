#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

#ifdef PLAY_PACKED

static int matrix_mul_trans_B_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    const play_fmt_v2_t *a2;
    const play_fmt_v2_t *b2;
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_t sum;
    int row_start;
    int row_end;
    int pairs;
    int block;
    int left;
    int id;
    int m, p, j;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);
    pairs = dim_N / 2;

    for (p = row_start; p < row_end; p++) {
        for (m = 0; m < dim_M; m++) {
            a2 = (const play_fmt_v2_t *) &src_a[m * dim_N];
            b2 = (const play_fmt_v2_t *) &src_b[p * dim_N];
            sum2_1[0] = 0;
            sum2_1[1] = 0;
            sum2_2[0] = 0;
            sum2_2[1] = 0;
            for (j = 0; (j + 1) < pairs; j += 2) {
                sum2_1 += a2[j] * b2[j];
                sum2_2 += a2[j + 1] * b2[j + 1];
            }
            if (j < pairs)
                sum2_1 += a2[j] * b2[j];

            sum2_1 += sum2_2;
            sum = sum2_1[0] + sum2_1[1];
            if (dim_N % 2)
                sum += src_a[m * dim_N + (dim_N - 1)] * src_b[p * dim_N + (dim_N - 1)];
            dst[m * dim_P + p] = sum;
        }
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int matrix_mul_trans_B_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int row_start;
    int row_end;
    int m, n, p;
    int num_ops;
    int rem_ops;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int block;
    int left;
    int id;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    rem_ops = dim_N % 2;
    p = row_start;
    do {
        m = 0;
        do {
            num_ops = dim_N / 2;
            sum1 = 0;
            sum2 = 0;
            n = 0;
            do {
                play_fmt_t src_a1, src_a2;
                play_fmt_t src_b1, src_b2;

                src_a1 = src_a[m * dim_N + n];
                src_a2 = src_a[m * dim_N + (n + 1)];
                src_b1 = src_b[p * dim_N + n];
                src_b2 = src_b[p * dim_N + (n + 1)];

                sum1 += src_a1 * src_b1;
                sum2 += src_a2 * src_b2;

                n += 2;
                num_ops--;
            } while (num_ops > 0);

            dst[m * dim_P + p] = sum1 + sum2;
            m++;
        } while (m < dim_M);

        p++;
    } while (p < row_end);

    if (rem_ops) {
        p = row_start;
        do {
            p = 0;
            do {
                play_fmt_t a;
                play_fmt_t b;

                a = src_a[m * dim_N + (dim_N - 1)];
                b = src_b[p * dim_N + (dim_N - 1)];

                dst[m * dim_P + p] += a * b;
                m++;
            } while (m < dim_M);
            p++;
        } while (p < row_end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_mul_trans_B_pulp_open_fc(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    play_fmt_t sum;

    for (int m = 0; m < dim_M; m++) {
        for (int p = 0; p < dim_P; p++) {
            sum = 0;
            for (int n = 0; n < dim_N; n++)
                sum += src_a[m * dim_N + n] * src_b[p * dim_N + n];
            dst[m * dim_P + p] = sum;
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int matrix_mul_trans_B_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_mul_trans_B_pulp_open_cluster(src_a, src_b, dst, dim_M, dim_N, dim_P);
#else
    ret = matrix_mul_trans_B_pulp_open_fc(src_a, src_b, dst, dim_M, dim_N, dim_P);
#endif

    return ret;
}
