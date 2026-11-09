#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int linalg_gemv_trans(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N)
{
    return linalg_gemv_trans_impl(mat, vec_x, vec_y, alpha, beta, dst, dim_M, dim_N);
}
