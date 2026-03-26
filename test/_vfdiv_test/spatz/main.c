#include <stdbool.h>

#include "snrt.h"
#include "printf.h"

#define N 16

float *a __attribute__((aligned(32)));
float *b __attribute__((aligned(32)));
float *expected __attribute__((aligned(32)));
float *result __attribute__((aligned(32)));

static void init_data()
{
    printf("Initializing data...");

    a = snrt_l1alloc(N * sizeof(float));
    b = snrt_l1alloc(N * sizeof(float));
    expected = snrt_l1alloc(N * sizeof(float));
    result = snrt_l1alloc(N * sizeof(float));

    for (int i = 0; i < N; i++) {
        a[i] = 0.1f;
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

    float *pa = a;
    float *pb = b;
    float *pr = result;

    for (; avl > 0; avl -= vl) {

        asm volatile ("vsetvli %0, %1, e32, m8, ta, ma" : "=r"(vl) : "r"(avl));

        asm volatile ("vle32.v v0, (%0)" :: "r"(pa));
        asm volatile ("vle32.v v8, (%0)" :: "r"(pb));

        asm volatile ("vfdiv.vv v16, v0, v8");

        snrt_cluster_hw_barrier();

        asm volatile ("vse32.v v16, (%0)" :: "r"(pr));

        pa += vl;
        pb += vl;
        pr += vl;
    }


    printf(" done!\n");
}

static void check_result()
{
    printf("Checking result...\n");

    float diff;
    float TOLL = 0.0004f;
    bool check = true;

    for (int i = 0; i < N; i++) {
        printf("idx:%d - result=%f - expected=%f --> ", i, result[i], expected[i]);

        diff = result[i] - expected[i];
        diff = (diff < 0) ? -diff : diff;
        if (diff > TOLL) {
            check = false;
            printf("ERROR: diff: %f\n", diff);
        } else {
            printf("OK\n");
        }
    }

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
