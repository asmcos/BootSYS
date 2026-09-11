/*
 * 10-thread — dual-hart timer preempt + shared print ring
 */
#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"

static int cpu1_booted;

static void banner(void)
{
	print_printf("");
	print_printf("==============================================");
	print_printf("  BootSYS  ·  10-thread");
	print_printf("  print ring; CPU0 drain in timer IRQ");
	print_printf("==============================================");
	print_printf("");
	print_drain_all();
}

static void print_help(void)
{
	print_printf("======= menu =======");
	print_printf("  t  test CPU1 print ring (probe, no CPU0 workers)");
	print_printf("  p  run batch (both CPUs → print ring)");
	print_printf("  n  set N (loops per thread)");
	print_printf("  s  status");
	print_printf("  h  help");
	print_printf("N=%u (default %u)", worker_get_loops(), WORKER_LOOPS_DEFAULT);
	print_printf("====================");
	print_drain_all();
}

static uint32_t read_uint_line(void)
{
	uint32_t v = 0;
	int got = 0;

	for (;;) {
		int c;

		do {
			c = uart_try_getc();
		} while (c < 0);

		if (c == '\r' || c == '\n') {
			uart_puts("\r\n");
			break;
		}
		if (c == 0x08 || c == 0x7f) {
			if (got) {
				v /= 10u;
				got--;
				uart_puts("\b \b");
			}
			continue;
		}
		if (c >= '0' && c <= '9') {
			uint32_t d = (uint32_t)(c - '0');

			if (v > 100000000u)
				continue;
			v = v * 10u + d;
			got++;
			uart_putc((char)c);
		}
	}
	return got ? v : 0;
}

static void set_loops_menu(void)
{
	uint32_t v;

	print_printf("loops (Enter keeps %u):", worker_get_loops());
	print_drain_all();
	v = read_uint_line();
	if (v == 0) {
		print_printf("keep N=%u", worker_get_loops());
		print_drain_all();
		return;
	}
	worker_set_loops(v);
	print_printf("N=%u", worker_get_loops());
	print_drain_all();
}

static void ensure_cpu1(void)
{
	uint32_t entry = (uint32_t)(uintptr_t)_hart1_entry;
	struct cpu1_boot_result br;
	uint32_t i;

	if (cpu1_booted)
		return;

	flush_image_cache();
	br = cpu1_boot(entry);
	if (!br.assert_ok)
		print_printf("cpu1: HW FAIL assert");
	else if (!br.released)
		print_printf("cpu1: HW FAIL release");
	print_drain_all();
	cpu1_dump_status("boot");

	for (i = 0; i < 2000u; i++) {
		thead_flush_caches();
		if (shmem_ctrl()->alive == CPU1_ALIVE_MAGIC)
			break;
		udelay(1000);
	}
	thead_flush_caches();
	print_printf("cpu1 alive=%s cpuid=%u (CSR mhartid often 0 on both)",
		     shmem_ctrl()->alive == CPU1_ALIVE_MAGIC ? "yes" : "no",
		     shmem_ctrl()->cpuid);
	print_drain_all();
	cpu1_booted = 1;
}

static void wait_cpu1_and_drain(void)
{
	uint32_t spins = 0;

	while (!shmem_cpu1_batch_done()) {
		print_drain_all();
		clint_msip_set(1, 1);
		if (++spins >= 2000000u) {
			thead_flush_caches();
			print_printf("[cpu0] timeout cpu1 go=%u done=%u go_seen=%u cpuid=%u mode=%u pend=%u",
				     shmem_cmd()->go,
				     shmem_ctrl()->cpu1_done,
				     shmem_ctrl()->go_seen,
				     shmem_ctrl()->cpuid,
				     shmem_cmd()->mode,
				     print_pending());
			print_drain_all();
			shmem_cmd_clear_go();
			break;
		}
		worker_delay(1000u);
	}
	print_drain_all();
}

static void test_cpu1_log(void)
{
	uint32_t n = worker_get_loops();

	print_printf(">>> cpu1 print test N=%u", n);
	print_drain_all();

	ensure_cpu1();
	print_init();
	shmem_cmd_kick_mode(n, CMD_MODE_PROBE);
	wait_cpu1_and_drain();
	print_printf("[cpu0] cpu1 test done, back to menu");
	print_drain_all();
}

static void start_demo(void)
{
	uint32_t n = worker_get_loops();
	uint32_t spins = 0;

	print_printf(">>> batch N=%u", n);
	print_drain_all();

	ensure_cpu1();
	print_init();
	shmem_cmd_kick(n);

	sched_init_hart();
	sched_create(worker_main, (void *)(uintptr_t)0);
	sched_create(worker_main, (void *)(uintptr_t)1);
	sched_start();

	while (!sched_all_done() || !shmem_cpu1_batch_done()) {
		print_drain_all();
		clint_msip_set(1, 1);
		if (++spins >= 500000u) {
			thead_flush_caches();
			print_printf("[cpu0] timeout cpu1 go=%u done=%u go_seen=%u cpuid=%u pend=%u",
				     shmem_cmd()->go,
				     shmem_ctrl()->cpu1_done,
				     shmem_ctrl()->go_seen,
				     shmem_ctrl()->cpuid,
				     print_pending());
			print_drain_all();
			break;
		}
		__asm__ volatile("wfi");
	}

	sched_stop();
	print_drain_all();
	print_printf("[cpu0] done, back to menu");
	print_drain_all();
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	print_init();
	sched_init_hart();
	worker_set_loops(WORKER_LOOPS_DEFAULT);
	shmem_cmd_clear_go();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	print_help();

	for (;;) {
		int c;

		print_drain_all();
		uart_puts("\r\n[M] > ");
		do {
			c = uart_try_getc();
		} while (c < 0);
		uart_putc((char)c);
		uart_puts("\r\n");

		switch (c) {
		case 't':
		case 'T':
			test_cpu1_log();
			break;
		case 'p':
		case 'P':
			start_demo();
			break;
		case 'n':
		case 'N':
			set_loops_menu();
			break;
		case 's':
		case 'S':
			print_printf("N=%u  ticks=%u  cpuid=%u  pend=%u",
				     worker_get_loops(), sched_ticks(),
				     bootsys_cpuid(), print_pending());
			print_drain_all();
			break;
		case 'h':
		case 'H':
		case '?':
			print_help();
			break;
		default:
			print_printf("unknown, press h");
			print_drain_all();
			break;
		}
	}
}
