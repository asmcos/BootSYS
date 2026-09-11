#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"

static uint32_t g_loops = WORKER_LOOPS_DEFAULT;

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

void worker_main(void *arg)
{
	uint32_t tid = (uint32_t)(uintptr_t)arg;
	uint32_t n;
	uint32_t max;
	uint64_t base;
	unsigned cpu = bootsys_cpuid();

	max = (cpu == 0) ? g_loops : shmem_cmd_loops();
	if (max == 0)
		max = WORKER_LOOPS_DEFAULT;

	base = clint_mtime() + (uint64_t)tid * WORKER_PHASE_MTIME;

	for (n = 1; n <= max; n++) {
		uint64_t due = base + (uint64_t)(n - 1u) * WORKER_PERIOD_MTIME;

		while (clint_mtime() < due) {
			if (cpu == 0)
				__asm__ volatile("wfi");
			else
				__asm__ volatile("ecall" ::: "memory");
		}

		print_printf("[cpu%u] T%u n=%u/%u tick=%u",
			     cpu, tid, n, max, sched_ticks());
	}
}
