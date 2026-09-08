#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

struct trap_frame {
	uint64_t ra, t0, t1, t2, s0, s1;
	uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
	uint64_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
	uint64_t t3, t4, t5, t6, gp, tp;
	uint64_t sp;
};

void trap_init(void);
void trap_handler(struct trap_frame *tf);

/* 访问测试期间置 1：打印简要异常，跳过指令，mret 回原特权级 */
extern volatile int trap_expect;
extern volatile int trap_quiet; /* trap_expect but do not print */
extern volatile int trap_saw;
extern volatile uint64_t trap_last_cause;
extern volatile uint64_t trap_last_mpp;

#endif
