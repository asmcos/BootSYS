/*
 * 09-irq — UART RX via PLIC (machine external interrupt)
 */
#include "k230.h"
#include "trap.h"
#include "uart_irq.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  09-irq                          │\r\n");
	uart_puts("│  UART RX → PLIC → mcause MEI (11)            │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======= menu =======\r\n");
	uart_puts("  i  IRQ on : keys arrive via trap (default)\r\n");
	uart_puts("  p  poll   : uart_try_getc, no UART IRQ\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("Type any other key: irq count should bump in IRQ mode.\r\n");
	uart_puts("08 timer was cause 7; this is cause 11 (PLIC src 16).\r\n");
	uart_puts("====================\r\n");
}

static void draw_status(int last)
{
	uart_puts("\r");
	uart_puts(uart_irq_mode() ? "IRQ " : "poll");
	uart_puts("  count=");
	uart_putdec32(uart_irq_count());
	uart_puts("  last=");
	if (last >= 32 && last < 127) {
		uart_putc('\'');
		uart_putc((char)last);
		uart_putc('\'');
	} else if (last >= 0) {
		uart_puthex32((uint32_t)last);
	} else {
		uart_puts("-");
	}
	uart_puts("  i/p/h");
	uart_puts("\x1b[K");
}

static int getc_now(void)
{
	if (uart_irq_mode())
		return uart_irq_getc();
	return uart_try_getc();
}

int main(void)
{
	int last = -1;
	uint32_t shown = ~0u;

	sysctl_early_init();
	uart_init();
	trap_init();
	uart_irq_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	print_help();
	uart_irq_on();
	draw_status(last);

	for (;;) {
		int c = getc_now();

		if (c >= 0) {
			last = c;
			switch (c) {
			case 'i':
			case 'I':
				uart_irq_on();
				break;
			case 'p':
			case 'P':
				uart_irq_off();
				break;
			case 'h':
			case 'H':
			case '?':
				uart_puts("\r\n");
				print_help();
				break;
			default:
				break;
			}
			shown = ~0u;
		}

		if (uart_irq_count() != shown) {
			shown = uart_irq_count();
			draw_status(last);
		} else if (c >= 0) {
			draw_status(last);
		}
	}
}
