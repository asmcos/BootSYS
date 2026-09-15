#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "atomic.h"

static void wait_go(void)
{
	for (;;) {
		thead_flush_caches();
		if (shmem_cmd()->go)
			return;
		worker_delay(100000u);
	}
}

static void finish_batch(void)
{
	print_printf("[cpu%u] done", bootsys_cpuid());

	shmem_ctrl()->cpu1_done = 1;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	shmem_cmd()->go = 0;
	shmem_cmd()->poke = 0;
	shmem_cmd()->start = 0;
	fence_rw();
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

static void run_probe(uint32_t loops)
{
	uint32_t i;

	if (loops == 0)
		loops = 5u;
	print_printf("[cpu%u] go probe", bootsys_cpuid());
	for (i = 1; i <= loops; i++) {
		print_printf("[cpu%u] probe n=%u/%u", bootsys_cpuid(), i, loops);
		worker_delay(15000000u);
	}
	finish_batch();
}

/* One thread on this hart — unused for t/l/a (those are CPU0-only now). */
static void run_adds_threaded(uint32_t mode)
{
	(void)mode;
	print_printf("[cpu%u] skip add (CPU0-only race)", bootsys_cpuid());
	finish_batch();
}

static void run_reserve(void)
{
	volatile struct atom_arena *a = atom();
	volatile struct shmem_cmd *c = shmem_cmd();

	print_printf("[cpu%u] go reserve", bootsys_cpuid());

	shmem_ctrl()->atom_armed = 1;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	while (!c->poke) {
		thead_flush_caches();
		worker_delay(100u);
	}

	a->reserved = 1;
	fence_rw();
	atom_flush();
	finish_batch();
}

void hart1_main(void)
{
	trap_init();

	shmem_ctrl()->cpuid = bootsys_cpuid();
	shmem_ctrl()->atom_armed = 0;
	shmem_ctrl()->cpu1_done = 0;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	print_printf("[cpu%u] hart1 ready", bootsys_cpuid());

	for (;;) {
		uint32_t mode;

		wait_go();
		shmem_ctrl()->go_seen = shmem_ctrl()->go_seen + 1;
		shmem_ctrl()->cpu1_done = 0;
		shmem_ctrl()->atom_armed = 0;
		shmem_ctrl()->epoch_ack = 0;
		shmem_ctrl()->add_done = 0;
		shmem_ctrl()->local_adds = 0;
		fence_rw();
		thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

		mode = shmem_cmd_mode();
		if (mode == CMD_MODE_PROBE)
			run_probe(shmem_cmd_loops());
		else if (mode == CMD_MODE_PLAIN || mode == CMD_MODE_LOCK ||
			 mode == CMD_MODE_AMO)
			run_adds_threaded(mode);
		else if (mode == CMD_MODE_RESERVE)
			run_reserve();
		else
			finish_batch();
	}
}
