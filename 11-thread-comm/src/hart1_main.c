#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "mbox.h"
#include "cache_probe.h"

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
	print_printf("[cpu1] done");

	shmem_ctrl()->cpu1_done = 1;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	shmem_cmd()->go = 0;
	fence_rw();
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

static void run_probe(uint32_t loops)
{
	uint32_t i;

	if (loops == 0)
		loops = WORKER_LOOPS_DEFAULT;
	print_printf("[cpu1] go");
	print_printf("[cpu1] probe");
	for (i = 1; i <= loops; i++) {
		print_printf("[cpu1] T0 n=%u/%u", i, loops);
		worker_delay(15000000u);
		print_printf("[cpu1] T1 n=%u/%u", i, loops);
		worker_delay(15000000u);
	}
	finish_batch();
}

static void run_recv(int cohere)
{
	print_printf("[cpu1] go recv cohere=%d", cohere);
	sched_init_hart();
	sched_create(worker_receiver, (void *)(uintptr_t)cohere);
	sched_start();
	while (!sched_all_done())
		__asm__ volatile("ecall" ::: "memory");
	sched_stop();
	finish_batch();
}

static void run_pong(void)
{
	print_printf("[cpu1] go pong");
	sched_init_hart();
	sched_create(worker_pong_c1, (void *)0);
	sched_start();
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
		shmem_ctrl()->rx_ok = 0;
		shmem_ctrl()->rx_bad = 0;
		fence_rw();
		thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

		mode = shmem_cmd_mode();
		print_printf("[cpu1] go mode=%u", mode);
		if (mode == CMD_MODE_PROBE)
			run_probe(shmem_cmd_loops());
		else if (mode == CMD_MODE_MBOX_OK)
			run_recv(1);
		else if (mode == CMD_MODE_MBOX_BAD)
			run_recv(0);
		else if (mode == CMD_MODE_PINGPONG)
			run_pong();
		else if (mode == CMD_MODE_CACHE_PROBE) {
			cache_probe_run_cpu1();
			finish_batch();
		} else {
			print_printf("[cpu1] unknown mode=%u", mode);
			finish_batch();
		}
	}
}
