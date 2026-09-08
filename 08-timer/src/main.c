/*
 * 08-timer — CLINT mtime/mtimecmp machine-timer interrupt + serial stopwatch
 */
#include "k230.h"
#include "trap.h"
#include "timer.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  08-timer                        │\r\n");
	uart_puts("│  CLINT mtime IRQ + stopwatch                 │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======= menu =======\r\n");
	uart_puts("  s  start     p  pause     r  reset\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("10 Hz machine timer (mcause interrupt 7).\r\n");
	uart_puts("mret does not skip; mepc is the interrupted insn.\r\n");
	uart_puts("====================\r\n");
}

static void put2(uint32_t n)
{
	uart_putc((char)('0' + (n / 10u) % 10u));
	uart_putc((char)('0' + n % 10u));
}

static void draw_clock(void)
{
	uint32_t t = timer_ticks();
	uint32_t tenths = t % 10u;
	uint32_t sec = (t / 10u) % 60u;
	uint32_t min = t / 600u;

	uart_puts("\r");
	put2(min);
	uart_putc(':');
	put2(sec);
	uart_putc('.');
	uart_putc((char)('0' + tenths));
	uart_puts(timer_running() ? "  run " : "  stop");
	uart_puts("  s/p/r/h");
	uart_puts("\x1b[K");
}

int main(void)
{
	uint32_t last = ~0u;

	sysctl_early_init();
	uart_init();
	trap_init();
	timer_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	uart_puts("mtime @ 27 MHz (CLINT). Press s to start.\r\n");
	print_help();
	draw_clock();

	for (;;) {
		int c = uart_try_getc();

		if (c >= 0) {
			switch (c) {
			case 's':
			case 'S':
				timer_start();
				break;
			case 'p':
			case 'P':
				timer_pause();
				break;
			case 'r':
			case 'R':
				timer_reset();
				last = ~0u;
				break;
			case 'h':
			case 'H':
			case '?':
				uart_puts("\r\n");
				print_help();
				last = ~0u;
				break;
			default:
				break;
			}
			draw_clock();
			last = timer_ticks();
		}

		if (timer_ticks() != last) {
			last = timer_ticks();
			draw_clock();
		}
	}
}
