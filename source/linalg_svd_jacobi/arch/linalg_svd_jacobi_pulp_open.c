#include "internal/arch_interface.h"
#include "play.h"

#include <stdbool.h>
#include <math.h>

#include "pmsis.h"

#ifdef CLUSTER

static PI_L1 float ZERO_f = 0.0f;
static PI_L1 float ONE_f = 1.0f;
static PI_L1 float TWO_f = 2.0f;
static PI_L1 int MAX_ITER = 200;
static PI_L1 float EPSILON = 1e-12;

/* indexed by round parity: a core may write the next round's maximum while a slower one
 * is still reading this round's */
PI_L1 float local_max[2][NUM_CORES];
/* rotation of every pair of the current round, written in the row phase and read in the
 * column phase: cos, sin (cos == 0 marks a pair that is not rotated), i, j */
PI_L1 float *rot;

static inline void barrier()
{
#if NUM_CORES > 1
    pi_cl_team_barrier();
#endif
}

static int matrix_set_identity(float *mat, const int dim_M)
{
    int start;
    int block;
    int left;
    int end;
    int id;

    id = pi_core_id();
    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    for (int m = start; m < end; m++)
            mat[m * dim_M + m] = ONE_f;

    barrier();

    return 0;
}

int linalg_svd_jacobi_pulp_open_cluster( float *mat, float *mat_V, float *vec_S, const int dim_M)
{
    int pairs_per_round;
    float max_offdiag;
    int even_dim;
    int iter;
    int id;

    matrix_set_identity(mat_V, dim_M);

    id = pi_core_id();
    even_dim = (dim_M % 2 == 0) ? dim_M : (dim_M + 1);
    pairs_per_round = even_dim / 2;

    max_offdiag = 0;

    if (id == 0)
        rot = pmsis_l1_malloc(4 * pairs_per_round * sizeof(float));

    barrier();

    iter = 0;
    while (iter++ < MAX_ITER) {

        for (int round = 0; round < (even_dim - 1); round++) {
            int pair_start;
            int pair_end;
            int block;
            int left;
            float lmax;

            block = pairs_per_round / NUM_CORES;
            left = pairs_per_round % NUM_CORES;
            pair_start = id * block + (id < left ? id : left);
            pair_end = pair_start + block + (id < left ? 1 : 0);

            /* The pairs of a round are disjoint, and no rotation touches the elements
             * (i,i), (j,j), (i,j) of another pair: all the row rotations of the round can
             * go first and all the column rotations after, with one barrier in between
             * instead of two per pair */
            for (int pair = pair_start; pair < pair_end; pair++) {
                bool compute;
                float cos;
                float sin;
                float tau;
                float t;
                int i;
                int j;

                /* round + pair and even_dim - 1 - pair + round stay below 2 * (even_dim - 1):
                 * one conditional subtraction instead of the modulo (a division, up to 35
                 * cycles on CV32E40P) */
                i = round + pair;
                if (i >= even_dim - 1)
                    i -= even_dim - 1;
                if (pair == 0) {
                    j = even_dim - 1;
                } else {
                    j = even_dim - 1 - pair + round;
                    if (j >= even_dim - 1)
                        j -= even_dim - 1;
                }

                compute = true;
                if (i >= dim_M || j >= dim_M)
                    compute = false;

                if (compute && fabs(mat[i * dim_M + j]) < EPSILON)
                    compute = false;

                if (compute) {
                    tau = (mat[j * dim_M + j] - mat[i * dim_M + i]) / (TWO_f * mat[i * dim_M + j]);
                    if (tau >= ZERO_f)
                        t = ONE_f / (tau + sqrtf(ONE_f + tau * tau));
                    else
                        t = ONE_f / (tau - sqrtf(ONE_f + tau * tau));

                    cos = ONE_f / sqrtf(ONE_f + t * t);
                    sin = t * cos;

                    /* Update rows i and j of MAT */
                    for (int m = 0; m < dim_M; m++) {
                        float im;
                        float jm;

                        im = mat[i * dim_M + m];
                        jm = mat[j * dim_M + m];

                        mat[i * dim_M + m] = (cos * im) - (sin * jm);
                        mat[j * dim_M + m] = (sin * im) + (cos * jm);
                    }

                } else {
                    cos = 0;
                    sin = 0;
                }

                rot[4 * pair] = cos;
                rot[4 * pair + 1] = sin;
                ((int *) rot)[4 * pair + 2] = i;
                ((int *) rot)[4 * pair + 3] = j;
            }

            barrier();

            lmax = 0;
            for (int pair = pair_start; pair < pair_end; pair++) {
                float cos;
                float sin;
                int i;
                int j;

                cos = rot[4 * pair];
                if (cos == ZERO_f)
                    continue;

                sin = rot[4 * pair + 1];
                i = ((int *) rot)[4 * pair + 2];
                j = ((int *) rot)[4 * pair + 3];

                /* Update cols i and j of MAT */
                for (int n = 0; n < dim_M; n++) {
                    float ni;
                    float nj;

                    ni = mat[n * dim_M + i];
                    nj = mat[n * dim_M + j];

                    mat[n * dim_M + i] = (cos * ni) - (sin * nj);
                    mat[n * dim_M + j] = (sin * ni) + (cos * nj);
                }

                /* Update cols i and j of V */
                for (int n = 0; n < dim_M; n++) {
                    float ni;
                    float nj;

                    ni = mat_V[n * dim_M + i];
                    nj = mat_V[n * dim_M + j];

                    mat_V[n * dim_M + i] = (cos * ni) - (sin * nj);
                    mat_V[n * dim_M + j] = (sin * ni) + (cos * nj);
                }

                if (fabs(mat[i * dim_M + j]) > lmax)
                    lmax = fabs(mat[i * dim_M + j]);
            }

            local_max[round & 1][id] = lmax;

            barrier();

            /* every core reduces on its own: no serial step on core 0, no extra barrier */
            max_offdiag = local_max[round & 1][0];
            for (int cid = 1; cid < NUM_CORES; cid++)
                if (local_max[round & 1][cid] > max_offdiag)
                    max_offdiag = local_max[round & 1][cid];

            if (max_offdiag < EPSILON)
                break;
        }   /* Round */

        if (max_offdiag < EPSILON)
            break;

    }   /* Iters */

    barrier();

    if (id == 0)
        pmsis_l1_malloc_free(rot, 4 * pairs_per_round * sizeof(float));

    if (iter >= MAX_ITER) {
#ifdef ENABLE_LOGGING
        if (id == 0)
            printf("ERROR | Problem solution not reached in %d iterations\n");
#endif
        pmsis_exit(-1);
    }

    int block;
    int start;
    int left;
    int end;

    block = dim_M / NUM_CORES;
    left = dim_M % NUM_CORES;
    start = id * block + (id < left ? id : left);
    end = start + block + (id < left ? 1 : 0);

    for (int i = start; i < end; i++) {
        if (mat[i * dim_M + i] > 0)
            vec_S[i] = sqrtf(mat[i * dim_M + i]);
        else
            vec_S[i] = 0;
    }

    barrier();

    return 0;
}

#else   /* CLUSTER */

/* these variable needs to be re-defined for FC since it does not have access to L1 memory */
static float ZERO_f = 0.0f;
static float ONE_f = 1.0f;
static float TWO_f = 2.0f;
static int MAX_ITER = 200;
static float EPSILON = 1e-12;

static int matrix_set_identity(float *mat, const int dim_M)
{
    for (int m = 0; m < dim_M; m++)
        mat[m * dim_M + m] = ONE_f;

    return 0;
}

static int linalg_svd_jacobi_pulp_open_fc(float *mat, float *mat_V, float *vec_S, const int dim_M)
{
    int iter;
    float max_offdiag;

    matrix_set_identity(mat_V, dim_M);

    iter = 0;
    while (iter++ < MAX_ITER) {
        max_offdiag = 0;

        for (int i = 0; i < (dim_M - 1); i++) {
            for (int j = (i + 1); j < dim_M; j++) {
                float cos;
                float sin;
                float tau;
                float t;

                if (i >= dim_M || j >= dim_M)
                    continue;

                if (fabs(mat[i * dim_M + j]) < EPSILON)
                    continue;

                tau = (mat[j * dim_M + j] - mat[i * dim_M + i]) / (TWO_f * mat[i * dim_M + j]);
                if (tau >= ZERO_f)
                    t = ONE_f / (tau + sqrtf(ONE_f + tau * tau));
                else
                    t = ONE_f / (tau - sqrtf(ONE_f + tau * tau));

                cos = ONE_f / sqrtf(ONE_f + t * t);
                sin = t * cos;

                /* Update rows i and j of MAT */
                for (int m = 0; m < dim_M; m++) {
                    float im;
                    float jm;

                    im = mat[i * dim_M + m];
                    jm = mat[j * dim_M + m];

                    mat[i * dim_M + m] = (cos * im) - (sin * jm);
                    mat[j * dim_M + m] = (sin * im) + (cos * jm);
                }

                /* Update cols i and j of MAT */
                for (int n = 0; n < dim_M; n++) {
                    float ni;
                    float nj;

                    ni = mat[n * dim_M + i];
                    nj = mat[n * dim_M + j];

                    mat[n * dim_M + i] = (cos * ni) - (sin * nj);
                    mat[n * dim_M + j] = (sin * ni) + (cos * nj);
                }

                /* Update cols i and j of V */
                for (int n = 0; n < dim_M; n++) {
                    float ni;
                    float nj;

                    ni = mat_V[n * dim_M + i];
                    nj = mat_V[n * dim_M + j];

                    mat_V[n * dim_M + i] = (cos * ni) - (sin * nj);
                    mat_V[n * dim_M + j] = (sin * ni) + (cos * nj);
                }

                if (fabs(mat[i * dim_M + j]) > max_offdiag)
                    max_offdiag = fabs(mat[i * dim_M + j]);
            }
        }

        if (max_offdiag < EPSILON)
            break;
    }

    /* Extract singular values as sqrtf of diag(MAT) */
    for (int i = 0; i < dim_M; i++) {
        if (mat[i * dim_M + i] > 0)
            vec_S[i] = sqrtf(mat[i * dim_M + i]);
        else
            vec_S[i] = 0;
    }

    return 0;
}

#endif  /* CLUSTER */

int linalg_svd_jacobi_impl(float *mat, float *mat_V, float *vec_S, const int dim_M)
{
    int ret;

#ifdef  CLUSTER
    ret = linalg_svd_jacobi_pulp_open_cluster(mat, mat_V, vec_S, dim_M);
#else   /* CLUSTER */
    ret = linalg_svd_jacobi_pulp_open_fc(mat, mat_V, vec_S, dim_M);
#endif  /* CLUSTER */

    return ret;
}
