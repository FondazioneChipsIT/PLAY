#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

PI_L1 float ZERO_f = 0.0f;
PI_L1 float ONE_f = 1.0f;

static int linalg_gemv_trans_pulp_open_cluster(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    int col_start;
    int col_end;
    int cols_quad;
    int block;
    int left;
    int id;
    int m, n;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_N);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_N);
        else
            vector_scale(vec_y, beta, dst, dim_N);
        return 0;
    }

    id = pi_core_id();
    block = dim_N / NUM_CORES;
    left = dim_N % NUM_CORES;
    col_start = id * block + (id < left ? id : left);
    col_end = col_start + block + (id < left ? 1 : 0);

    if (dim_M < 2) {
        for (n = col_start; n < col_end; n++)
            dst[n] = alpha * (dim_M == 1 ? mat[n] * vec_x[0] : ZERO_f) + beta * vec_y[n];
        goto exit;
    }

    /* four columns at a time */
    cols_quad = col_start + ((col_end - col_start) & ~3);

    for (n = col_start; n < cols_quad; n += 4) {    /* columns n..n+3 of mat, dst[n]..dst[n+3] */
        const float *c = mat + n;                   /* mat[0][n] */
        float s0, s1, s2, s3;                       /* dst[n]..dst[n+3], before alpha and beta */

        s0 = 0;
        s1 = 0;
        s2 = 0;
        s3 = 0;
        m = 0;
        do {
            float x;

            x = vec_x[m];                           /* vec_x[m], shared by the four columns */
            s0 += c[m * dim_N] * x;                 /* mat[m][n] * vec_x[m] */
            s1 += c[m * dim_N + 1] * x;             /* mat[m][n+1] * vec_x[m] */
            s2 += c[m * dim_N + 2] * x;             /* mat[m][n+2] * vec_x[m] */
            s3 += c[m * dim_N + 3] * x;             /* mat[m][n+3] * vec_x[m] */
            m++;
        } while (m < dim_M);

        dst[n] = alpha * s0 + beta * vec_y[n];
        dst[n + 1] = alpha * s1 + beta * vec_y[n + 1];
        dst[n + 2] = alpha * s2 + beta * vec_y[n + 2];
        dst[n + 3] = alpha * s3 + beta * vec_y[n + 3];
    }

    for (; n < col_end; n++) {
        /* leftover columns, one at a time with two accumulators */
        const float *c = mat + n;
        float s0, s1;

        s0 = 0;
        s1 = 0;
        for (m = 0; m + 1 < dim_M; m += 2) {
            s0 += c[m * dim_N] * vec_x[m];
            s1 += c[(m + 1) * dim_N] * vec_x[m + 1];
        }
        if (m < dim_M)
            s0 += c[m * dim_N] * vec_x[m];

        dst[n] = alpha * (s0 + s1) + beta * vec_y[n];
    }

exit:

#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif

    return 0;
}

#else   /* CLUSTER */

float ZERO_f = 0.0f;
float ONE_f = 1.0f;

static int linalg_gemv_trans_pulp_open_fc(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    float sum;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_N);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_N);
        else
            vector_scale(vec_y, beta, dst, dim_N);
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

int linalg_gemv_trans_impl(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_gemv_trans_pulp_open_cluster(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_gemv_trans_pulp_open_fc(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
