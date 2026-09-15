#include "k230.h"
#include "atomic.h"
#include "thread.h"

/*
 * Same-hart: amoswap TAS. While waiting, yield so the lock holder can run
 * (UP + preemption: spinning without yield deadlocks if holder is READY).
 */
void spin_lock(volatile uint64_t *lock)
{
	uint64_t tmp;

	for (;;) {
		__asm__ volatile("amoswap.d.aq %0, %1, (%2)"
				 : "=r"(tmp)
				 : "r"(1ULL), "r"(lock)
				 : "memory");
		if (tmp == 0)
			return;
		sched_yield();
	}
}

void spin_unlock(volatile uint64_t *lock)
{
	__asm__ volatile("amoswap.d.rl zero, zero, (%0)"
			 :
			 : "r"(lock)
			 : "memory");
}
