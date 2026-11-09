#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int matrix_mul(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P)
{
    return matrix_mul_impl(src_a, src_b, dst, dim_M, dim_N, dim_P);
}
