/* Experiment 01 — UART bring-up only (single CPU0). */
#include "board.h"

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

static void print_banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  01-base-boot                     │\r\n");
	uart_puts("│  minimal boot + UART (CPU0 only)             │\r\n");
	uart_puts("├──────────────────────────────────────────────┤\r\n");
	uart_puts("│  board   K230                                │\r\n");
	uart_puts("│  load    0x80200000                          │\r\n");
	uart_puts("│  uart0   0x91400000  115200                  │\r\n");
	uart_puts("│  build   " __DATE__ " " __TIME__ "                  │\r\n");
	uart_puts("│  git     " BOOTSYS_GIT "\r\n");
	uart_puts("│  status  ready — type to echo                │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

int main(void)
{
	uart_init();

	/* Clear leftover ANSI scroll-region from other demos. */
	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");

	print_banner();

	uart_puts("cpu0 mhartid=");
	uart_puthex64(mhartid());
	uart_puts("\r\n");

	for (;;) {
		int c = uart_try_getc();
		if (c < 0)
			continue;
		if (c == '\r')
			uart_puts("\r\n");
		else
			uart_putc((char)c);
	}
}
