#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include "trap.h"

#define HART_MAX           2
#define THREAD_PER_HART    2
#define THREAD_STACK_SIZE  2048

enum thread_state {
	THREAD_UNUSED = 0,
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_DONE,
};

/*
 * Full integer context for timer preemption.
 * Matches trap_frame GPR layout + mepc (saved beside it in struct thread).
 */
struct thread {
	struct trap_frame frame;
	uint64_t mepc;
	enum thread_state state;
	int id; /* 0..THREAD_PER_HART-1 on this hart */
	void (*entry)(void *arg);
	void *arg;
	uint64_t stack[THREAD_STACK_SIZE / sizeof(uint64_t)];
};

void sched_init_hart(void);
int sched_create(void (*entry)(void *), void *arg);
void sched_start(void); /* enable MTIE + MIE on this hart; current = idle */
void sched_stop(void);  /* park timer, clear enabled */
void sched_on_timer(struct trap_frame *tf);
void sched_mark_done(void); /* current thread finished */
int sched_all_done(void);   /* every created thread is DONE */
void sched_yield(void);     /* ecall → reschedule */

int sched_current_id(void);
uint32_t sched_ticks(void);
uint32_t sched_switches(void); /* # of thread/idle switches this run */

#endif
