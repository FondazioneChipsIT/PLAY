#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_mul(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len)
{
    return vector_mul_impl(src_a, src_b, dst, len);
}
