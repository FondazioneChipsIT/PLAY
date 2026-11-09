#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

static int matrix_mul_trans_A_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int m, n, p;
    int num_ops;
    int rem_ops;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int start;
    int block;
    int left;
    int end;
    int id;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);
    rem_ops = dim_M % 2;

    p = start;
    do {
        n = 0;
        do {
            num_ops = dim_M / 2;
            sum1 = 0;
            sum2 = 0;
            m = 0;
            do {
                play_fmt_t src_a1, src_a2;
                play_fmt_t src_b1, src_b2;

                src_a1 = src_a[m * dim_N + n];
                src_a2 = src_a[(m + 1) * dim_N + n];
                src_b1 = src_b[m * dim_P + p];
                src_b2 = src_b[(m + 1) * dim_P + p];

                sum1 += src_a1 * src_b1;
                sum2 += src_a2 * src_b2;

                m += 2;
                num_ops--;
            } while (num_ops > 0);

            dst[n * dim_P + p] = sum1 + sum2;
            n++;
        } while (n < dim_N);

        p++;
    } while (p < end);

    if (rem_ops) {
        p = start;
        do {
            n = 0;
            do {
                play_fmt_t a;
                play_fmt_t b;

                a = src_a[(dim_M - 1) * dim_N + n];
                b = src_b[(dim_M - 1) * dim_P + p];

                dst[n * dim_P + p] = a * b;
                n++;
            } while (n < dim_N);
            p++;
        } while (p < end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

static int matrix_mul_trans_A_pulp_open_fc(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    play_fmt_t sum;

    for (int n = 0; n < dim_N; n++) {
        for (int p = 0; p < dim_P; p++) {
            sum = 0;
            for (int m = 0; m < dim_M; m++)
                sum += src_a[m * dim_N + n] * src_b[m * dim_P + p];
            dst[n * dim_P + p] = sum;
        }
    }

    return 0;
}

#endif  /* CLUSTER */

int matrix_mul_trans_A_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int ret;

#ifdef CLUSTER
    ret = matrix_mul_trans_A_pulp_open_cluster(src_a, src_b, dst, dim_M, dim_N, dim_P);
#else
    ret = matrix_mul_trans_A_pulp_open_fc(src_a, src_b, dst, dim_M, dim_N, dim_P);
#endif

    return ret;
}
