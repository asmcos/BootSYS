#include "k230.h"
#include "trap.h"
#include "priv.h"

volatile int trap_expect;
volatile int trap_quiet;
volatile int trap_saw;
volatile uint64_t trap_last_cause;
volatile uint64_t trap_last_mpp;

void trap_jump_shell(void (*entry)(void), unsigned long mpp_bits);

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

static inline void w_mstatus(uint64_t v)
{
	__asm__ volatile("csrw mstatus, %0" ::"r"(v));
}

static const char *cause_name(uint64_t cause)
{
	uint64_t code = cause & 0xfffu;

	if ((cause >> 63) & 1)
		return "interrupt";
	switch (code) {
	case 1:
		return "Instruction access fault (check PMP)";
	case 2:
		return "Illegal instruction";
	case 5:
		return "Load access fault";
	case 7:
		return "Store access fault";
	case 8:
		return "ECALL from U-mode";
	case 9:
		return "ECALL from S-mode";
	case 11:
		return "ECALL from M-mode";
	default:
		return "Other";
	}
}

static const char *mpp_name(uint64_t mstatus)
{
	switch ((mstatus >> 11) & 3u) {
	case 3:
		return "M";
	case 1:
		return "S";
	case 0:
		return "U";
	default:
		return "?";
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
	trap_expect = 0;
	trap_quiet = 0;
	trap_saw = 0;
}

void trap_handler(struct trap_frame *tf)
{
	uint64_t cause = r_mcause();
	uint64_t epc = r_mepc();
	uint64_t code = cause & 0xfffu;
	int interrupt = (int)((cause >> 63) & 1);
	uint64_t ms = r_mstatus();

	if (!interrupt && (code == 8 || code == 9 || code == 11) &&
	    !trap_expect) {
		unsigned long req = (unsigned long)tf->a0;

		/* Raise current privilege to M, then continue after ecall. */
		if (req == ECALL_ENTER_M) {
			ms &= ~((uint64_t)3 << 11);
			ms |= MSTATUS_MPP_M;
			w_mstatus(ms);
			w_mepc(epc + insn_len(epc));
			g_cur_mode = PRIV_M;
			return;
		}

		if (req == ECALL_GOTO_M) {
			uart_puts("\r\n[trap] ecall -> M-mode\r\n");
			g_cur_mode = PRIV_M;
			trap_jump_shell(shell_m, MSTATUS_MPP_M);
		}
		if (req == ECALL_GOTO_S) {
			uart_puts("\r\n[trap] ecall -> S-mode\r\n");
			g_cur_mode = PRIV_S;
			trap_jump_shell(shell_s, MSTATUS_MPP_S);
		}
		if (req == ECALL_GOTO_U) {
			uart_puts("\r\n[trap] ecall -> U-mode\r\n");
			g_cur_mode = PRIV_U;
			trap_jump_shell(shell_u, MSTATUS_MPP_U);
		}
	}

	if (!interrupt && trap_expect) {
		trap_saw = 1;
		trap_last_cause = cause;
		trap_last_mpp = (ms >> 11) & 3u;
		w_mepc(epc + insn_len(epc));
		if (trap_quiet)
			return;

		uart_puts("\r\n  !! TRAP\r\n");
		uart_puts("     type : ");
		uart_puts(cause_name(cause));
		uart_puts("\r\n");
		put_kv64("mcause", cause);
		put_kv64("mepc  ", epc);
		put_kv64("mtval ", r_mtval());
		uart_puts("     mstatus.MPP = ");
		uart_putdec32((uint32_t)trap_last_mpp);
		uart_puts(" (");
		uart_puts(mpp_name(ms));
		uart_puts(")  <- privilege BEFORE this trap\r\n");
		uart_puts("     action: skip insn, return same privilege\r\n");
		return;
	}

	uart_puts("\r\n!!!! UNEXPECTED TRAP !!!!\r\n");
	uart_puts(cause_name(cause));
	uart_puts("\r\n");
	put_kv64("mcause", cause);
	put_kv64("mepc  ", epc);
	put_kv64("mtval ", r_mtval());
	put_kv64("mstatus", ms);
	w_mepc(epc + insn_len(epc));
	uart_puts("-> skip and continue\r\n");
}
