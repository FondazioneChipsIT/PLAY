#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_axpy(const play_fmt_t *src_a, const play_fmt_t *src_b, const play_fmt_t alpha, play_fmt_t *dst, const int len)
{
    return vector_axpy_impl(src_a, src_b, alpha, dst, len);
}
