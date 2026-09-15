/*
 * 11-thread-comm — dual-hart mailbox + shared print ring
 */
#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "mbox.h"
#include "cache_probe.h"

static int cpu1_booted;

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

static void banner(void)
{
	/* Direct UART — print ring truncates UTF-8 box drawing. */
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  11-thread-comm                  │\r\n");
	uart_puts("│  mailbox + flush/pull; print ring on CPU0    │\r\n");
	uart_puts("├──────────────────────────────────────────────┤\r\n");
	uart_puts("│  build   " __DATE__ " " __TIME__ "                │\r\n");
	uart_puts("│  git     " BOOTSYS_GIT "                             │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	print_printf("======= menu =======");
	print_printf("  t  print probe (CPU1 → print ring)");
	print_printf("  g  mailbox GOOD (flush+pull)");
	print_printf("  b  mailbox BAD  (no cohere) — expect loss/stale");
	print_printf("  p  ping-pong (CPU0↔CPU1)");
	print_printf("  c  cache probe (cpu0 write, cpu1 see?)");
	print_printf("  n  set N");
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

static int ensure_cpu1(void)
{
	uint32_t entry = (uint32_t)(uintptr_t)_hart1_entry;
	struct cpu1_boot_result br;
	uint32_t i;
	int alive;

	if (cpu1_booted) {
		thead_flush_caches();
		if (shmem_ctrl()->alive == CPU1_ALIVE_MAGIC)
			return 1;
		print_printf("cpu1 was booted but alive lost — re-boot");
		print_drain_all();
		cpu1_booted = 0;
	}

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
	alive = (shmem_ctrl()->alive == CPU1_ALIVE_MAGIC);
	print_printf("cpu1 alive=%s cpuid=%u",
		     alive ? "yes" : "no", shmem_ctrl()->cpuid);
	print_drain_all();
	if (!alive)
		return 0;
	cpu1_booted = 1;
	return 1;
}

static void wait_cpu1_and_drain(void)
{
	uint32_t spins = 0;

	while (!shmem_cpu1_batch_done()) {
		print_drain_all();
		clint_msip_set(1, 1);
		if (++spins >= 3000000u) {
			thead_flush_caches();
			print_printf("[cpu0] timeout go=%u done=%u seen=%u rx_ok=%u",
				     shmem_cmd()->go, shmem_ctrl()->cpu1_done,
				     shmem_ctrl()->go_seen, shmem_ctrl()->rx_ok);
			print_drain_all();
			shmem_cmd_clear_go();
			break;
		}
		worker_delay(1000u);
	}
	print_drain_all();
}

static void run_cpu0_sender(int cohere, uint32_t mode)
{
	uint32_t n = worker_get_loops();
	uint32_t spins = 0;

	print_printf(">>> mbox %s N=%u", cohere ? "GOOD" : "BAD", n);
	print_drain_all();

	if (!ensure_cpu1())
		return;
	print_init();
	mbox_init();
	shmem_cmd_kick_mode(n, mode);

	sched_init_hart();
	sched_create(worker_sender, (void *)(uintptr_t)cohere);
	sched_start();

	while (!sched_all_done() || !shmem_cpu1_batch_done()) {
		print_drain_all();
		clint_msip_set(1, 1);
		if (++spins >= 500000u) {
			print_printf("[cpu0] timeout");
			print_drain_all();
			break;
		}
		__asm__ volatile("wfi");
	}
	sched_stop();
	print_drain_all();
	thead_flush_caches();
	print_printf("[cpu0] summary rx_ok=%u rx_bad=%u drops=%u",
		     shmem_ctrl()->rx_ok, shmem_ctrl()->rx_bad,
		     mbox_pair()->c0_to_c1.drops);
	print_printf("[cpu0] back to menu");
	print_drain_all();
}

static void run_pingpong(void)
{
	uint32_t n = worker_get_loops();
	uint32_t spins = 0;

	print_printf(">>> ping-pong N=%u", n);
	print_drain_all();

	if (!ensure_cpu1())
		return;
	print_init();
	mbox_init();
	shmem_cmd_kick_mode(n, CMD_MODE_PINGPONG);

	sched_init_hart();
	sched_create(worker_ping_c0, (void *)0);
	sched_start();

	while (!sched_all_done() || !shmem_cpu1_batch_done()) {
		print_drain_all();
		clint_msip_set(1, 1);
		if (++spins >= 500000u) {
			print_printf("[cpu0] ping-pong timeout");
			print_drain_all();
			break;
		}
		__asm__ volatile("wfi");
	}
	sched_stop();
	print_drain_all();
	print_printf("[cpu0] back to menu");
	print_drain_all();
}

static void test_print_probe(void)
{
	uint32_t n = worker_get_loops();

	print_printf(">>> print probe N=%u", n);
	print_drain_all();
	if (!ensure_cpu1())
		return;
	print_init();
	shmem_cmd_kick_mode(n, CMD_MODE_PROBE);
	wait_cpu1_and_drain();
	print_printf("[cpu0] probe done");
	print_drain_all();
}

static void test_cache_probe(void)
{
	print_printf(">>> cache probe");
	print_drain_all();

	if (!ensure_cpu1())
		return;

	/* print_init wipes the ring — drain alive line first (already done). */
	print_init();

	shmem_ctrl()->cpu1_done = 0;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	shmem_cmd_kick_mode(1, CMD_MODE_CACHE_PROBE);
	clint_msip_set(1, 1);

	/*
	 * Do not wait on go_seen alone — phase handshake inside the probe
	 * is enough. CPU0 and CPU1 run the probe concurrently.
	 */
	cache_probe_run_cpu0();
	wait_cpu1_and_drain();
	print_printf("[cpu0] back to menu");
	print_drain_all();
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	print_init();
	mbox_init();
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
			test_print_probe();
			break;
		case 'g':
		case 'G':
			run_cpu0_sender(1, CMD_MODE_MBOX_OK);
			break;
		case 'b':
		case 'B':
			run_cpu0_sender(0, CMD_MODE_MBOX_BAD);
			break;
		case 'p':
		case 'P':
			run_pingpong();
			break;
		case 'c':
		case 'C':
			test_cache_probe();
			break;
		case 'n':
		case 'N':
			set_loops_menu();
			break;
		case 's':
		case 'S':
			thead_flush_caches();
			print_printf("N=%u ticks=%u cpuid=%u rx_ok=%u drops=%u",
				     worker_get_loops(), sched_ticks(),
				     bootsys_cpuid(), shmem_ctrl()->rx_ok,
				     mbox_pair()->c0_to_c1.drops);
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
