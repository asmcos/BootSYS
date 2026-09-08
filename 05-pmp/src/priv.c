#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "priv.h"

#define PMP_R       (1ULL << 0)
#define PMP_W       (1ULL << 1)
#define PMP_X       (1ULL << 2)
#define PMP_A_NAPOT (3ULL << 3)

#define PMP_SRAM_SIZE   0x40000UL
#define PMP_SECRET_SIZE 0x1000UL
#define PMP_UART_SIZE   0x10000UL

#define SECRET_MAGIC 0x50504d50u

volatile enum priv_mode g_cur_mode = PRIV_M;
volatile enum pmp_policy g_pmp_policy = PMP_ON;

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

const char *pmp_policy_name(enum pmp_policy p)
{
	return (p == PMP_ON) ? "on" : "off";
}

void priv_request_mode(int ecall_code)
{
	register unsigned long a0 asm("a0") = (unsigned long)ecall_code;

	__asm__ volatile("ecall" : "+r"(a0) : : "memory");
}

void priv_force_m(void)
{
	register unsigned long a0 asm("a0") = ECALL_ENTER_M;

	__asm__ volatile("ecall" : "+r"(a0) : : "memory");
}

static int pmp_csr_begin(void)
{
	trap_saw = 0;
	trap_quiet = 1;
	trap_expect = 1;
	return 0;
}

static int pmp_csr_end(void)
{
	int fail = trap_saw;

	trap_expect = 0;
	trap_quiet = 0;
	if (fail)
		uart_puts("PMP CSR illegal (not in M-mode, or no PMP).\r\n");
	return fail ? 0 : 1;
}

static uint64_t pmpaddr_napot(uintptr_t base, uintptr_t size)
{
	return ((uint64_t)base >> 2) | ((size >> 3) - 1ULL);
}

static void csr_pmpaddr0(uint64_t v)
{
	__asm__ volatile("csrw pmpaddr0, %0" ::"r"(v));
}

static void csr_pmpaddr1(uint64_t v)
{
	__asm__ volatile("csrw pmpaddr1, %0" ::"r"(v));
}

static void csr_pmpaddr2(uint64_t v)
{
	__asm__ volatile("csrw pmpaddr2, %0" ::"r"(v));
}

static void csr_pmpcfg0(uint64_t v)
{
	__asm__ volatile("csrw pmpcfg0, %0" ::"r"(v));
}

int pmp_clear(void)
{
	pmp_csr_begin();
	csr_pmpcfg0(0);
	csr_pmpaddr0((uint64_t)-1);
	csr_pmpaddr1(0);
	csr_pmpaddr2(0);
	csr_pmpcfg0(PMP_A_NAPOT | PMP_R | PMP_W | PMP_X);
	__asm__ volatile("sfence.vma" ::: "memory");
	if (!pmp_csr_end())
		return 0;
	g_pmp_policy = PMP_OFF;
	return 1;
}

int pmp_enable(void)
{
	uint64_t cfg;
	uintptr_t secret = (uintptr_t)_pmp_secret;

	pmp_csr_begin();
	csr_pmpcfg0(0);
	csr_pmpaddr0(pmpaddr_napot(secret, PMP_SECRET_SIZE));
	csr_pmpaddr1(pmpaddr_napot(LOAD_BASE, PMP_SRAM_SIZE));
	csr_pmpaddr2(pmpaddr_napot(UART0_BASE, PMP_UART_SIZE));

	cfg = (PMP_A_NAPOT) |
	      ((PMP_A_NAPOT | PMP_R | PMP_W | PMP_X) << 8) |
	      ((PMP_A_NAPOT | PMP_R | PMP_W) << 16);
	csr_pmpcfg0(cfg);
	__asm__ volatile("sfence.vma" ::: "memory");
	if (!pmp_csr_end())
		return 0;

	*(volatile uint32_t *)secret = SECRET_MAGIC;
	g_pmp_policy = PMP_ON;
	return 1;
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

static int expect_region_trap(void)
{
	return (g_cur_mode != PRIV_M) && (g_pmp_policy == PMP_ON);
}

void test_region_read(void)
{
	volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)_pmp_secret;
	uint32_t v = 0;
	int expect = expect_region_trap();

	uart_puts("\r\n>>> LOAD region @ ");
	uart_puthex64((uint64_t)(uintptr_t)p);
	uart_puts(expect ? " (expect TRAP, mcause=5)\r\n" : " (expect OK)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	v = *p;
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  value: ");
		uart_puthex32(v);
		uart_puts("\r\n");
	}
	finish_test(expect);
}

void test_region_write(void)
{
	volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)_pmp_secret;
	int expect = expect_region_trap();

	uart_puts("\r\n>>> STORE region @ ");
	uart_puthex64((uint64_t)(uintptr_t)p);
	uart_puts(expect ? " (expect TRAP, mcause=7)\r\n" : " (expect OK)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	*p = SECRET_MAGIC;
	trap_expect = 0;

	finish_test(expect);
}
