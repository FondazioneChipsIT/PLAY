#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "snrt.h"
#include "printf.h"

#define FP16

#ifdef FP16
typedef _Float16 VAR_T;
#define TYPE "_Float16"
#else
typedef float VAR_T;
#define VAR_T_FMT "%f"
#define TYPE "float"
#endif

#define N 16
#define START_IDX 0
#define ULP_TOLL 80
#define TOLL 0.004f

static const VAR_T input_a[] = { 3.201172f, 2.734375f, 1.083008f, 0.301758f, 0.987305f, 1.052734f, 0.218628f, 1.004883f, 1.408203f, 0.584961f, 0.211304f, 1.275391f, -0.474365f, 0.453125f, 0.547363f, 0.420410f};
static const VAR_T input_b[] = { -0.383789f, 1.050781f, -0.299805f, 0.915039f, 0.297852f, 1.842773f, 0.703125f, -0.224121f, 2.724609f, -0.670410f, 1.736328f, 0.505371f, 0.752930f, -1.387695f, -1.142578f, 0.120300f };

VAR_T *expected __attribute__((aligned(32)));
VAR_T *result __attribute__((aligned(32)));
VAR_T *src_a __attribute__((aligned(32)));
VAR_T *src_b __attribute__((aligned(32)));

#ifdef FP16

static inline uint16_t get_raw(_Float16 val)
{
    uint16_t raw;
    memcpy(&raw, &val, sizeof(raw));
    return raw;
}

static inline bool fp16_is_invalid(uint16_t x)
{
    return (x & 0x7C00) == 0x7C00;
}

static inline int32_t fp16_to_ordered(uint16_t x)
{
    int32_t i = (int32_t)x;
    return (i & 0x8000) ? (0x8000 - (i & 0x7FFF)) : (i + 0x8000);
}

static inline bool vector_compare_fp16_bitwise(const _Float16 *res, const _Float16 *exp, int len)
{
    bool ret = true;

    for (int i = 0; i < len; i++) {
        uint16_t expected_raw = get_raw(exp[i]);
        uint16_t result_raw = get_raw(res[i]);

        if (fp16_is_invalid(expected_raw) || fp16_is_invalid(result_raw)) {
            printf("Invalid FP16 value at idx %d - expected: 0x%04x - computed: 0x%04x\n",
                   i, expected_raw, result_raw);
            ret = false;
            continue;
        }

        int32_t ord_exp = fp16_to_ordered(expected_raw);
        int32_t ord_res = fp16_to_ordered(result_raw);
        int32_t ulp_dif = (ord_exp > ord_res) ? (ord_exp - ord_res) : (ord_res - ord_exp);

        if (ulp_dif > ULP_TOLL) {
            printf("Mismatch at index %d - expected: 0x%04x - computed: 0x%04x - ulp: %d\n",
                   i, expected_raw, result_raw, ulp_dif);
            ret = false;
        } else {
            printf("idx:%d - result=0x%04x - expected=0x%04x --> OK\n",
                   i, result_raw, expected_raw);
        }
    }

    return ret;
}

#else

bool vector_compare(const float *res, const float *exp, const int len)
{
    float abs_diff;
    bool ret;

    ret = true;
    for (int i = 0; i < len; i++) {
        abs_diff = (exp[i] > res[i]) ? (exp[i] - res[i]) : (res[i] - exp[i]);
        if (abs_diff > TOLL) {
            printf("Mismatch at index %d - expected: %.4f - computed: %.4f - abs_diff=%f\n", i, exp[i], res[i], abs_diff);
            ret = false;
        } else {
            printf("idx: %d - expected: %.4f - computed: %.4f --> OK\n", i, exp[i], res[i]);
        }

    }

    return ret;
}

#endif

static void init_data()
{
    printf("Initializing data...");

    expected = snrt_l1alloc(N * sizeof(VAR_T));
    result = snrt_l1alloc(N * sizeof(VAR_T));
    src_a = snrt_l1alloc(N * sizeof(VAR_T));
    src_b = snrt_l1alloc(N * sizeof(VAR_T));

    for (int i = 0; i < N; i++) {
        src_a[i] = input_a[i];
        src_b[i] = input_b[i];
        expected[i] = src_a[i] + src_b[i];
        result[i] = 0.0f;
    }

    printf(" done!\n");
}

static void test_vle_odd_idx()
{
    printf("Executing test...");

    VAR_T *p_src_a;
    VAR_T *p_src_b;
    VAR_T *p_dst;
    size_t avl;
    size_t vl;

    p_src_a = src_a + START_IDX;
    p_src_b = src_b + START_IDX;
    p_dst = result + START_IDX;

    for (; avl > 0; avl -= vl) {

#ifdef FP16
        asm volatile ("vsetvli %0, %1, e16, m8, ta, ma" : "=r"(vl) : "r"(avl));

        asm volatile ("vle16.v v0, (%0)" :: "r"(p_src_a));
        asm volatile ("vle16.v v8, (%0)" :: "r"(p_src_b));

        asm volatile ("vfadd.vv v16, v0, v8");

        asm volatile ("vse16.v v16, (%0)" :: "r"(p_dst));
#else
        asm volatile ("vsetvli %0, %1, e32, m8, ta, ma" : "=r"(vl) : "r"(avl));

        asm volatile ("vle32.v v0, (%0)" :: "r"(p_src_a));
        asm volatile ("vle32.v v8, (%0)" :: "r"(p_src_b));

        asm volatile ("vfadd.vv v16, v0, v8");

        asm volatile ("vse32.v v16, (%0)" :: "r"(p_dst));
#endif

        p_src_a += vl;
        p_src_b += vl;
        p_dst += vl;
    }

    printf(" done!\n");
}

static void check_result()
{
    printf("Checking result...\n");

#ifdef FP16
    bool check = vector_compare_fp16_bitwise(result + START_IDX, expected + START_IDX, (N - START_IDX));
#else
    bool check = vector_compare(result + START_IDX, expected + START_IDX, (N - START_IDX));
#endif

    if (check)
        printf("Test SUCCESS\n");
    else
        printf("Test FAILED\n");
}

int main()
{
    printf("\n#################### VLE ODD IDX TEST (idx: %d - Type: %s ) ####################\n", START_IDX, TYPE);

    init_data();
    test_vle_odd_idx();
    check_result();

    printf("###############################################################\n\n");

    return 0;
}
