#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "snrt.h"
#include "printf.h"

#define N 16
#define ULP_TOLL 80

_Float16 *a __attribute__((aligned(16)));
_Float16 *b __attribute__((aligned(16)));
_Float16 *expected __attribute__((aligned(16)));
_Float16 *result __attribute__((aligned(16)));

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

static void init_data()
{
    printf("Initializing data...");

    a = snrt_l1alloc(N * sizeof(_Float16));
    b = snrt_l1alloc(N * sizeof(_Float16));
    expected = snrt_l1alloc(N * sizeof(_Float16));
    result = snrt_l1alloc(N * sizeof(_Float16));

    for (int i = 0; i < N; i++) {
        a[i] = (_Float16)i / 10.0f;
        b[i] = 0.001f;
        result[i] = 0.0f;
        expected[i] = a[i] / b[i];
        snrt_cluster_hw_barrier();
    }

    printf(" done!\n");
}

static void test_vfdiv()
{
    printf("Starting RVV division test...\n");

    size_t avl = N;
    size_t vl;

    _Float16 *pa = a;
    _Float16 *pb = b;
    _Float16 *pr = result;

    for (; avl > 0; avl -= vl) {

        asm volatile ("vsetvli %0, %1, e16, m8, ta, ma" : "=r"(vl) : "r"(avl));

        asm volatile ("vle16.v v0, (%0)" :: "r"(pa));
        asm volatile ("vle16.v v8, (%0)" :: "r"(pb));

        asm volatile ("vfdiv.vv v16, v0, v8");

        snrt_cluster_hw_barrier();

        asm volatile ("vse16.v v16, (%0)" :: "r"(pr));

        pa += vl;
        pb += vl;
        pr += vl;
    }


    printf(" done!\n");
}

static void check_result()
{
    printf("Checking result...\n");

    bool check = vector_compare_fp16_bitwise(result, expected, N);

    if (check)
        printf("Test SUCCESS\n");
    else
        printf("Test FAILED\n");
}

int main()
{
    init_data();
    test_vfdiv();
    check_result();
    return 0;
}
