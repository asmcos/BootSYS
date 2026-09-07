/*
 * 04-privilege — interactive M/S/U shell (ASCII UI for stable UART).
 */
#include "k230.h"
#include "trap.h"
#include "priv.h"

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("|  BootSYS  -  04-privilege                    |\r\n");
	uart_puts("|  privilege switch + permission check         |\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("\r\n");
}

static void print_status(void)
{
	uart_puts("\r\n");
	uart_puts("======== current: ");
	uart_puts(priv_mode_name(g_cur_mode));
	uart_puts(" ========\r\n");
}

static void print_help(void)
{
	print_status();
	uart_puts("tests (run in current mode; trap returns here):\r\n");
	uart_puts("  1  csrr mhartid   - M only; S/U -> trap\r\n");
	uart_puts("  2  csrr sstatus   - M/S ok; U -> trap\r\n");
	uart_puts("  3  csrr mstatus   - M only; S/U -> trap\r\n");
	uart_puts("  4  ecall          - cause M=11 / S=9 / U=8\r\n");
	uart_puts("  p  prove mode     - ecall + mstatus.MPP (hard proof)\r\n");
	uart_puts("switch mode:\r\n");
	uart_puts("  m  enter M-mode\r\n");
	uart_puts("  s  enter S-mode\r\n");
	uart_puts("  u  enter U-mode\r\n");
	uart_puts("  h  help\r\n");
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

static int handle_common(int c)
{
	switch (c) {
	case '1':
		test_csrr_mhartid();
		return 1;
	case '2':
		test_csrr_sstatus();
		return 1;
	case '3':
		test_csrr_mstatus();
		return 1;
	case '4':
		test_ecall();
		return 1;
	case 'p':
	case 'P':
		prove_current_mode();
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
	uint64_t misa;

	sysctl_early_init();
	uart_init();
	trap_init();
	pmp_allow_all();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();

	uart_puts("cpu0 mhartid=");
	uart_puthex64(mhartid());
	uart_puts("\r\n");

	__asm__ volatile("csrr %0, misa" : "=r"(misa));
	uart_puts("misa S=");
	uart_puts((misa & (1ULL << 18)) ? "yes" : "NO");
	uart_puts(" U=");
	uart_puts((misa & (1ULL << 20)) ? "yes" : "NO");
	uart_puts("\r\n");
	uart_puts("PMP open (need for S/U fetch at 0x80200000).\r\n");
	uart_puts("traps in M (medeleg=0); after test stay in same mode.\r\n");

	shell_m();
	return 0;
}
