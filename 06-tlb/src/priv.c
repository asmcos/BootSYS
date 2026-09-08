#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "priv.h"

#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_A (1ULL << 6)
#define PTE_D (1ULL << 7)

#define PTE_LEAF (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)
#define PTE_NEXT(pa) (PTE_V | (((uint64_t)(pa) >> 12) << 10))
#define PTE_LEAF_PA(pa) (PTE_LEAF | (((uint64_t)(pa) >> 12) << 10))

#define MXSTATUS_MAEE (1UL << 21)
#define MEGA_MASK     ~0x1FFFFFUL

#define PMP_R       (1ULL << 0)
#define PMP_W       (1ULL << 1)
#define PMP_X       (1ULL << 2)
#define PMP_A_NAPOT (3ULL << 3)

#define PROBE_MAGIC 0x544c4231u /* 'TLB1' */

volatile enum priv_mode g_cur_mode = PRIV_M;
volatile int g_mmu_on;

static uint64_t pt_l2[512] __attribute__((section(".pgtbl"), aligned(4096)));
static uint64_t pt_l1_a[512] __attribute__((section(".pgtbl"), aligned(4096)));
static uint64_t pt_l1_b[512] __attribute__((section(".pgtbl"), aligned(4096)));
static uint64_t pt_l1_lo[512] __attribute__((section(".pgtbl"), aligned(4096)));
static uint64_t pt_l0_lo[512] __attribute__((section(".pgtbl"), aligned(4096)));
static uint32_t tlb_page[1024] __attribute__((section(".pgtbl"), aligned(4096)));

static uint64_t *probe_pte;

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

void priv_force_m(void)
{
	register unsigned long a0 asm("a0") = ECALL_ENTER_M;

	__asm__ volatile("ecall" : "+r"(a0) : : "memory");
}

void pmp_allow_all(void)
{
	__asm__ volatile("csrw pmpcfg0, zero");
	__asm__ volatile("csrw pmpaddr0, %0" ::"r"((uint64_t)-1));
	__asm__ volatile("csrw pmpcfg0, %0" ::"r"(PMP_A_NAPOT | PMP_R | PMP_W | PMP_X));
	__asm__ volatile("sfence.vma" ::: "memory");
}

void mxstatus_clear_maee(void)
{
	unsigned long v;

	__asm__ volatile("csrr %0, 0x7c0" : "=r"(v));
	v &= ~MXSTATUS_MAEE;
	__asm__ volatile("csrw 0x7c0, %0" ::"r"(v));
}

static void pgtbl_zero(uint64_t *t)
{
	int i;

	for (i = 0; i < 512; i++)
		t[i] = 0;
}

static uint64_t *l1_for_vpn2(unsigned vpn2)
{
	unsigned vpn2_load = (LOAD_BASE >> 30) & 0x1ffu;

	if (vpn2 == vpn2_load)
		return pt_l1_a;
	return pt_l1_b;
}

/* Identity-map one 2MiB megapage. VPN2=0 is reserved for the 4K probe. */
static void map_2m(uint64_t pa)
{
	unsigned vpn2 = (pa >> 30) & 0x1ffu;
	unsigned vpn1 = (pa >> 21) & 0x1ffu;
	uint64_t aligned = pa & MEGA_MASK;
	uint64_t *l1;

	if (vpn2 == 0)
		return;

	l1 = l1_for_vpn2(vpn2);
	if (!(pt_l2[vpn2] & PTE_V))
		pt_l2[vpn2] = PTE_NEXT((uintptr_t)l1);
	l1[vpn1] = PTE_LEAF_PA(aligned);
}

void pgtbl_init(void)
{
	pgtbl_zero(pt_l2);
	pgtbl_zero(pt_l1_a);
	pgtbl_zero(pt_l1_b);
	pgtbl_zero(pt_l1_lo);
	pgtbl_zero(pt_l0_lo);

	/* SRAM: code+RAM share one 2MiB page. DDR: 0x50000000 and 0x50200000. */
	map_2m(LOAD_BASE);
	map_2m(SHMEM_BASE);
	map_2m(UART0_BASE);

	/* VPN2=0: probe VA 0x1000 -> tlb_page */
	pt_l2[0] = PTE_NEXT((uintptr_t)pt_l1_lo);
	pt_l1_lo[0] = PTE_NEXT((uintptr_t)pt_l0_lo);
	probe_pte = &pt_l0_lo[(TLB_PROBE_VA >> 12) & 0x1ffu];
	*probe_pte = PTE_LEAF_PA((uintptr_t)tlb_page);
	tlb_page[0] = PROBE_MAGIC;

	/* PTW may not snoop D-cache; push PTEs out before satp/sfence. */
	thead_flush_range((uintptr_t)pt_l2, (uintptr_t)pt_l2 + sizeof(pt_l2));
	thead_flush_range((uintptr_t)pt_l1_a, (uintptr_t)pt_l1_a + sizeof(pt_l1_a));
	thead_flush_range((uintptr_t)pt_l1_b, (uintptr_t)pt_l1_b + sizeof(pt_l1_b));
	thead_flush_range((uintptr_t)pt_l1_lo, (uintptr_t)pt_l1_lo + sizeof(pt_l1_lo));
	thead_flush_range((uintptr_t)pt_l0_lo, (uintptr_t)pt_l0_lo + sizeof(pt_l0_lo));
	thead_flush_range((uintptr_t)tlb_page, (uintptr_t)tlb_page + sizeof(tlb_page));
}

static void w_satp(uint64_t v)
{
	__asm__ volatile("csrw satp, %0" ::"r"(v));
	__asm__ volatile("sfence.vma" ::: "memory");
}

int mmu_on(void)
{
	if (g_cur_mode != PRIV_M) {
		uart_puts("enable MMU in M (press m).\r\n");
		return 0;
	}
	mxstatus_clear_maee();
	pgtbl_init();
	w_satp(SATP_MODE_SV39 | ((uintptr_t)pt_l2 >> 12));
	g_mmu_on = 1;
	uart_puts("satp=Sv39 identity SRAM+UART, probe VA ");
	uart_puthex64(TLB_PROBE_VA);
	uart_puts(" -> tlb_page\r\n");
	uart_puts("switch -> S-mode (M does not walk the page table)\r\n");
	g_cur_mode = PRIV_S;
	priv_enter(shell_s, MSTATUS_MPP_S);
	return 1;
}

void mmu_off(void)
{
	w_satp(0);
	g_mmu_on = 0;
}

static int need_s_mmu(void)
{
	if (!g_mmu_on) {
		uart_puts("MMU off. Press o in M first.\r\n");
		return 0;
	}
	if (g_cur_mode != PRIV_S) {
		uart_puts("do this in S (MMU walks only in S/U).\r\n");
		return 0;
	}
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

void test_mapped_load(void)
{
	uint32_t v = 0;

	if (!need_s_mmu())
		return;

	uart_puts("\r\n>>> LOAD mapped VA ");
	uart_puthex64(TLB_PROBE_VA);
	uart_puts(" (expect OK, magic TLB1)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	v = *(volatile uint32_t *)TLB_PROBE_VA;
	trap_expect = 0;

	if (!trap_saw) {
		uart_puts("  value: ");
		uart_puthex32(v);
		uart_puts("\r\n");
	}
	finish_test(0);
}

void test_unmapped_load(void)
{
	uint32_t v = 0;

	if (!need_s_mmu())
		return;

	uart_puts("\r\n>>> LOAD unmapped VA ");
	uart_puthex64(TLB_UNMAP_VA);
	uart_puts(" (expect page fault, mcause=13)\r\n");

	trap_saw = 0;
	trap_expect = 1;
	v = *(volatile uint32_t *)TLB_UNMAP_VA;
	trap_expect = 0;
	(void)v;

	if (trap_saw) {
		uint64_t code = trap_last_cause & 0xfffu;

		uart_puts("  cause=");
		uart_putdec32((uint32_t)code);
		uart_puts(code == 13 ? " PASS\r\n" : " (want 13)\r\n");
	}
	finish_test(1);
}

void test_tlb_stale(void)
{
	uint32_t v1 = 0, v2 = 0;
	uint64_t saved;

	if (!need_s_mmu())
		return;

	uart_puts("\r\n>>> TLB: clear PTE.V, load, sfence, load again\r\n");

	/* Warm TLB */
	trap_saw = 0;
	trap_expect = 1;
	v1 = *(volatile uint32_t *)TLB_PROBE_VA;
	trap_expect = 0;
	if (trap_saw) {
		uart_puts("  warmup load trapped (mapping broken)\r\n");
		return;
	}
	uart_puts("  warmup OK value=");
	uart_puthex32(v1);
	uart_puts("\r\n");

	saved = *probe_pte;
	*probe_pte = 0; /* V=0, no sfence yet */
	thead_flush_range((uintptr_t)probe_pte, (uintptr_t)probe_pte + 8);

	trap_saw = 0;
	trap_expect = 1;
	v2 = *(volatile uint32_t *)TLB_PROBE_VA;
	trap_expect = 0;

	if (trap_saw)
		uart_puts("  after drop V, no fence: TRAP (TLB miss/refill saw new PTE)\r\n");
	else {
		uart_puts("  after drop V, no fence: still OK ");
		uart_puthex32(v2);
		uart_puts("  <- stale TLB\r\n");
	}

	__asm__ volatile("sfence.vma" ::: "memory");

	trap_saw = 0;
	trap_expect = 1;
	v2 = *(volatile uint32_t *)TLB_PROBE_VA;
	trap_expect = 0;

	if (trap_saw) {
		uint64_t code = trap_last_cause & 0xfffu;

		uart_puts("  after sfence.vma: TRAP cause=");
		uart_putdec32((uint32_t)code);
		uart_puts(code == 13 ? " PASS\r\n" : "\r\n");
	} else {
		uart_puts("  after sfence.vma: still OK (FAIL, TLB not flushed)\r\n");
	}

	*probe_pte = saved;
	thead_flush_range((uintptr_t)probe_pte, (uintptr_t)probe_pte + 8);
	__asm__ volatile("sfence.vma" ::: "memory");
	uart_puts("  PTE restored\r\n");
}
