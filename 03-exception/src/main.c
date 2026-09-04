/*
 * 03-exception — trap dump demo (based on 02 bring-up, CPU0 focus).
 *
 * RISC-V does NOT trap on integer divide-by-zero (result is all-ones).
 * Menu 'z' shows that, then forces ebreak to exercise the handler.
 */
#include "k230.h"
#include "trap.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  03-exception                    │\r\n");
	uart_puts("│  trap dump: access fault / div0 probe        │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======= menu =======\r\n");
	uart_puts("  a  load access fault (bad addr)\r\n");
	uart_puts("  s  store access fault\r\n");
	uart_puts("  z  div0 probe (HW no-trap + ebreak)\r\n");
	uart_puts("  i  illegal instruction\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("====================\r\n");
}

static void test_load_fault(void)
{
	volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)0x00000001UL;

	uart_puts("cpu0: load from 0x1 ...\r\n");
	(void)*p;
	uart_puts("cpu0: resumed after load fault\r\n");
}

static void test_store_fault(void)
{
	volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)0x00000008UL;

	uart_puts("cpu0: store to 0x8 ...\r\n");
	*p = 0xdeadbeefu;
	uart_puts("cpu0: resumed after store fault\r\n");
}

static void test_div0(void)
{
	volatile uint64_t a = 100;
	volatile uint64_t b = 0;
	volatile uint64_t q;

	uart_puts("cpu0: RISC-V DIV by 0 (no trap expected)...\r\n");
	q = a / b;
	uart_puts("cpu0: quotient=");
	uart_puthex64(q);
	uart_puts(" (typically all ones)\r\n");

	uart_puts("cpu0: force ebreak as div0 software probe...\r\n");
	__asm__ volatile("ebreak");
	uart_puts("cpu0: resumed after ebreak\r\n");
}

static void test_illegal(void)
{
	uart_puts("cpu0: illegal insn ...\r\n");
	__asm__ volatile(".word 0x00000000");
	uart_puts("cpu0: resumed after illegal\r\n");
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();

	uart_puts("cpu0 mhartid=");
	uart_puthex64(mhartid());
	uart_puts("\r\n");
	uart_puts("mtvec installed; sync traps will dump CSRs/GPRs then skip insn.\r\n");
	print_help();

	for (;;) {
		int c = uart_try_getc();
		if (c < 0)
			continue;
		uart_putc((char)c);
		uart_puts("\r\n");

		switch (c) {
		case 'a':
		case 'A':
			test_load_fault();
			break;
		case 's':
		case 'S':
			test_store_fault();
			break;
		case 'z':
		case 'Z':
			test_div0();
			break;
		case 'i':
		case 'I':
			test_illegal();
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
