#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int linalg_svd_jacobi(play_fmt_t *mat, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M)
{
    return linalg_svd_jacobi_impl(mat, mat_V, vec_S, dim_M);
}
