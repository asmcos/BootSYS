#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "atomic.h"

static uint32_t g_loops = WORKER_LOOPS_DEFAULT;
static uint32_t g_race_mode;
static volatile uint32_t g_local_adds[2];

void worker_set_loops(uint32_t n)
{
	if (n == 0)
		n = WORKER_LOOPS_DEFAULT;
	g_loops = n;
}

uint32_t worker_get_loops(void)
{
	return g_loops;
}

void worker_delay(uint32_t loops)
{
	volatile uint32_t n = loops;

	while (n--)
		__asm__ volatile("" ::: "memory");
}

void worker_set_race_mode(uint32_t mode)
{
	g_race_mode = mode;
}

uint32_t worker_local_adds(unsigned tid)
{
	if (tid > 1u)
		return 0;
	return g_local_adds[tid];
}

/*
 * CPU0 only — two preemptible threads (arg = tid 0/1), 10-style.
 * Same L1; no D$ off; no CPU1.
 */
void worker_add(void *arg)
{
	uint32_t tid = (uint32_t)(uintptr_t)arg;
	uint32_t mode = g_race_mode;
	uint32_t n = g_loops;
	uint32_t done = 0;
	uint32_t chunk;
	uint32_t step;

	if (tid > 1u)
		tid = 0;
	if (n == 0)
		n = WORKER_LOOPS_DEFAULT;

	step = 100u;
	if (n < step)
		step = n ? n : 1u;

	g_local_adds[tid] = 0;
	print_printf("[T%u] BEGIN mode=%u N=%u sw=%u",
		     tid, mode, n, sched_switches());
	sched_yield();

	while (done < n) {
		chunk = n - done;
		if (chunk > step)
			chunk = step;
		atom_run_adds(mode, chunk);
		done += chunk;
		g_local_adds[tid] = done;
		print_printf("[T%u] progress %u/%u ctr=%u sw=%u",
			     tid, done, n, (uint32_t)atom()->counter,
			     sched_switches());
		sched_yield();
	}

	g_local_adds[tid] = done;
	print_printf("[T%u] DONE local=%u ctr=%u sw=%u",
		     tid, done, (uint32_t)atom()->counter, sched_switches());
}
