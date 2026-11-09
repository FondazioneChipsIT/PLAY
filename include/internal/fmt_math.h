#ifndef FMT_MATH_H_
#define FMT_MATH_H_

/*
 * Math helpers on the selected data format (see PLAY_FORMAT in play.h).
 *
 * FP32 keeps the calls the kernels used before the format switch (fabs on double, sqrtf), so
 * the generated code does not change. FP16 uses the half-precision builtins, so the operation
 * stays in _Float16 (fsqrt.h / sign-bit clear with Zhinx). BF16 has no sqrt instruction: the
 * operation goes through float.
 */

#include <math.h>

#include "play_fmt.h"

#if PLAY_FORMAT == PLAY_FMT_FP32
#define play_fabs(x)    fabs(x)
#define play_sqrt(x)    sqrtf(x)
#elif PLAY_FORMAT == PLAY_FMT_FP16
#define play_fabs(x)    __builtin_fabsf16(x)
#define play_sqrt(x)    __builtin_sqrtf16(x)
#elif PLAY_FORMAT == PLAY_FMT_BF16
#define play_fabs(x)    ((play_fmt_t) fabsf((float) (x)))
#define play_sqrt(x)    ((play_fmt_t) sqrtf((float) (x)))
#endif

#endif  /* FMT_MATH_H_ */
