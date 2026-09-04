#ifdef STATS

#include "stats.h"

#include "utils.h"

#include <stdio.h>

#if TARGET_IS_SPATZ

#include "printf.h"

void print_stats(unsigned long _cycles)
{
    int id;

    id = snrt_cluster_core_idx();
    if (id == 0)
        printf("INFO | Printing statistics:\n");

    barrier();

    for (int i = 0; i < NUM_CC; i++) {
        if (id == i) {
            printf("[%d] cycles:\t%lu\n", id, _cycles);
        }
        barrier();
    }

}

#elif TARGET_IS_PULP_OPEN

void print_stats(unsigned long _cycles, unsigned long _instr,
                unsigned long _ldstall, unsigned long _jrstall, unsigned long _imiss,
                unsigned long _ld, unsigned long _st, unsigned long _jump,
                unsigned long _branch, unsigned long _btaken, unsigned long _rvc)
{
    int id;

    id = pi_core_id();
    if (id == 0)
        printf("INFO | Printing statistics:\n");

    barrier();

    printf("[%d] cycles:\t\t\t%lu\n", id, _cycles/REPEAT);
    printf("[%d] instr execd:\t\t%lu\n", id, _instr/REPEAT);
    printf("[%d] ld stall:\t\t\t%lu\n", id, _ldstall/REPEAT);
    printf("[%d] jr stall:\t\t\t%lu\n", id, _jrstall/REPEAT);
    printf("[%d] imiss:\t\t\t%lu\n", id, _imiss/REPEAT);
    printf("[%d] ld:\t\t\t\t%lu\n", id, _ld/REPEAT);
    printf("[%d] st:\t\t\t\t%lu\n", id, _st/REPEAT);
    printf("[%d] jump:\t\t\t%lu\n", id, _jump/REPEAT);
    printf("[%d] branch:\t\t\t%lu\n", id, _branch/REPEAT);
    printf("[%d] btaken:\t\t\t%lu\n", id, _btaken/REPEAT);
    printf("[%d] rvc:\t\t\t%lu\n", id, _rvc/REPEAT);
}

#endif  /* TARGET_IS_ */

#endif  /* STATS */
