#include <stdbool.h>
#include <math.h>

#include "data.h"
#include "play.h"
#include "stats.h"
#include "utils.h"

#include "pmsis.h"

PI_L1 play_fmt_t src[DIM_M * DIM_N] __attribute__((aligned(4)));
PI_L1 play_fmt_t mat_V[DIM_N * DIM_N] __attribute__((aligned(4)));
PI_L1 play_fmt_t vec_S[DIM_N] __attribute__((aligned(4)));
PI_L1 play_fmt_t result[DIM_M * DIM_N] __attribute__((aligned(4)));

static void initialize_data()
{
    if (!is_master_core())
        return;

    for (int m = 0; m < DIM_M; m++) {
        for (int n = 0; n < DIM_N; n++) {
            src[m * DIM_N + n] = mat[m * DIM_N + n];
            result[m * DIM_N + n] = 0;
        }
    }

    for (int m = 0; m < DIM_N; m++) {
        for (int n = 0; n < DIM_N; n++) {
            mat_V[m * DIM_N + n] = V[m * DIM_N + n];
        }
    }

    for (int n = 0; n < DIM_N; n++)
        vec_S[n] = S[n];

}

static void check_result()
{
    bool test_result;

    if (!is_master_core())
        return;

    test_result = matrix_compare_abs(result, expected, DIM_M, DIM_N);

#ifdef PRINT_DATA
    matrix_print(src, DIM_M, DIM_N, "mat");
    matrix_print(mat_V, DIM_N, DIM_N, "V");
    vector_print(vec_S, DIM_N, "S");
    matrix_print(result, DIM_M, DIM_N, "result");
    matrix_print(expected, DIM_M, DIM_N, "expected");
#endif  /* PRINT_DATA */

    printf("INFO | Test %s\n", test_result ? "SUCCESS" : "FAILED");
}

static void run_test()
{
    volatile int m = DIM_M;
    volatile int n = DIM_N;
    initialize_data();
    barrier();

    INIT_STATS();
    START_LOOP_STATS();
    START_STATS();
    linalg_svd_lsv(src, mat_V, vec_S, result, m, n);
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

static int test_linalg_svd_lsv()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'linalg_svd_lsv' test on PULP Cluster with %d cores\n", NUM_CORES);
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

static int test_linalg_svd_lsv()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'linalg_svd_lsv' test on Fabric Controller\n");
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
    printf("\n##################################### LINALG_SVD_LSV TEST ####################################\n\n");
#endif  /* ENABLE_LOGGING */

    ret = test_linalg_svd_lsv();

#ifdef  ENABLE_LOGGING
    printf("\n##########################################################################################\n\n");
#endif  /* ENABLE_LOGGING */

    pmsis_exit(ret);
}

int main()
{
    return pmsis_kickoff(test_kickoff);
}
