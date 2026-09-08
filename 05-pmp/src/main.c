/*
 * 05-pmp — set a PMP region, test M/S/U R/W, then clear and retest.
 */
#include "k230.h"
#include "trap.h"
#include "priv.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  05-pmp                          │\r\n");
	uart_puts("│  set region, test M/S/U, then clear          │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_status(void)
{
	uart_puts("\r\n");
	uart_puts("======== ");
	uart_puts(priv_mode_name(g_cur_mode));
	uart_puts("  PMP: ");
	uart_puts(pmp_policy_name(g_pmp_policy));
	uart_puts(" ========\r\n");
}

static void print_help(void)
{
	print_status();
	uart_puts("  1  read  protected region\r\n");
	uart_puts("  2  write protected region\r\n");
	uart_puts("  e  enable region  (M only)\r\n");
	uart_puts("  c  clear  region  (M only)\r\n");
	uart_puts("  m / s / u   switch mode     h help\r\n");
	uart_puts("PMP on : M R/W OK; S/U trap (load=5 store=7)\r\n");
	uart_puts("PMP off: M/S/U R/W all OK\r\n");
	uart_puts("==============================================\r\n");
}

static void prompt(void)
{
	uart_puts("\r\n[");
	switch (g_cur_mode) {
	case PRIV_M:
		uart_puts("M");
		break;
	case PRIV_S:
		uart_puts("S");
		break;
	case PRIV_U:
		uart_puts("U");
		break;
	}
	uart_puts("] > ");
}

static void pmp_from_m(int enable)
{
	if (g_cur_mode != PRIV_M) {
		uart_puts("do this in M (press m first).\r\n");
		return;
	}
	if (enable) {
		if (pmp_enable())
			uart_puts("PMP: on (region denied to S/U)\r\n");
	} else {
		if (pmp_clear())
			uart_puts("PMP: off (region open)\r\n");
	}
}

static int handle_common(int c)
{
	switch (c) {
	case '1':
		test_region_read();
		return 1;
	case '2':
		test_region_write();
		return 1;
	case 'e':
	case 'E':
		pmp_from_m(1);
		return 1;
	case 'c':
	case 'C':
		pmp_from_m(0);
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

static void goto_m_from_lower(void)
{
	uart_puts("request -> M-mode ...\r\n");
	priv_request_mode(ECALL_GOTO_M);
}

static void goto_s_from_lower(void)
{
	uart_puts("request -> S-mode ...\r\n");
	priv_request_mode(ECALL_GOTO_S);
}

static void goto_u_from_lower(void)
{
	uart_puts("request -> U-mode ...\r\n");
	priv_request_mode(ECALL_GOTO_U);
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
		case 'u':
		case 'U':
			uart_puts("switch -> U-mode ...\r\n");
			g_cur_mode = PRIV_U;
			priv_enter(shell_u, MSTATUS_MPP_U);
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
			goto_m_from_lower();
			break;
		case 's':
		case 'S':
			uart_puts("already in S-mode.\r\n");
			break;
		case 'u':
		case 'U':
			goto_u_from_lower();
			break;
		default:
			uart_puts("unknown, press h\r\n");
			break;
		}
	}
}

void shell_u(void)
{
	g_cur_mode = PRIV_U;
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
			goto_m_from_lower();
			break;
		case 's':
		case 'S':
			goto_s_from_lower();
			break;
		case 'u':
		case 'U':
			uart_puts("already in U-mode.\r\n");
			break;
		default:
			uart_puts("unknown, press h\r\n");
			break;
		}
	}
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	/* Prompt may say [M] while hart is still U (JTAG/no reset). Raise first. */
	priv_force_m();
	pmp_enable();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();

	uart_puts("region @ ");
	uart_puthex64((uint64_t)(uintptr_t)_pmp_secret);
	uart_puts("  4K   PMP: ");
	uart_puts(pmp_policy_name(g_pmp_policy));
	uart_puts("\r\n");

	shell_m();
	return 0;
}
