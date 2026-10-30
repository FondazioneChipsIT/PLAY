#include "internal/arch_interface.h"
#include "play.h"

#include "pmsis.h"

#ifdef  CLUSTER

PI_L1 float ZERO_f = 0.0f;
PI_L1 float ONE_f = 1.0f;

static int linalg_gemv_pulp_open_cluster(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    int row_start;
    int row_end;
    int rows_quad;
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
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    row_start = id * block + (id < left ? id : left);
    row_end = row_start + block + (id < left ? 1 : 0);

    if (dim_N < 2) {
        for (m = row_start; m < row_end; m++)
            dst[m] = alpha * (dim_N == 1 ? mat[m] * vec_x[0] : ZERO_f) + beta * vec_y[m];
        goto exit;
    }

    /* four rows at a time */
    rows_quad = row_start + ((row_end - row_start) & ~3);

    for (m = row_start; m < rows_quad; m += 4) {    /* rows m..m+3 of mat and dst */
        const float *r0 = mat + m * dim_N;          /* row m of mat */
        const float *r1 = r0 + dim_N;               /* row m+1 of mat */
        const float *r2 = r1 + dim_N;               /* row m+2 of mat */
        const float *r3 = r2 + dim_N;               /* row m+3 of mat */
        float s0, s1, s2, s3;                       /* dst[m]..dst[m+3], before alpha and beta */

        s0 = 0;
        s1 = 0;
        s2 = 0;
        s3 = 0;
        n = 0;
        do {
            float x;

            x = vec_x[n];                           /* vec_x[n], shared by the four rows */
            s0 += r0[n] * x;                        /* mat[m][n] * vec_x[n] */
            s1 += r1[n] * x;                        /* mat[m+1][n] * vec_x[n] */
            s2 += r2[n] * x;                        /* mat[m+2][n] * vec_x[n] */
            s3 += r3[n] * x;                        /* mat[m+3][n] * vec_x[n] */
            n++;
        } while (n < dim_N);

        dst[m] = alpha * s0 + beta * vec_y[m];
        dst[m + 1] = alpha * s1 + beta * vec_y[m + 1];
        dst[m + 2] = alpha * s2 + beta * vec_y[m + 2];
        dst[m + 3] = alpha * s3 + beta * vec_y[m + 3];
    }

    for (; m < row_end; m++) {
        /* leftover rows, one at a time with two accumulators */
        const float *r0 = mat + m * dim_N;
        float s0, s1;

        s0 = 0;
        s1 = 0;
        for (n = 0; n + 1 < dim_N; n += 2) {
            s0 += r0[n] * vec_x[n];
            s1 += r0[n + 1] * vec_x[n + 1];
        }
        if (n < dim_N)
            s0 += r0[n] * vec_x[n];

        dst[m] = alpha * (s0 + s1) + beta * vec_y[m];
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

static int linalg_gemv_pulp_open_fc(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    float sum;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_M);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_M);
        else
            vector_scale(vec_y, beta, dst, dim_M);
        return 0;
    }

    for (int m = 0; m < dim_M; m++) {
        sum = 0;
        for (int n = 0; n < dim_N; n++)
            sum += mat[m * dim_N + n] * vec_x[n];
        dst[m] = alpha * sum + beta * vec_y[m];
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_gemv_impl(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_gemv_pulp_open_cluster(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#else   /* CLUSTER */
    ret = linalg_gemv_pulp_open_fc(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#endif  /* CLUSTER */

    return ret;
}
