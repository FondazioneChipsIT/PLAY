#include "internal/arch_interface.h"
#include "play.h"

#include "snrt.h"

static int linalg_gemv_trans_spatz_serial(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    float ZERO_f = 0.0f;
    float ONE_f = 1.0f;

    const float *row_m;
    const float *p_mat;
    float *p_dst;
    float elem_x;

    size_t avl;
    size_t vl;

    if (alpha == ZERO_f) {
        if (beta == ZERO_f)
            vector_set_all(dst, ZERO_f, dim_N);
        else if (beta == ONE_f)
            vector_memcpy(vec_y, dst, dim_N);
        else
            vector_scale(vec_y, beta, dst, dim_N);
        return 0;
    }

    if (beta == ZERO_f)
        vector_set_all(dst, ZERO_f, dim_N);
    else if (beta == ONE_f)
        vector_memcpy(vec_y, dst, dim_N);
    else
        vector_scale(vec_y, beta, dst, dim_N);

    avl = dim_N;
    p_dst = dst;
    p_mat = mat;

    for (; avl > 0; avl -= vl) {
        asm volatile ("vsetvli %0, %1, e32, m8, ta, ma" : "=r"(vl) : "r"(avl));
        asm volatile ("vfmv.v.f v0, %0" :: "f"(ZERO_f));
        asm volatile ("vle32.v v24, (%0)" :: "r"(p_dst));

        for (int m = 0; m < dim_M; m++) {
            row_m = p_mat + (m * dim_N);
            elem_x = vec_x[m];

            asm volatile ("vle32.v v8, (%0)" :: "r"(row_m));
            asm volatile ("vfmacc.vf v0, %0, v8" :: "f"(elem_x));
        }

        asm volatile ("vfmul.vf v0, v0, %0" :: "f"(alpha));
        asm volatile ("vfadd.vv v0, v24, v0");
        asm volatile ("vse32.v v0, (%0)" :: "r"(p_dst));

        p_dst += vl;
        p_mat += vl;
    }

    return 0;
}

int linalg_gemv_trans_impl(const float *mat, const float *vec_x, const float *vec_y, const float alpha, const float beta, float *dst, const int dim_M, const int dim_N)
{
    int ret;

#if NUM_CC > 1
    #error "Parallel execution on SPATZ not supported yet"
#else
    ret = linalg_gemv_trans_spatz_serial(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
#endif

    return ret;
}
