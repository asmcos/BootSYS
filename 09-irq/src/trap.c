#include "k230.h"
#include "trap.h"
#include "uart_irq.h"

volatile int trap_skip_insn;

static void put_kv64(const char *k, uint64_t v)
{
	uart_puts("  ");
	uart_puts(k);
	uart_puts(" = ");
	uart_puthex64(v);
	uart_puts("\r\n");
}

static inline uint64_t r_mepc(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mepc" : "=r"(v));
	return v;
}

static inline void w_mepc(uint64_t v)
{
	__asm__ volatile("csrw mepc, %0" ::"r"(v));
}

static inline uint64_t r_mcause(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mcause" : "=r"(v));
	return v;
}

static inline uint64_t r_mtval(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mtval" : "=r"(v));
	return v;
}

static inline uint64_t r_mstatus(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mstatus" : "=r"(v));
	return v;
}

static const char *cause_name(uint64_t cause)
{
	int interrupt = (int)((cause >> 63) & 1);
	uint64_t code = cause & 0xfffu;

	if (interrupt) {
		if (code == MCAUSE_MEI)
			return "Machine external interrupt";
		return "interrupt";
	}
	switch (code) {
	case 2:
		return "Illegal instruction";
	case 5:
		return "Load access fault";
	default:
		return "Unknown";
	}
}

static unsigned insn_len(uint64_t mepc)
{
	uint16_t half = *(volatile uint16_t *)(uintptr_t)mepc;

	if ((half & 0x3u) != 0x3u)
		return 2;
	return 4;
}

void trap_init(void)
{
	extern void trap_entry(void);

	__asm__ volatile("csrw mtvec, %0" ::"r"((uintptr_t)trap_entry));
	__asm__ volatile("csrw medeleg, zero");
	__asm__ volatile("csrw mideleg, zero");
	trap_skip_insn = 1;
}

void trap_handler(struct trap_frame *tf)
{
	uint64_t cause = r_mcause();
	uint64_t epc = r_mepc();
	uint64_t code = cause & 0xfffu;
	int interrupt = (int)((cause >> 63) & 1);

	(void)tf;

	if (interrupt && code == MCAUSE_MEI) {
		uart_irq_on_mei();
		return;
	}

	uart_puts("\r\n!!!! UNEXPECTED TRAP !!!!\r\n");
	uart_puts(cause_name(cause));
	uart_puts("\r\n");
	put_kv64("mcause ", cause);
	put_kv64("  code ", code);
	uart_puts("  irq?  = ");
	uart_puts(interrupt ? "yes\r\n" : "no\r\n");
	put_kv64("mepc   ", epc);
	put_kv64("mtval  ", r_mtval());
	put_kv64("mstatus", r_mstatus());

	if (!interrupt && trap_skip_insn) {
		w_mepc(epc + insn_len(epc));
		uart_puts("-> skip fault insn, resume\r\n\r\n");
		return;
	}

	uart_puts("-> halt\r\n");
	for (;;)
		__asm__ volatile("wfi");
}
