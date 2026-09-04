/* Exception dump + skip-faulting-insn resume. */
#include "k230.h"
#include "trap.h"

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

static inline uint64_t r_mtvec(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mtvec" : "=r"(v));
	return v;
}

static inline uint64_t r_misa(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, misa" : "=r"(v));
	return v;
}

static const char *cause_name(uint64_t cause)
{
	int interrupt = (cause >> 63) & 1;
	uint64_t code = cause & 0xfffu;

	if (interrupt)
		return "interrupt";
	switch (code) {
	case 0:
		return "Instruction address misaligned";
	case 1:
		return "Instruction access fault";
	case 2:
		return "Illegal instruction";
	case 3:
		return "Breakpoint";
	case 4:
		return "Load address misaligned";
	case 5:
		return "Load access fault";
	case 6:
		return "Store/AMO address misaligned";
	case 7:
		return "Store/AMO access fault";
	case 8:
		return "Environment call from U-mode";
	case 9:
		return "Environment call from S-mode";
	case 11:
		return "Environment call from M-mode";
	case 12:
		return "Instruction page fault";
	case 13:
		return "Load page fault";
	case 15:
		return "Store/AMO page fault";
	default:
		return "Unknown";
	}
}

static unsigned insn_len(uint64_t mepc)
{
	uint16_t half = *(volatile uint16_t *)(uintptr_t)mepc;
	/* RVC: low 2 bits != 11 */
	if ((half & 0x3u) != 0x3u)
		return 2;
	return 4;
}

void trap_init(void)
{
	extern void trap_entry(void);
	uintptr_t ent = (uintptr_t)trap_entry;

	__asm__ volatile("csrw mtvec, %0" ::"r"(ent));
	trap_skip_insn = 1;
}

void trap_handler(struct trap_frame *tf)
{
	uint64_t cause = r_mcause();
	uint64_t epc = r_mepc();
	uint64_t tval = r_mtval();
	uint64_t code = cause & 0xfffu;
	int interrupt = (int)((cause >> 63) & 1);

	uart_puts("\r\n!!!! EXCEPTION !!!!\r\n");
	uart_puts(cause_name(cause));
	uart_puts("\r\n");

	put_kv64("mhartid", mhartid());
	put_kv64("mcause ", cause);
	put_kv64("  code ", code);
	uart_puts("  irq?  = ");
	uart_puts(interrupt ? "yes\r\n" : "no\r\n");
	put_kv64("mepc   ", epc);
	put_kv64("mtval  ", tval);
	put_kv64("mstatus", r_mstatus());
	put_kv64("mtvec  ", r_mtvec());
	put_kv64("misa   ", r_misa());

	uart_puts("---- GPRs (at trap) ----\r\n");
	put_kv64("ra ", tf->ra);
	put_kv64("sp ", tf->sp);
	put_kv64("gp ", tf->gp);
	put_kv64("tp ", tf->tp);
	put_kv64("t0 ", tf->t0);
	put_kv64("t1 ", tf->t1);
	put_kv64("t2 ", tf->t2);
	put_kv64("s0 ", tf->s0);
	put_kv64("s1 ", tf->s1);
	put_kv64("a0 ", tf->a0);
	put_kv64("a1 ", tf->a1);
	put_kv64("a2 ", tf->a2);
	put_kv64("a3 ", tf->a3);
	put_kv64("a4 ", tf->a4);
	put_kv64("a5 ", tf->a5);
	put_kv64("a6 ", tf->a6);
	put_kv64("a7 ", tf->a7);
	put_kv64("s2 ", tf->s2);
	put_kv64("s3 ", tf->s3);
	put_kv64("t3 ", tf->t3);
	put_kv64("t4 ", tf->t4);
	put_kv64("t5 ", tf->t5);
	put_kv64("t6 ", tf->t6);

	if (!interrupt && trap_skip_insn) {
		unsigned len = insn_len(epc);
		w_mepc(epc + len);
		uart_puts("-> skip fault insn, mepc+=");
		uart_putdec32(len);
		uart_puts(", resume\r\n\r\n");
		return;
	}

	uart_puts("-> halt\r\n");
	for (;;)
		__asm__ volatile("wfi");
}
