/*
 * 10-thread — dual-hart timer preempt; CPU1 logs via shmem, CPU0 prints
 */
#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"

static int cpu1_booted;

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  10-thread                       │\r\n");
	uart_puts("│  dual-hart timer; CPU1→buffer→CPU0 print     │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======= menu =======\r\n");
	uart_puts("  t  test CPU1 log (no CPU0 workers)\r\n");
	uart_puts("  p  run batch (CPU0 UART + CPU1 log buffer)\r\n");
	uart_puts("  n  set N (loops per thread)\r\n");
	uart_puts("  s  status\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("N=");
	uart_putdec32(worker_get_loops());
	uart_puts(" (default ");
	uart_putdec32(WORKER_LOOPS_DEFAULT);
	uart_puts(")\r\n");
	uart_puts("====================\r\n");
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

	uart_puts("loops (Enter keeps ");
	uart_putdec32(worker_get_loops());
	uart_puts("): ");
	v = read_uint_line();
	if (v == 0) {
		uart_puts("keep N=");
		uart_putdec32(worker_get_loops());
		uart_puts("\r\n");
		return;
	}
	worker_set_loops(v);
	uart_puts("N=");
	uart_putdec32(worker_get_loops());
	uart_puts("\r\n");
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
		uart_puts("cpu1: HW FAIL assert\r\n");
	else if (!br.released)
		uart_puts("cpu1: HW FAIL release\r\n");
	cpu1_dump_status("boot");

	for (i = 0; i < 2000u; i++) {
		thead_flush_caches();
		if (shmem_ctrl()->alive == CPU1_ALIVE_MAGIC)
			break;
		udelay(1000);
	}
	thead_flush_caches();
	uart_puts("cpu1 alive=");
	uart_puts(shmem_ctrl()->alive == CPU1_ALIVE_MAGIC ? "yes" : "no");
	uart_puts(" cpuid=");
	uart_putdec32(shmem_ctrl()->cpuid);
	uart_puts(" (CSR mhartid often 0 on both)\r\n");
	cpu1_booted = 1;
}

static void wait_cpu1_and_drain(void)
{
	uint32_t spins = 0;

	/*
	 * Do NOT wfi here: menu `t` never enables CPU0 MTIE, so wfi would
	 * sleep forever and never hit the timeout / return to menu.
	 */
	while (!shmem_cpu1_batch_done()) {
		shmem_log_drain();
		clint_msip_set(1, 1);
		if (++spins >= 2000000u) {
			uart_puts("[cpu0] timeout cpu1; go=");
			thead_flush_caches();
			uart_putdec32(shmem_cmd()->go);
			uart_puts(" done=");
			uart_putdec32(shmem_ctrl()->cpu1_done);
			uart_puts(" go_seen=");
			uart_putdec32(shmem_ctrl()->go_seen);
			uart_puts(" cpuid=");
			uart_putdec32(shmem_ctrl()->cpuid);
			uart_puts(" mode=");
			uart_putdec32(shmem_cmd()->mode);
			uart_puts(" log_w=");
			uart_putdec32(shmem_log()->w);
			uart_puts("\r\n");
			shmem_cmd_clear_go();
			break;
		}
		worker_delay(1000u);
	}
	shmem_log_drain_all();
}

/* Probe CPU1→buffer→UART before full dual-hart batch. */
static void test_cpu1_log(void)
{
	uint32_t n = worker_get_loops();

	uart_puts("\r\n>>> cpu1 log test N=");
	uart_putdec32(n);
	uart_puts("\r\n");

	ensure_cpu1();
	shmem_log_reset();
	shmem_cmd_kick_mode(n, CMD_MODE_PROBE);
	wait_cpu1_and_drain();
	uart_puts("[cpu0] cpu1 test done, back to menu\r\n");
}

static void start_demo(void)
{
	uint32_t n = worker_get_loops();

	uart_puts("\r\n>>> batch N=");
	uart_putdec32(n);
	uart_puts("\r\n");

	ensure_cpu1();
	shmem_log_reset();
	shmem_cmd_kick(n);

	sched_init_hart();
	sched_create(worker_main, (void *)(uintptr_t)0);
	sched_create(worker_main, (void *)(uintptr_t)1);
	sched_start();

	{
		uint32_t spins = 0;

		while (!sched_all_done() || !shmem_cpu1_batch_done()) {
			shmem_log_drain();
			clint_msip_set(1, 1);
			if (++spins >= 500000u) {
				uart_puts("[cpu0] timeout cpu1; go=");
				thead_flush_caches();
				uart_putdec32(shmem_cmd()->go);
				uart_puts(" done=");
				uart_putdec32(shmem_ctrl()->cpu1_done);
				uart_puts(" go_seen=");
				uart_putdec32(shmem_ctrl()->go_seen);
				uart_puts(" cpuid=");
				uart_putdec32(shmem_ctrl()->cpuid);
				uart_puts(" log_w=");
				uart_putdec32(shmem_log()->w);
				uart_puts("\r\n");
				break;
			}
			__asm__ volatile("wfi");
		}
	}

	sched_stop();
	shmem_log_drain_all();
	uart_puts("[cpu0] done, back to menu\r\n");
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	sched_init_hart();
	worker_set_loops(WORKER_LOOPS_DEFAULT);
	shmem_cmd_clear_go();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	print_help();

	for (;;) {
		int c;

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
			uart_puts("N=");
			uart_putdec32(worker_get_loops());
			uart_puts("  ticks=");
			uart_putdec32(sched_ticks());
			uart_puts("  cpuid=");
			uart_putdec32(bootsys_cpuid());
			uart_puts("\r\n");
			break;
		case 'h':
		case 'H':
		case '?':
			print_help();
			break;
		default:
			uart_puts("unknown, press h\r\n");
			break;
		}
	}
}
