#include <stdbool.h>

#include "data.h"
#include "play.h"
#include "stats.h"
#include "utils.h"

#include "pmsis.h"

PI_L1 play_fmt_t src_a[DIM_M * DIM_N] __attribute__((aligned(4)));
PI_L1 play_fmt_t src_b[DIM_P * DIM_N] __attribute__((aligned(4)));
PI_L1 play_fmt_t result[DIM_M * DIM_P] __attribute__((aligned(4)));

static void initialize_matrices()
{
    if (!is_master_core())
        return;

    for (int m = 0; m < DIM_M; m++) {
        for (int n = 0; n < DIM_N; n++)
            src_a[m * DIM_N + n] = mat_a[m * DIM_N + n];
    }

    for (int p = 0; p < DIM_P; p++)
        for (int n = 0; n < DIM_N; n++) {
            src_b[p * DIM_N + n] = mat_b[p * DIM_N + n];
    }

    for (int m = 0; m < DIM_M; m++) {
        for (int p = 0; p < DIM_P; p++) {
            result[m * DIM_P + p] = 0;
        }
    }
}

static void check_result()
{
    bool test_result;

    if (!is_master_core())
        return;

    test_result = matrix_compare(result, expected, DIM_M, DIM_P);

#ifdef  PRINT_DATA
    matrix_print(src_a, DIM_M, DIM_N, "src_a");
    matrix_print(src_b, DIM_P, DIM_N, "src_b");
    matrix_print(result, DIM_M, DIM_P, "result");
    matrix_print(expected, DIM_M, DIM_P, "expected");
#endif  /* PRINT_DATA */

    printf("INFO | Test %s\n", test_result ? "SUCCESS" : "FAILED");
}

static void run_test()
{
    volatile int m = DIM_M;
    volatile int n = DIM_N;
    volatile int p = DIM_P;
    initialize_matrices();
    barrier();

    INIT_STATS();
    START_LOOP_STATS();
    START_STATS();
    matrix_mul_trans_B(src_a, src_b, result, m, n, p);
    STOP_STATS();
    END_LOOP_STATS();

    barrier();
    check_result();
}

#ifdef  CLUSTER

static void cluster_entry()
{
    pi_cl_team_fork(NUM_CORES, run_test, NULL);
}

static int run_test_on_cluster()
{
    struct pi_cluster_conf cl_conf;
    struct pi_cluster_task cl_task;
    struct pi_device cluster_dev;
    int ret;

    pi_cluster_conf_init(&cl_conf);
    pi_open_from_conf(&cluster_dev, &cl_conf);

    ret = pi_cluster_open(&cluster_dev);
    if (ret) {
#ifdef  ENABLE_LOGGING
        printf("ERROR | Unable to open cluster device\n");
#endif  /* ENABLE_LOGGING */
        goto exit;
    }

    pi_cluster_task(&cl_task, cluster_entry, NULL);
    ret = pi_cluster_send_task_to_cl(&cluster_dev, &cl_task);
    if (ret) {
#ifdef  ENABLE_LOGGING
        printf("ERROR | Unable to send task to cluster controller\n");
#endif  /* ENABLE_LOGGING */
        goto exit;
    }

    pi_cluster_close(&cluster_dev);

exit:
    return ret;
}

static int test_matrix_mul_trans_B()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'matrix_mul_trans_B' test on PULP Cluster with %d cores\n", NUM_CORES);
#endif  /* ENABLE_LOGGING */

    ret = run_test_on_cluster();

#ifdef  ENABLE_LOGGING
    if (ret)
        printf("ERROR | Unable to run test on cluster\n");
#endif  /* ENABLE_LOGGING */

    return ret;
}

#else   /* CLUSTER */

static int run_test_on_fabric()
{
    run_test();
    return 0;
}

static int test_matrix_mul_trans_B()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'matrix_mul_trans_B' test on Fabric Controller\n");
#endif  /* ENABLE_LOGGING */

    ret = run_test_on_fabric();

#ifdef  ENABLE_LOGGING
    if (ret)
        printf("ERROR | Unable to run test on FC\n");
#endif  /* ENABLE_LOGGING */

    return ret;
}

#endif  /* CLUSTER */

static void test_kickoff()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("\n##################################### MATRIX_MUL_TRANS_B TEST ####################################\n\n");
#endif  /* ENABLE_LOGGING */

    ret = test_matrix_mul_trans_B();

#ifdef  ENABLE_LOGGING
    printf("\n##########################################################################################\n\n");
#endif  /* ENABLE_LOGGING */

    pmsis_exit(ret);
}

int main()
{
    return pmsis_kickoff(test_kickoff);
}
