/*
 * 07-cache — C908 L1 tag dump via MCINDEX / MCINS / MCDATA
 */
#include "k230.h"
#include "trap.h"
#include "cache_tag.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  07-cache                        │\r\n");
	uart_puts("│  L1 tag dump (MCINDEX / MCINS / MCDATA)      │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======= menu =======\r\n");
	uart_puts("  1  touch probe, dump D-tag set\r\n");
	uart_puts("  2  call snippet, dump I-tag set\r\n");
	uart_puts("  f  flush L1, dump probe D-tag\r\n");
	uart_puts("  s  scan D-cache, print V=1\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("M-mode only. Tag decode is C9xx-style; raw always printed.\r\n");
	uart_puts("====================\r\n");
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	cache_tag_init();
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
		case '1':
			test_dcache_touch_dump();
			break;
		case '2':
			test_icache_dump();
			break;
		case 'f':
		case 'F':
			test_dcache_flush_dump();
			break;
		case 's':
		case 'S':
			test_dcache_scan_valid();
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
