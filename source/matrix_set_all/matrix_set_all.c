#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int matrix_set_all(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N)
{
    return matrix_set_all_impl(mat, val, dim_M, dim_N);
}
