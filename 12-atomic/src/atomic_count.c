#include "k230.h"
#include "atomic.h"
#include "shmem_log.h"
#include "thread.h"

#ifndef ATOM_ADD_BUSY_BASE
#define ATOM_ADD_BUSY_BASE 2000u
#endif
#ifndef ATOM_ADD_BUSY_SPAN
#define ATOM_ADD_BUSY_SPAN 60000u
#endif

static uint32_t g_rng = 0xA5A5u;

static void push_cmd(void)
{
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

void atom_rng_seed(uint32_t seed)
{
	g_rng = seed ? seed : 0xA5A5u;
}

static uint32_t rnd(void)
{
	uint32_t x = g_rng;

	/* xorshift32 */
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	g_rng = x ? x : 0xA5A5u;
	return g_rng;
}

static void delay_random(void)
{
	volatile uint32_t n = ATOM_ADD_BUSY_BASE + (rnd() % ATOM_ADD_BUSY_SPAN);

	while (n--)
		__asm__ volatile("" ::: "memory");
}

/*
 * Intentional non-atomic RMW for menu 't'.
 * Random busy length + occasional mid-RMW yield → less regular loss.
 */
void atom_add_plain(volatile uint64_t *p)
{
	uint64_t v;

	v = *p;
	delay_random();
	/* ~1/32 chance to switch between ld and sd */
	if ((rnd() & 31u) == 0u)
		sched_yield();
	v += 1;
	*p = v;
}

void atom_add_locked(volatile uint64_t *lock, volatile uint64_t *p)
{
	spin_lock(lock);
	*p = *p + 1;
	spin_unlock(lock);
}

void atom_add_amo(volatile uint64_t *p)
{
	__asm__ volatile("amoadd.d zero, %1, (%0)"
			 :
			 : "r"(p), "r"(1ULL)
			 : "memory");
}

void atom_run_adds(uint32_t mode, uint32_t n)
{
	volatile struct atom_arena *a = atom();
	uint32_t i;

	for (i = 0; i < n; i++) {
		if (mode == CMD_MODE_LOCK)
			atom_add_locked(&a->lock, &a->counter);
		else if (mode == CMD_MODE_AMO)
			atom_add_amo(&a->counter);
		else
			atom_add_plain(&a->counter);
		/* After full RMW: random-ish yield (~1/8) to interleave progress. */
		if ((rnd() & 7u) == 0u)
			sched_yield();
	}
}

void atom_flush(void)
{
	thead_flush_range(ATOM_BASE, ATOM_BASE + ATOM_BYTES);
}

void atom_reset(void)
{
	volatile struct atom_arena *a = atom();

	a->lock = 0;
	a->counter = 0;
	a->reserved = 0;
	fence_rw();
	atom_flush();
	thead_flush_caches();
}

static int wait_armed(uint32_t tries)
{
	uint32_t i;

	for (i = 0; i < tries; i++) {
		thead_flush_caches();
		if (shmem_ctrl()->atom_armed)
			return 1;
	}
	return 0;
}

int atom_sc_fails_after_foreign_store(void)
{
	volatile struct atom_arena *a = atom();
	volatile struct shmem_cmd *c = shmem_cmd();
	uint64_t tmp, st;
	uint32_t i;
	uint32_t seen;

	atom_reset();
	thead_flush_caches();
	seen = shmem_ctrl()->go_seen;

	shmem_cmd_kick_mode(0, CMD_MODE_RESERVE);

	for (i = 0; i < 2000000u; i++) {
		thead_flush_caches();
		if (shmem_ctrl()->go_seen != seen)
			break;
	}
	if (shmem_ctrl()->go_seen == seen) {
		shmem_cmd_clear_go();
		return -1;
	}

	if (!wait_armed(2000000u)) {
		shmem_cmd_clear_go();
		return -1;
	}

	__asm__ volatile("lr.d %0, (%1)"
			 : "=&r"(tmp)
			 : "r"(&a->reserved)
			 : "memory");

	c->poke = 1;
	fence_rw();
	push_cmd();
	thead_flush_caches();

	for (i = 0; i < 2000000u; i++) {
		if (shmem_cpu1_batch_done())
			break;
	}
	if (!shmem_cpu1_batch_done()) {
		__asm__ volatile("sc.d %0, zero, (%1)"
				 : "=&r"(st)
				 : "r"(&a->reserved)
				 : "memory");
		(void)st;
		shmem_cmd_clear_go();
		return -1;
	}

	__asm__ volatile("sc.d %0, %1, (%2)"
			 : "=&r"(st)
			 : "r"(1ULL), "r"(&a->reserved)
			 : "memory");
	return st != 0;
}
