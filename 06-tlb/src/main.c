/*
 * 06-tlb — Sv39 identity map + unmapped fault + stale TLB vs sfence.vma
 */
#include "k230.h"
#include "trap.h"
#include "priv.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  06-tlb                          │\r\n");
	uart_puts("│  Sv39 page table + TLB / sfence.vma          │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	uart_puts("\r\n======== ");
	uart_puts(priv_mode_name(g_cur_mode));
	uart_puts("  MMU: ");
	uart_puts(g_mmu_on ? "on" : "off");
	uart_puts(" ========\r\n");
	uart_puts("  o  MMU on   (M): satp Sv39, enter S\r\n");
	uart_puts("  z  MMU off      : satp=0, enter M\r\n");
	uart_puts("  1  load mapped VA 0x1000   (S, expect OK)\r\n");
	uart_puts("  2  load unmapped VA 0x4000 (S, expect pf 13)\r\n");
	uart_puts("  f  drop PTE.V, load, sfence, load\r\n");
	uart_puts("  m / s           h help\r\n");
	uart_puts("M never walks satp; tests 1/2/f need S + MMU on.\r\n");
	uart_puts("==============================================\r\n");
}

static void prompt(void)
{
	uart_puts("\r\n[");
	uart_puts((g_cur_mode == PRIV_M) ? "M" :
		  (g_cur_mode == PRIV_S) ? "S" : "U");
	uart_puts(g_mmu_on ? "/mmu" : "");
	uart_puts("] > ");
}

static void do_mmu_off(void)
{
	if (g_cur_mode == PRIV_M) {
		mmu_off();
		uart_puts("satp=0  MMU: off\r\n");
		return;
	}
	uart_puts("request MMU off -> M ...\r\n");
	priv_request_mode(ECALL_MMU_OFF);
}

static int handle_common(int c)
{
	switch (c) {
	case '1':
		test_mapped_load();
		return 1;
	case '2':
		test_unmapped_load();
		return 1;
	case 'f':
	case 'F':
		test_tlb_stale();
		return 1;
	case 'o':
	case 'O':
		mmu_on();
		return 1;
	case 'z':
	case 'Z':
		do_mmu_off();
		return 1;
	case 'h':
	case 'H':
	case '?':
		print_help();
		return 1;
	default:
		return 0;
	}
}

void shell_m(void)
{
	g_cur_mode = PRIV_M;
	print_help();

	for (;;) {
		int c;

		prompt();
		do {
			c = uart_try_getc();
		} while (c < 0);
		uart_putc((char)c);
		uart_puts("\r\n");
		if (handle_common(c))
			continue;
		switch (c) {
		case 'm':
		case 'M':
			uart_puts("already in M-mode.\r\n");
			break;
		case 's':
		case 'S':
			uart_puts("switch -> S-mode ...\r\n");
			g_cur_mode = PRIV_S;
			priv_enter(shell_s, MSTATUS_MPP_S);
			break;
		default:
			uart_puts("unknown, press h\r\n");
			break;
		}
	}
}

void shell_s(void)
{
	g_cur_mode = PRIV_S;
	print_help();

	for (;;) {
		int c;

		prompt();
		do {
			c = uart_try_getc();
		} while (c < 0);
		uart_putc((char)c);
		uart_puts("\r\n");
		if (handle_common(c))
			continue;
		switch (c) {
		case 'm':
		case 'M':
			uart_puts("request -> M-mode ...\r\n");
			priv_request_mode(ECALL_GOTO_M);
			break;
		case 's':
		case 'S':
			uart_puts("already in S-mode.\r\n");
			break;
		default:
			uart_puts("unknown, press h\r\n");
			break;
		}
	}
}

void shell_u(void)
{
	/* Unused: Sv39 PTEs have no U bit. */
	priv_enter(shell_s, MSTATUS_MPP_S);
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	priv_force_m();
	pmp_allow_all();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	uart_puts("satp bare. Press o to install Sv39 and enter S.\r\n");
	shell_m();
	return 0;
}
