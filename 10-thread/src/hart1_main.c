#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"

static void wait_go(void)
{
	for (;;) {
		/*
		 * Must invalidate: flush_range is writeback-only, so a cached
		 * go=0 would never observe CPU0's go=1 in DRAM.
		 */
		thead_flush_caches();
		if (shmem_cmd()->go)
			return;
		worker_delay(100000u);
	}
}

static void finish_batch(void)
{
	cpu1_msg("done");

	shmem_ctrl()->cpu1_done = 1;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	shmem_cmd()->go = 0;
	fence_rw();
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

/* Message-ring probe — no timer/threads (menu `t`). */
static void run_probe(uint32_t loops)
{
	uint32_t i;

	if (loops == 0)
		loops = WORKER_LOOPS_DEFAULT;

	cpu1_msg("go");
	cpu1_msg("probe");
	for (i = 1; i <= loops; i++) {
		cpu1_msg_worker(0, i, loops, i);
		worker_delay(20000000u);
		cpu1_msg_worker(1, i, loops, i);
		worker_delay(20000000u);
	}
	finish_batch();
}

static void run_batch(void)
{
	cpu1_msg("go");

	sched_init_hart();
	sched_create(worker_main, (void *)(uintptr_t)0);
	sched_create(worker_main, (void *)(uintptr_t)1);
	sched_start();

	/*
	 * Drive the scheduler with ecall yields — do not rely on wfi/MTIP/MSI
	 * delivery on the big core.
	 */
	while (!sched_all_done())
		__asm__ volatile("ecall" ::: "memory");

	sched_stop();
	finish_batch();
}

void hart1_main(void)
{
	trap_init();

	shmem_ctrl()->cpuid = bootsys_cpuid();
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	for (;;) {
		uint32_t mode;

		wait_go();

		shmem_ctrl()->go_seen = shmem_ctrl()->go_seen + 1;
		shmem_ctrl()->cpu1_done = 0;
		fence_rw();
		thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

		mode = shmem_cmd_mode();
		if (mode == CMD_MODE_PROBE)
			run_probe(shmem_cmd_loops());
		else
			run_batch();
	}
}
