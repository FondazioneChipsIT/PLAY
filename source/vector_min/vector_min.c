#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_min(const play_fmt_t *src, play_fmt_t *min, const int len)
{
    return vector_min_impl(src, min, len);
}
