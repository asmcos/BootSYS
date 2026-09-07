#include "k230.h"
#include "trap.h"
#include "priv.h"

volatile enum priv_mode g_cur_mode = PRIV_M;

const char *priv_mode_name(enum priv_mode m)
{
	switch (m) {
	case PRIV_M:
		return "M-mode";
	case PRIV_S:
		return "S-mode";
	case PRIV_U:
		return "U-mode";
	default:
		return "?";
	}
}

void priv_request_mode(int ecall_code)
{
	register unsigned long a0 asm("a0") = (unsigned long)ecall_code;

	__asm__ volatile("ecall" : "+r"(a0) : : "memory");
}

void pmp_allow_all(void)
{
	__asm__ volatile("csrw pmpaddr0, %0" ::"r"((uint64_t)-1));
	__asm__ volatile("csrw pmpcfg0, %0" ::"r"(0x1fULL));
	__asm__ volatile("sfence.vma" ::: "memory");
}

static void finish_test(int ok_expect_trap)
{
	if (trap_saw) {
		uart_puts("  result: trapped, still in ");
		uart_puts(priv_mode_name(g_cur_mode));
		uart_puts("\r\n");
		uart_puts(ok_expect_trap ? "  verdict: PASS\r\n"
					 : "  verdict: FAIL (unexpected trap)\r\n");
	} else {
		uart_puts("  result: ok, no trap\r\n");
		uart_puts(ok_expect_trap ? "  verdict: FAIL (expected trap)\r\n"
					 : "  verdict: PASS\r\n");
	}
}

void test_csrr_mhartid(void)
{
	uint64_t v = 0;
	int expect_trap = (g_cur_mode != PRIV_M);

	uart_puts("\r\n>>> test: csrr mhartid");
	uart_puts(expect_trap ? " (expect TRAP)\r\n" : " (expect OK)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	__asm__ volatile("csrr %0, mhartid" : "=r"(v) : : "memory");
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  value: ");
		uart_puthex64(v);
		uart_puts("\r\n");
	}
	finish_test(expect_trap);
}

void test_csrr_sstatus(void)
{
	uint64_t v = 0;
	int expect_trap = (g_cur_mode == PRIV_U);

	uart_puts("\r\n>>> test: csrr sstatus");
	uart_puts(expect_trap ? " (expect TRAP)\r\n" : " (expect OK)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	__asm__ volatile("csrr %0, sstatus" : "=r"(v) : : "memory");
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  value: ");
		uart_puthex64(v);
		uart_puts("\r\n");
	}
	finish_test(expect_trap);
}

void test_csrr_mstatus(void)
{
	uint64_t v = 0;
	int expect_trap = (g_cur_mode != PRIV_M);

	uart_puts("\r\n>>> test: csrr mstatus");
	uart_puts(expect_trap ? " (expect TRAP)\r\n" : " (expect OK)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	__asm__ volatile("csrr %0, mstatus" : "=r"(v) : : "memory");
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  value: ");
		uart_puthex64(v);
		uart_puts("\r\n");
	}
	finish_test(expect_trap);
}

void test_ecall(void)
{
	int expect_cause =
		(g_cur_mode == PRIV_M) ? 11 : (g_cur_mode == PRIV_S) ? 9 : 8;

	uart_puts("\r\n>>> test: ecall (expect cause=");
	uart_putdec32((uint32_t)expect_cause);
	uart_puts(", stay in mode)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	{
		register unsigned long a0 asm("a0") = ECALL_TEST;

		__asm__ volatile("ecall" : "+r"(a0) : : "memory");
	}
	trap_expect = 0;

	if (trap_saw) {
		uint64_t code = trap_last_cause & 0xfffu;

		uart_puts("  cause code: ");
		uart_putdec32((uint32_t)code);
		uart_puts(code == (uint64_t)expect_cause ? " PASS\r\n" : " FAIL\r\n");
	}
	finish_test(1);
}

void prove_current_mode(void)
{
	int expect_cause =
		(g_cur_mode == PRIV_M) ? 11 : (g_cur_mode == PRIV_S) ? 9 : 8;
	int expect_mpp =
		(g_cur_mode == PRIV_M) ? 3 : (g_cur_mode == PRIV_S) ? 1 : 0;
	uint64_t code;
	int pass;

	uart_puts("\r\n>>> PROVE privilege (no CSR = \"current mode\")\r\n");
	uart_puts("  method: ecall -> M trap, then read mcause + mstatus.MPP\r\n");
	uart_puts("  expect: cause=");
	uart_putdec32((uint32_t)expect_cause);
	uart_puts("  MPP=");
	uart_putdec32((uint32_t)expect_mpp);
	uart_puts(" (3=M,1=S,0=U)\r\n");

	trap_saw = 0;
	trap_last_mpp = 0xff;
	trap_expect = 1;
	{
		register unsigned long a0 asm("a0") = ECALL_TEST;

		__asm__ volatile("ecall" : "+r"(a0) : : "memory");
	}
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  FAIL: ecall did not trap\r\n");
		return;
	}

	code = trap_last_cause & 0xfffu;
	uart_puts("  got:    cause=");
	uart_putdec32((uint32_t)code);
	uart_puts("  MPP=");
	uart_putdec32((uint32_t)trap_last_mpp);
	uart_puts("\r\n");

	pass = (code == (uint64_t)expect_cause) &&
	       (trap_last_mpp == (uint64_t)expect_mpp);
	uart_puts(pass ? "  => switch/mode PROVED OK\r\n"
		       : "  => PROVE FAIL\r\n");
}
