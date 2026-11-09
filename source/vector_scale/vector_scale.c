#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_scale(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len)
{
    return vector_scale_impl(src, val, dst, len);
}
