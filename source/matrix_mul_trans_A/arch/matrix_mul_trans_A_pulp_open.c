#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

#ifdef PLAY_PACKED

static int matrix_mul_trans_A_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_v2_t a2_1;
    play_fmt_v2_t a2_2;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int start;
    int block;
    int left;
    int end;
    int id;
    int m, n, p;

    id = pi_core_id();
    block = dim_P / NUM_CORES;
    left = dim_P % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    p = start;
    if ((p & 1) && (p < end)) {
        for (n = 0; n < dim_N; n++) {
            sum1 = 0;
            for (m = 0; m < dim_M; m++)
                sum1 += src_a[m * dim_N + n] * src_b[m * dim_P + p];
            dst[n * dim_P + p] = sum1;
        }
        p++;
    }

    for (; (p + 1) < end; p += 2) {
        for (n = 0; n < dim_N; n++) {
            const play_fmt_t *a;
            const play_fmt_t *b;

            a = &src_a[n];                  /* src_a[m][n] */
            b = &src_b[p];                  /* src_b[m][p], src_b[m][p + 1] */
            sum2_1[0] = 0;
            sum2_1[1] = 0;
            sum2_2[0] = 0;
            sum2_2[1] = 0;
            for (m = 0; (m + 1) < dim_M; m += 2) {
                a2_1[0] = a[0];
                a2_1[1] = a2_1[0];
                a2_2[0] = a[dim_N];
                a2_2[1] = a2_2[0];
                a += 2 * dim_N;

                sum2_1 += a2_1 * *(const play_fmt_v2_t *) &b[0];
                sum2_2 += a2_2 * *(const play_fmt_v2_t *) &b[dim_P];
                b += 2 * dim_P;
            }
            if (m < dim_M) {
                a2_1[0] = a[0];
                a2_1[1] = a2_1[0];
                sum2_1 += a2_1 * *(const play_fmt_v2_t *) &b[0];
            }
            *(play_fmt_v2_t *) &dst[n * dim_P + p] = sum2_1 + sum2_2;
        }
    }

    if (p < end) {
        for (n = 0; n < dim_N; n++) {
            sum1 = 0;
            sum2 = 0;
            for (m = 0; (m + 1) < dim_M; m += 2) {
                sum1 += src_a[m * dim_N + n] * src_b[m * dim_P + p];
                sum2 += src_a[(m + 1) * dim_N + n] * src_b[(m + 1) * dim_P + p];
            }
            if (m < dim_M)
                sum1 += src_a[m * dim_N + n] * src_b[m * dim_P + p];
            dst[n * dim_P + p] = sum1 + sum2;
        }
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static int matrix_mul_trans_A_pulp_open_cluster(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    int m, n, p;
    int num_ops;
    int rem_ops;
    float sum1;
    float sum2;
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
                float src_a1, src_a2;
                float src_b1, src_b2;

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
                float a;
                float b;

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

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

static int matrix_mul_trans_A_pulp_open_fc(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    float sum;

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
