#ifndef SHMEM_LOG_H
#define SHMEM_LOG_H

#include <stdint.h>
#include "memmap.h"

/*
 * Line 0  SHMEM+0     ctrl (CPU1 stores)
 * Line 1  SHMEM+64    cmd  (CPU0 stores; CPU1 clears go)
 * Line 2+ SHMEM+128   print ring (see print.h) — both enqueue, CPU0 drains
 */

#define SHMEM_CMD_BASE (SHMEM_BASE + 64UL)

struct shmem_ctrl {
	volatile uint32_t alive;
	volatile uint32_t cpu1_done;
	volatile uint32_t cpuid;
	volatile uint32_t go_seen;
};

#define CMD_MODE_BATCH  0u
#define CMD_MODE_PROBE  1u

struct shmem_cmd {
	volatile uint32_t go;
	volatile uint32_t loops;
	volatile uint32_t mode;
};

static inline volatile struct shmem_ctrl *shmem_ctrl(void)
{
	return (volatile struct shmem_ctrl *)SHMEM_BASE;
}

static inline volatile struct shmem_cmd *shmem_cmd(void)
{
	return (volatile struct shmem_cmd *)SHMEM_CMD_BASE;
}

void shmem_cmd_kick(uint32_t loops);
void shmem_cmd_kick_mode(uint32_t loops, uint32_t mode);
void shmem_cmd_clear_go(void);
int shmem_cpu1_batch_done(void);
uint32_t shmem_cmd_loops(void);
uint32_t shmem_cmd_mode(void);

#endif
