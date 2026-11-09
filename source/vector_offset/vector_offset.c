#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_offset(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len)
{
    return vector_offset_impl(src, offset, dst, len);
}
