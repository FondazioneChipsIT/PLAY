#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_memcpy(const play_fmt_t *src, play_fmt_t *dst, const int len)
{
    return vector_memcpy_impl(src, dst, len);
}
