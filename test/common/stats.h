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
#define REPEAT      (5)

#include "pmsis.h"


static inline void perf_csr_enable(void)
{
    /* clear CY (bit 0) and IR (bit 2) -> minstret runs */
    asm volatile("csrrci zero, 0x320, 0x5" ::: "memory");
}

static inline void perf_csr_disable(void)
{
    /* set CY (bit 0) and IR (bit 2) -> minstret frozen */
    asm volatile("csrrsi zero, 0x320, 0x5" ::: "memory");
}

static inline unsigned long perf_csr_instret(void)
{
    unsigned long v;
    asm volatile("csrr %0, 0xB02" : "=r"(v) :: "memory");
    return v;
}

static inline unsigned long perf_csr_cycle(void)
{
    unsigned long v;
    asm volatile("csrr %0, 0xB00" : "=r"(v) :: "memory");
    return v;
}



void print_stats(unsigned long _cycles, unsigned long _instr, unsigned long _ldstall,
                unsigned long _jrstall, unsigned long _imiss, unsigned long _ld, unsigned long _st,
                unsigned long _jump, unsigned long _branch, unsigned long _btaken, unsigned long _rvc);


#define DECL_MHPMCTR_PLAY(NAME, EVENT_CSR, COUNTER_CSR, SEL_VAL)               \
    static inline void hpmevent_set_##NAME(void) {                            \
        asm volatile("csrw " #EVENT_CSR ", %0" :: "r"((unsigned)(SEL_VAL)));  \
    }                                                                         \
    static inline void hpmcounter_reset_##NAME(void) {                       \
        asm volatile("csrw " #COUNTER_CSR ", zero");                         \
    }                                                                         \
    static inline unsigned long hpmcounter_get_##NAME(void) {                \
        unsigned long v;                                                     \
        asm volatile("csrr %0, " #COUNTER_CSR : "=r"(v));                    \
        return v;                                                            \
    }

DECL_MHPMCTR_PLAY(ld_stall, 0x325, 0xB05, (1u << 2))
DECL_MHPMCTR_PLAY(jr_stall, 0x326, 0xB06, (1u << 3))
DECL_MHPMCTR_PLAY(imiss,    0x327, 0xB07, (1u << 4))
DECL_MHPMCTR_PLAY(ld,       0x328, 0xB08, (1u << 5))
DECL_MHPMCTR_PLAY(st,       0x329, 0xB09, (1u << 6))
DECL_MHPMCTR_PLAY(jump,     0x32A, 0xB0A, (1u << 7))
DECL_MHPMCTR_PLAY(branch,   0x32B, 0xB0B, (1u << 8))
DECL_MHPMCTR_PLAY(btaken,   0x32C, 0xB0C, (1u << 9))
DECL_MHPMCTR_PLAY(rvc,      0x32D, 0xB0D, (1u << 10))
#undef DECL_MHPMCTR_PLAY

static inline void hpmevent_configure_all(void) {
    hpmevent_set_ld_stall(); hpmevent_set_jr_stall(); hpmevent_set_imiss();
    hpmevent_set_ld();       hpmevent_set_st();       hpmevent_set_jump();
    hpmevent_set_branch();   hpmevent_set_btaken();   hpmevent_set_rvc();
}

static inline void hpmcounter_reset_all(void) {
    hpmcounter_reset_ld_stall(); hpmcounter_reset_jr_stall(); hpmcounter_reset_imiss();
    hpmcounter_reset_ld();       hpmcounter_reset_st();       hpmcounter_reset_jump();
    hpmcounter_reset_branch();   hpmcounter_reset_btaken();   hpmcounter_reset_rvc();
}

/* mcountinhibit bits 5..13 = mhpmcounter5..mhpmcounter13 (0xB05..0xB0D above). */
#define MHPMCTR_INHIBIT_MASK (0x3FE0u)

static inline void hpmcount_ext_en(void) {
    unsigned mask = MHPMCTR_INHIBIT_MASK;
    asm volatile("csrrc zero, 0x320, %0" :: "r"(mask));
}

static inline void hpmcount_ext_dis(void) {
    unsigned mask = MHPMCTR_INHIBIT_MASK;
    asm volatile("csrrs zero, 0x320, %0" :: "r"(mask));
}

#define INIT_STATS()                    \
    unsigned long _cycles       = 0;    \
    unsigned long _start_cycles = 0;    \
    unsigned long _end_cycles   = 0;    \
    unsigned long _instr        = 0;    \
    unsigned long _start_instr  = 0;    \
    unsigned long _end_instr    = 0;    \
    unsigned long _ldstall      = 0;    \
    unsigned long _jrstall      = 0;    \
    unsigned long _imiss        = 0;    \
    unsigned long _ld           = 0;    \
    unsigned long _st           = 0;    \
    unsigned long _jump         = 0;    \
    unsigned long _branch       = 0;    \
    unsigned long _btaken       = 0;    \
    unsigned long _rvc          = 0;    \

#define START_LOOP_STATS()                              \
    for (int _k = 0; _k < (HOTTING + REPEAT); _k++) {   \
        hpmevent_configure_all();                       \

#define START_STATS()               \
        hpmcounter_reset_all();     \
        hpmcount_ext_en();          \
        perf_csr_enable();          \
        _start_instr = perf_csr_instret();   \
        _start_cycles = perf_csr_cycle();    \

#define STOP_STATS()                                                \
        _end_cycles = perf_csr_cycle();                             \
        _end_instr = perf_csr_instret();                            \
        perf_csr_disable();                                         \
        hpmcount_ext_dis();                                         \
        if ( _k >= HOTTING) {                                       \
            _cycles     += _end_cycles - _start_cycles;             \
            _instr      += _end_instr - _start_instr;               \
            _ldstall    += hpmcounter_get_ld_stall();                \
            _jrstall    += hpmcounter_get_jr_stall();                \
            _imiss      += hpmcounter_get_imiss();                   \
            _ld         += hpmcounter_get_ld();                      \
            _st         += hpmcounter_get_st();                      \
            _jump       += hpmcounter_get_jump();                    \
            _branch     += hpmcounter_get_branch();                  \
            _btaken     += hpmcounter_get_btaken();                  \
            _rvc        += hpmcounter_get_rvc();                     \
        }                                                           \

#define END_LOOP_STATS()                                            \
        if (_k == HOTTING + REPEAT - 1) {                           \
            print_stats(_cycles, _instr, _ldstall,                  \
                        _jrstall, _imiss, _ld, _st, _jump,          \
                        _branch, _btaken, _rvc);                    \
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
