/* CPU1 wake-research main. */
#include "k230.h"
#include "memmap.h"

#define MIE_MSIE    (1ULL << 3)
#define MSTATUS_MIE (1ULL << 3)
#define MIP_MSIP    (1ULL << 3)

static void csr_set_mie_msie(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(MIE_MSIE));
}

static void csr_clear_mstatus_mie(void)
{
	__asm__ volatile("csrc mstatus, %0" ::"r"(MSTATUS_MIE));
}

static uint64_t csr_mip(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mip" : "=r"(v));
	return v;
}

static void delay_loop(volatile uint32_t n)
{
	while (n--)
		__asm__ volatile("" ::: "memory");
}

static void delay_ms(uint32_t ms)
{
	uint64_t start = mcycle();
	uint64_t ticks = (uint64_t)ms * 2000ULL * 1000ULL;
	while (mcycle() - start < ticks)
		;
}

static void wait_console(void)
{
	for (;;) {
		thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
		if (shmem_console_ready())
			return;
		delay_ms(10);
	}
}

void hart1_main(void)
{
	uint32_t n = 0;
	uint32_t entry = (uint32_t)(uintptr_t)_hart1_entry;
	uint32_t mode = CPU1_STATE_RUNNING;

	shmem_cpu1_mark_alive(entry);
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
	csr_set_mie_msie();
	csr_clear_mstatus_mie();
	clint_ipi_clear_cpu1();

	/* Hold UART until CPU0 finishes banner / menu. */
	wait_console();

	for (;;) {
		uint32_t cmd = cpu1_shmem->cmd;

		if (cmd != CPU1_CMD_NONE) {
			cpu1_shmem->cmd = CPU1_CMD_NONE;
			fence_rw();
			if (cmd == CPU1_CMD_GO_RUNNING)
				mode = CPU1_STATE_RUNNING;
			else if (cmd == CPU1_CMD_GO_SOFT_LOOP)
				mode = CPU1_STATE_SOFT_LOOP;
			else if (cmd == CPU1_CMD_GO_WFI)
				mode = CPU1_STATE_WFI_SLEEP;
		}

		if (mode == CPU1_STATE_WFI_SLEEP) {
			shmem_cpu1_set_state(CPU1_STATE_WFI_SLEEP);
			uart_puts("[cpu1] wfi\r\n");
			clint_ipi_clear_cpu1();
			fence_rw();
			__asm__ volatile("wfi");
			clint_ipi_clear_cpu1();
			shmem_cpu1_on_wake();
			uart_puts("[cpu1] wake\r\n");
			mode = CPU1_STATE_RUNNING;
			continue;
		}

		if (mode == CPU1_STATE_SOFT_LOOP) {
			shmem_cpu1_set_state(CPU1_STATE_SOFT_LOOP);
			if (clint_msip_get(1) || (csr_mip() & MIP_MSIP)) {
				shmem_cpu1_on_ipi_seen();
				clint_ipi_clear_cpu1();
				uart_puts("[cpu1] ipi\r\n");
			}
			delay_loop(100000U);
			continue;
		}

		shmem_cpu1_set_state(CPU1_STATE_RUNNING);
		shmem_cpu1_mark_tick(n);
		uart_puts("[cpu1] tick ");
		uart_puthex32(n);
		uart_puts("\r\n");
		n++;
		for (uint32_t i = 0; i < 40; i++) {
			if (cpu1_shmem->cmd != CPU1_CMD_NONE)
				break;
			delay_ms(50);
		}
	}
}
