#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "snrt.h"
#include "printf.h"

#define N        16
#define ULP_TOLL 80

static const _Float16 input[] = { 3.201172f, 2.734375f, 1.083008f, 0.301758f,
                               0.987305f, 1.052734f, 0.218628f, 1.004883f,
                               1.408203f, 0.584961f, 0.211304f, 1.275391f,
                              -0.474365f, 0.453125f, 0.547363f, 0.420410f };

_Float16 *src;
_Float16 *dst;

static inline uint16_t get_raw(_Float16 val)
{
    uint16_t raw;
    memcpy(&raw, &val, sizeof(raw));
    return raw;
}

static bool vector_compare_fp16_bitwise(const _Float16 *res, const _Float16 *exp, int len)
{
    bool ret = true;

    for (int i = 0; i < len; i++) {
        uint16_t e = get_raw(exp[i]);
        uint16_t r = get_raw(res[i]);
        int32_t d = (int32_t)e - (int32_t)r;
        if (d < 0)
            d = -d;

        if (d > ULP_TOLL) {
            printf("Mismatch idx %d - expected 0x%04x - computed 0x%04x - ulp %d\n", i, e, r, d);
            ret = false;
        } else {
            printf("idx %d - computed 0x%04x - expected 0x%04x --> OK\n", i, r, e);
        }
    }

    return ret;
}

static void init_data()
{
    src = snrt_l1alloc(N * sizeof(_Float16));
    dst = snrt_l1alloc(N * sizeof(_Float16));

    for (int i = 0; i < N; i++)
        src[i] = input[i];
}

static bool run_case(const char *name, int src_idx, int dst_idx)
{
    _Float16 *p_src = src + src_idx;
    _Float16 *p_dst = dst + dst_idx;
    int len = N - (src_idx > dst_idx ? src_idx : dst_idx);
    size_t avl = len;
    size_t vl;

    for (int i = 0; i < N; i++)
        dst[i] = 0.0f;

    for (; avl > 0; avl -= vl) {
        asm volatile ("vsetvli %0, %1, e16, m8, ta, ma" : "=r"(vl) : "r"(avl));
        asm volatile ("vle16.v v0, (%0)" :: "r"(p_src));
        asm volatile ("vse16.v v0, (%0)" :: "r"(p_dst) : "memory");

        p_src += vl;
        p_dst += vl;
    }

    printf("\n[%s] src_idx=%d dst_idx=%d len=%d\n", name, src_idx, dst_idx, len);
    bool ok = vector_compare_fp16_bitwise(dst + dst_idx, src + src_idx, len);
    printf("[%s] %s\n", name, ok ? "SUCCESS" : "FAILED");

    return ok;
}

int main()
{
    printf("\n#################### VLE/VSE ALIGNMENT ISOLATION TEST ####################\n");

    init_data();

    run_case("baseline aligned", 0, 0);
    run_case("load  misaligned", 1, 0);
    run_case("store misaligned", 0, 1);

    printf("#########################################################################\n\n");

    return 0;
}
