#ifndef PLAY_FMT_H_
#define PLAY_FMT_H_

/*
 * Data format.
 *
 * Every kernel works on play_fmt_t, selected at build time with -DPLAY_FORMAT=<value>:
 *   PLAY_FMT_FP32  float      (default)
 *   PLAY_FMT_FP16  _Float16   IEEE binary16; scalar ops need Zhinx, packed ops Xcvhv
 *   PLAY_FMT_BF16  __bf16     bfloat16;      scalar ops need Xcvbf, packed ops Xcvbfv
 * The test build sets it from FORMAT=FP32|FP16|BF16 (test/common/arch_select.mk).
 */
#define PLAY_FMT_FP32 1
#define PLAY_FMT_FP16 2
#define PLAY_FMT_BF16 3

#ifndef PLAY_FORMAT
#define PLAY_FORMAT PLAY_FMT_FP32
#endif

#if PLAY_FORMAT == PLAY_FMT_FP32
typedef float play_fmt_t;
#define PLAY_FORMAT_NAME "FP32"
#elif PLAY_FORMAT == PLAY_FMT_FP16
typedef _Float16 play_fmt_t;
#define PLAY_FORMAT_NAME "FP16"
#elif PLAY_FORMAT == PLAY_FMT_BF16
typedef __bf16 play_fmt_t;
#define PLAY_FORMAT_NAME "BF16"
#else
#error "PLAY_FORMAT must be PLAY_FMT_FP32, PLAY_FMT_FP16 or PLAY_FMT_BF16"
#endif

#endif  /* PLAY_FMT_H_ */
