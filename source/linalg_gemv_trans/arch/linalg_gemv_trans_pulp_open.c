#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

PI_L1 play_fmt_t ZERO_f = 0.0f;
PI_L1 play_fmt_t ONE_f = 1.0f;

#ifdef PLAY_PACKED

static inline void gemv_trans_col(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int n, const int dim_M, const int dim_N)
{
    play_fmt_t sum1;
    play_fmt_t sum2;
    int m;

    sum1 = 0;
    sum2 = 0;
    for (m = 0; (m + 1) < dim_M; m += 2) {
        sum1 += mat[m * dim_N + n] * vec_x[m];
        sum2 += mat[(m + 1) * dim_N + n] * vec_x[m + 1];
    }
    if (m < dim_M)
        sum1 += mat[m * dim_N + n] * vec_x[m];
    dst[n] = alpha * (sum1 + sum2) + beta * vec_y[n];
}

static int linalg_gemv_trans_pulp_open_cluster(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    play_fmt_v2_t sum2_1;
    play_fmt_v2_t sum2_2;
    play_fmt_v2_t x2_1;
    play_fmt_v2_t x2_2;
    play_fmt_v2_t alpha2;
    play_fmt_v2_t beta2;
    int row_start;
    int row_end;
    int block;
    int left;
    int id;
    int m, n;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_M);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_M);
        else
            vector_scale(vec_y, beta, dst, dim_M);
        return 0;
    }

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    alpha2[0] = alpha;
    alpha2[1] = alpha;
    beta2[0] = beta;
    beta2[1] = beta;

    n = row_start;
    if ((n & 1) && (n < row_end)) {
        gemv_trans_col(mat, vec_x, vec_y, alpha, beta, dst, n, dim_M, dim_N);
        n++;
    }

    for (; (n + 1) < row_end; n += 2) {
        const play_fmt_t *a;

        a = &mat[n];
        sum2_1[0] = 0;
        sum2_1[1] = 0;
        sum2_2[0] = 0;
        sum2_2[1] = 0;
        for (m = 0; (m + 1) < dim_M; m += 2) {
            x2_1[0] = vec_x[m];
            x2_1[1] = x2_1[0];
            x2_2[0] = vec_x[m + 1];
            x2_2[1] = x2_2[0];

            sum2_1 += x2_1 * *(const play_fmt_v2_t *) &a[0];
            sum2_2 += x2_2 * *(const play_fmt_v2_t *) &a[dim_N];
            a += 2 * dim_N;
        }
        if (m < dim_M) {
            x2_1[0] = vec_x[m];
            x2_1[1] = x2_1[0];
            sum2_1 += x2_1 * *(const play_fmt_v2_t *) &a[0];
        }
        *(play_fmt_v2_t *) &dst[n] = alpha2 * (sum2_1 + sum2_2) + beta2 * *(const play_fmt_v2_t *) &vec_y[n];
    }

    if (n < row_end)
        gemv_trans_col(mat, vec_x, vec_y, alpha, beta, dst, n, dim_M, dim_N);

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* PLAY_PACKED */

static inline void mat_vec_mul(const play_fmt_t *mat, const play_fmt_t *vec_x, const int n, const int dim_M, const int dim_N, play_fmt_t *sum1, play_fmt_t *sum2)
{
    int num_ops;
    int ops;
    int m;

    m = 0;
    ops = 0;
    *sum1 = 0;
    *sum2 = 0;
    num_ops = dim_M / 2;
    do {
        play_fmt_t mat1, mat2;
        play_fmt_t vec1, vec2;

        mat1 = mat[m * dim_N + n];
        mat2 = mat[(m + 1) * dim_N + n];
        vec1 = vec_x[m];
        vec2 = vec_x[m + 1];

        *sum1 += mat1 * vec1;
        *sum2 += mat2 * vec2;

        m += 2;
        ops++;
    } while (ops < num_ops);
}

static int linalg_gemv_trans_pulp_open_cluster(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int row_start;
    int row_end;
    int rem_ops;
    play_fmt_t sum1;
    play_fmt_t sum2;
    int block;
    int left;
    int id;
    int n;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_M);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_M);
        else
            vector_scale(vec_y, beta, dst, dim_M);
        return 0;
    }

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);
    rem_ops = dim_M % 2;

    n = row_start;
    if (rem_ops) {
        /* loop unroll will have leftovers */
        do {
            mat_vec_mul(mat, vec_x, n, dim_M, dim_N, &sum1, &sum2);
            /* leftover */
            sum1 += mat[(dim_M - 1) * dim_N + n] * vec_x[dim_M - 1];
            dst[n] = alpha * (sum1 + sum2) + beta * vec_y[n];
            n++;
        } while (n < row_end);

    } else {
        /* loop unroll will NOT have leftovers */
        do {
            mat_vec_mul(mat, vec_x, n, dim_M, dim_N, &sum1, &sum2);
            dst[n] = alpha * (sum1 + sum2) + beta * vec_y[n];
            n++;
        } while (n < row_end);
    }

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#endif  /* PLAY_PACKED */

#else   /* CLUSTER */

play_fmt_t ZERO_f = 0.0f;
play_fmt_t ONE_f = 1.0f;

static int linalg_gemv_trans_pulp_open_fc(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    play_fmt_t sum;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_M);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_M);
        else
            vector_scale(vec_y, beta, dst, dim_M);
        return 0;
    }

    for (int n = 0; n < dim_N; n++) {
        sum = 0;
        for (int m = 0; m < dim_M; m++)
            sum += mat[m * dim_N + n] * vec_x[m];
        dst[n] = alpha * sum + beta * vec_y[n];
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_gemv_trans_impl(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_gemv_trans_pulp_open_cluster(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_gemv_trans_pulp_open_fc(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
