#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

/* Saved frame layout must match trap.S */
struct trap_frame {
	uint64_t ra, t0, t1, t2, s0, s1;
	uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
	uint64_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
	uint64_t t3, t4, t5, t6, gp, tp;
	uint64_t sp;
};

void trap_init(void);
void trap_handler(struct trap_frame *tf);

/* After a handled sync exception, skip the faulting insn and continue. */
extern volatile int trap_skip_insn;

#endif
