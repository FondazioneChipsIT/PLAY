#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int linalg_cholesky_decomp(const play_fmt_t *src, play_fmt_t *dst, const int dim)
{
    return linalg_cholesky_decomp_impl(src, dst, dim);
}
