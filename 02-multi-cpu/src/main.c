/* BootSYS K230 — CPU0 bring-up + wake research menu. */
#include "k230.h"
#include "memmap.h"
#include "banner.h"

static void uart_rx_drain(void)
{
	while (uart_try_getc() >= 0)
		;
}

static int wait_cpu1_alive(uint32_t timeout_ms)
{
	for (uint32_t i = 0; i < timeout_ms; i++) {
		/* Drop stale D-cache line after CPU1 write. */
		thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
		if (shmem_cpu1_alive())
			return 1;
		udelay(1000);
	}
	return 0;
}

static void print_sw_status(void)
{
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
	uart_puts("cpu1: alive=");
	uart_puts(shmem_cpu1_alive() ? "yes" : "no");
	uart_puts(" state=");
	uart_puts(shmem_state_name(shmem_cpu1_state()));
	uart_puts(" wake=");
	uart_putdec32(cpu1_shmem->wake_count);
	uart_puts(" ipi=");
	uart_putdec32(cpu1_shmem->ipi_seen);
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n");
	uart_puts("======= menu =======\r\n");
	uart_puts("  s  status\r\n");
	uart_puts("  1  run\r\n");
	uart_puts("  2  soft-loop\r\n");
	uart_puts("  3  wfi\r\n");
	uart_puts("  i  ipi probe\r\n");
	uart_puts("  c  clear msip\r\n");
	uart_puts("  q  park (J-Link)\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("====================\r\n");
}

static void report_boot_result(const struct cpu1_boot_result *r)
{
	if (!r->assert_ok)
		uart_puts("cpu1: HW FAIL assert\r\n");
	else if (!r->released)
		uart_puts("cpu1: HW FAIL release\r\n");
}

static void ipi_probe(void)
{
	uint32_t wake0, ipi0;

	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
	wake0 = cpu1_shmem->wake_count;
	ipi0 = cpu1_shmem->ipi_seen;

	if (cpu1_is_in_hard_reset()) {
		uart_puts("ipi: skip (hard reset)\r\n");
		return;
	}

	clint_ipi_send_cpu1();
	udelay(50000);
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	uart_puts("ipi: wake ");
	uart_putdec32(wake0);
	uart_puts("->");
	uart_putdec32(cpu1_shmem->wake_count);
	uart_puts("  seen ");
	uart_putdec32(ipi0);
	uart_puts("->");
	uart_putdec32(cpu1_shmem->ipi_seen);
	uart_puts("\r\n");
}

static void park_for_jlink(void)
{
	uart_puts("park\r\n");
	cpu1_hold_reset();
	thead_cache_off();
	for (;;)
		__asm__ volatile("wfi");
}

int main(void)
{
	uint32_t entry = (uint32_t)(uintptr_t)_hart1_entry;
	struct cpu1_boot_result br;

	/* thead_cpu_init already done in _start — do not call again. */
	sysctl_early_init();
	uart_init();
	shmem_init();
	clint_ipi_clear_cpu1();

	/*
	 * Undo leftover ANSI from older builds (e.g. scroll-region 1;21).
	 * Without this, minicom keeps printing "in mid-air" with a dirty bottom.
	 */
	uart_puts("\x1b[r");   /* reset scroll region = full screen */
	uart_puts("\x1b[0m");  /* reset attrs */
	uart_puts("\x1b[2J");  /* clear screen */
	uart_puts("\x1b[H");   /* cursor home */

	print_boot_banner();

	thead_flush_range(LOAD_BASE, LOAD_BASE + 0x8000);
	thead_flush_caches();
	br = cpu1_boot(entry);
	report_boot_result(&br);

	if (wait_cpu1_alive(800))
		uart_puts("cpu1: ok\r\n");
	else
		uart_puts("cpu1: no shmem  (s=detail)\r\n");

	/* Let CPU1 print ticks only after boot chatter is done. */
	shmem_cpu0_console_ready();
	uart_rx_drain();
	print_help();

	for (;;) {
		int c = uart_try_getc();
		if (c < 0)
			continue;
		uart_putc((char)c);
		uart_puts("\r\n");

		switch (c) {
		case 's':
		case 'S':
			cpu1_dump_status("s");
			print_sw_status();
			break;
		case '1':
			shmem_cpu0_cmd(CPU1_CMD_GO_RUNNING);
			break;
		case '2':
			shmem_cpu0_cmd(CPU1_CMD_GO_SOFT_LOOP);
			break;
		case '3':
			shmem_cpu0_cmd(CPU1_CMD_GO_WFI);
			break;
		case 'i':
		case 'I':
			ipi_probe();
			break;
		case 'c':
		case 'C':
			clint_ipi_clear_cpu1();
			break;
		case 'q':
		case 'Q':
			park_for_jlink();
			break;
		case 'h':
		case 'H':
		case '?':
			print_help();
			break;
		default:
			break;
		}
	}
}
