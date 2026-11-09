#include "internal/arch_interface.h"
#include "play.h"

__attribute__((noinline)) int vector_set_all(play_fmt_t *vec, const play_fmt_t val, const int len)
{
    return vector_set_all_impl(vec, val, len);
}
