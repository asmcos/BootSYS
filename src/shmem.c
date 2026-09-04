#include "k230.h"
#include "memmap.h"

#define CPU1_TICK_MAGIC 0xC0017100u

volatile struct cpu1_shmem *const cpu1_shmem =
	(volatile struct cpu1_shmem *)SHMEM_BASE;

void shmem_init(void)
{
	cpu1_shmem->alive = 0;
	cpu1_shmem->tick = 0;
	cpu1_shmem->entry_pc = 0;
	cpu1_shmem->state = CPU1_STATE_RESET;
	cpu1_shmem->cmd = CPU1_CMD_NONE;
	cpu1_shmem->wake_count = 0;
	cpu1_shmem->ipi_seen = 0;
	cpu1_shmem->seq = 0;
	cpu1_shmem->console_ready = 0;
	fence_rw();
}

int shmem_cpu1_alive(void)
{
	return cpu1_shmem->alive == CPU1_ALIVE_MAGIC;
}

uint32_t shmem_cpu1_state(void)
{
	return cpu1_shmem->state;
}

const char *shmem_state_name(uint32_t st)
{
	switch (st) {
	case CPU1_STATE_RESET:
		return "RESET";
	case CPU1_STATE_RUNNING:
		return "RUNNING";
	case CPU1_STATE_SOFT_LOOP:
		return "SOFT_LOOP";
	case CPU1_STATE_WFI_SLEEP:
		return "WFI_SLEEP";
	default:
		return "UNKNOWN";
	}
}

void shmem_cpu0_cmd(uint32_t cmd)
{
	cpu1_shmem->cmd = cmd;
	fence_rw();
}

void shmem_cpu1_mark_alive(uint32_t entry_pc)
{
	cpu1_shmem->entry_pc = entry_pc;
	fence_rw();
	cpu1_shmem->alive = CPU1_ALIVE_MAGIC;
	cpu1_shmem->state = CPU1_STATE_RUNNING;
	fence_rw();
}

void shmem_cpu1_set_state(uint32_t st)
{
	cpu1_shmem->state = st;
	cpu1_shmem->seq = cpu1_shmem->seq + 1;
	fence_rw();
}

void shmem_cpu1_mark_tick(uint32_t n)
{
	cpu1_shmem->tick = CPU1_TICK_MAGIC | (n & 0xffffu);
	fence_rw();
}

void shmem_cpu1_on_wake(void)
{
	cpu1_shmem->wake_count = cpu1_shmem->wake_count + 1;
	cpu1_shmem->state = CPU1_STATE_RUNNING;
	fence_rw();
}

void shmem_cpu1_on_ipi_seen(void)
{
	cpu1_shmem->ipi_seen = cpu1_shmem->ipi_seen + 1;
	fence_rw();
}

void shmem_cpu0_console_ready(void)
{
	cpu1_shmem->console_ready = 1;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
}

int shmem_console_ready(void)
{
	return cpu1_shmem->console_ready != 0;
}
