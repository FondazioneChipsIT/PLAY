#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int linalg_lu_decomp(play_fmt_t *mat, int *perm, const int dim_M, const int dim_N)
{
    return linalg_lu_decomp_impl(mat, perm, dim_M, dim_N);
}
