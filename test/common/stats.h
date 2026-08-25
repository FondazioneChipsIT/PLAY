#ifndef STATS_H_
#define STATS_H_

#ifdef  STATS

#if TARGET_IS_SPATZ

#define HOTTING     (0)
#define REPEAT      (2)

#include "snrt.h"

void print_stats(unsigned long _cycles);

#define INIT_STATS()                    \
    unsigned long _start_cycles = 0;    \
    unsigned long _end_cycles   = 0;    \
    unsigned long _cycles   = 0;        \

#define START_LOOP_STATS()                              \
    for (int _k = 0; _k < (HOTTING + REPEAT); _k++) {   \

#define START_STATS()                       \
        _start_cycles = read_csr(mcycle);   \

#define STOP_STATS()                        \
        _end_cycles = read_csr(mcycle);     \

#define END_LOOP_STATS()                            \
        if (_k == HOTTING + REPEAT - 1) {           \
            _cycles = _end_cycles - _start_cycles;  \
            print_stats(_cycles);                   \
        }                                           \
    }                                               \

#elif TARGET_IS_PULP_OPEN

#define HOTTING     (2)
#define REPEAT      (1)

#include "pmsis.h"


void print_stats(unsigned long _cycles, unsigned long _active, unsigned long _instr, unsigned long _ldstall,
                unsigned long _jrstall, unsigned long _imiss, unsigned long _ld, unsigned long _st,
                unsigned long _jump, unsigned long _branch, unsigned long _btaken, unsigned long _rvc,
                unsigned long _ld_ext, unsigned long _st_ext, unsigned long _ld_ext_cyc, unsigned long _st_ext_cyc,
                unsigned long _tcdm_cont);


#define EVENTS_BASE (1<<PI_PERF_CYCLES)     | (1<<PI_PERF_ACTIVE_CYCLES)    | (1<<PI_PERF_INSTR)    |   \
                    (1<<PI_PERF_LD_STALL)   | (1<<PI_PERF_JR_STALL)         | (1<<PI_PERF_IMISS)    |   \
                    (1<<PI_PERF_LD)         | (1<<PI_PERF_ST)               | (1<<PI_PERF_JUMP)     |   \
                    (1<<PI_PERF_BRANCH)     | (1<<PI_PERF_BTAKEN)           | (1<<PI_PERF_RVC)

#ifdef  CLUSTER

#define EXTENDED_STATS  (1)
#define EVENTS          (EVENTS_BASE | (1<<PI_PERF_LD_EXT) | (1<<PI_PERF_ST_EXT) | (1<<PI_PERF_LD_EXT_CYC) | (1<<PI_PERF_ST_EXT_CYC) | (1<<PI_PERF_TCDM_CONT))

#else   /* CLUSTER */

#define EXTENDED_STATS  (0)
#define EVENTS          EVENTS_BASE

#endif  /* CLUSTER */

#define INIT_STATS()                    \
    unsigned long _cycles       = 0;    \
    unsigned long _active       = 0;    \
    unsigned long _instr        = 0;    \
    unsigned long _ldstall      = 0;    \
    unsigned long _jrstall      = 0;    \
    unsigned long _imiss        = 0;    \
    unsigned long _ld           = 0;    \
    unsigned long _st           = 0;    \
    unsigned long _jump         = 0;    \
    unsigned long _branch       = 0;    \
    unsigned long _btaken       = 0;    \
    unsigned long _rvc          = 0;    \
    unsigned long _ld_ext       = 0;    \
    unsigned long _st_ext       = 0;    \
    unsigned long _ld_ext_cyc   = 0;    \
    unsigned long _st_ext_cyc   = 0;    \
    unsigned long _tcdm_cont    = 0;    \

#define START_LOOP_STATS()                              \
    for (int _k = 0; _k < (HOTTING + REPEAT); _k++) {   \
        pi_perf_conf(EVENTS);                           \

#define START_STATS()       \
        pi_perf_reset();    \
        pi_perf_start();    \

#define STOP_STATS()                                                \
        pi_perf_stop();                                             \
        if ( _k >= HOTTING) {                                       \
            _cycles     += pi_perf_read(PI_PERF_CYCLES);            \
            _active     += pi_perf_read(PI_PERF_ACTIVE_CYCLES);     \
            _instr      += pi_perf_read(PI_PERF_INSTR);             \
            _ldstall    += pi_perf_read(PI_PERF_LD_STALL);          \
            _jrstall    += pi_perf_read(PI_PERF_JR_STALL);          \
            _imiss      += pi_perf_read(PI_PERF_IMISS);             \
            _ld         += pi_perf_read(PI_PERF_LD);                \
            _st         += pi_perf_read(PI_PERF_ST);                \
            _jump       += pi_perf_read(PI_PERF_JUMP);              \
            _branch     += pi_perf_read(PI_PERF_BRANCH);            \
            _btaken     += pi_perf_read(PI_PERF_BTAKEN);            \
            _rvc        += pi_perf_read(PI_PERF_RVC);               \
            if (EXTENDED_STATS) {                                   \
                _ld_ext     += pi_perf_read(PI_PERF_LD_EXT);        \
                _st_ext     += pi_perf_read(PI_PERF_ST_EXT);        \
                _ld_ext_cyc += pi_perf_read(PI_PERF_LD_EXT_CYC);    \
                _st_ext_cyc += pi_perf_read(PI_PERF_ST_EXT_CYC);    \
                _tcdm_cont  += pi_perf_read(PI_PERF_TCDM_CONT);     \
            }                                                       \
        }                                                           \

#define END_LOOP_STATS()                                            \
        if (_k == HOTTING + REPEAT - 1) {                           \
            print_stats(_cycles, _active, _instr, _ldstall,         \
                        _jrstall, _imiss, _ld, _st, _jump,          \
                        _branch, _btaken, _rvc, _ld_ext, _st_ext,   \
                        _ld_ext_cyc, _st_ext_cyc, _tcdm_cont);      \
        }                                                           \
    }

#endif  /* TARGET_IS_ */

#else   /* STATS */

#define INIT_STATS()
#define START_LOOP_STATS()
#define START_STATS()
#define STOP_STATS()
#define END_LOOP_STATS()

#endif  /* STATS */

#endif  /* STATS_H_ */
