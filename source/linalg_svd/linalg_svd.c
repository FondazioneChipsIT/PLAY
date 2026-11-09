#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int linalg_svd(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N)
{
    return linalg_svd_impl(src, dst, mat_V, vec_S, dim_M, dim_N);
}
