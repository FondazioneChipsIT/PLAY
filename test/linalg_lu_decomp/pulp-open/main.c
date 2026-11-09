#include <stdbool.h>

#include "data.h"
#include "play.h"
#include "stats.h"
#include "utils.h"

#include "pmsis.h"

PI_L1 int perm[DIM_M] __attribute__((aligned(4)));
PI_L1 play_fmt_t src[DIM_M * DIM_N] __attribute__((aligned(4)));

static void initialize_data()
{
    if (!is_master_core())
        return;

    for (int m = 0; m < DIM_M; m++){
        for (int n = 0; n < DIM_N; n++)
            src[m * DIM_N + n] = mat[m * DIM_N + n];
    }

    for (int m = 0; m < DIM_M; m++) {
        perm[m] = 0;
    }
}

static void check_result()
{
    bool test_result;

    if (!is_master_core())
        return;

    test_result = matrix_compare(src, expected_mat, DIM_M, DIM_N);
    if (test_result)
        test_result = vector_compare((play_fmt_t *) perm, (play_fmt_t *) expected_perm, DIM_M);

#ifdef  PRINT_DATA
    matrix_print(src, DIM_M, DIM_N, "result");
    matrix_print(expected_mat, DIM_M, DIM_N, "expected mat");
    vector_print((play_fmt_t *)perm, DIM_M, "computed permutation");
    vector_print((play_fmt_t *)expected_perm, DIM_M, "expected permutation");
#endif  /* PRINT_DATA */

    printf("INFO | Test %s\n", test_result ? "SUCCESS" : "FAILED");
}

static void run_test()
{
    volatile int m = DIM_M;
    volatile int n = DIM_N;

    INIT_STATS();
    START_LOOP_STATS();
    initialize_data();
    barrier();
    START_STATS();
    linalg_lu_decomp(src, perm, m, n);
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

static int test_linalg_lu_decomp()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'linalg_lu_decomp' test on PULP Cluster with %d cores\n", NUM_CORES);
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

static int test_linalg_lu_decomp()
{
    int ret;

#ifdef  ENABLE_LOGGING
    printf("INFO | Running 'linalg_lu_decomp' test on Fabric Controller\n");
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
    printf("\n##################################### LINALG_LU_DECOMP TEST ####################################\n\n");
#endif  /* ENABLE_LOGGING */

    ret = test_linalg_lu_decomp();

#ifdef  ENABLE_LOGGING
    printf("\n##########################################################################################\n\n");
#endif  /* ENABLE_LOGGING */

    pmsis_exit(ret);
}

int main()
{
    return pmsis_kickoff(test_kickoff);
}
