#ifndef SHMEM_LOG_H
#define SHMEM_LOG_H

#include <stdint.h>
#include "memmap.h"

/*
 * +0      ctrl (CPU1)
 * +64     cmd  (CPU0)
 * +128    atom arena (see atomic.h)
 * +0x800  print ring (see print.h)
 */

#define SHMEM_CMD_BASE (SHMEM_BASE + 64UL)

struct shmem_ctrl {
	volatile uint32_t alive;
	volatile uint32_t cpu1_done;   /* batch finished (finish_batch) */
	volatile uint32_t cpuid;
	volatile uint32_t go_seen;
	volatile uint32_t atom_armed; /* at race barrier for this epoch */
	volatile uint32_t epoch_ack;  /* cmd.epoch copied at barrier */
	volatile uint32_t add_done;   /* epoch when worker_add finished */
	volatile uint32_t local_adds; /* how many adds this hart actually did */
};

#define CMD_MODE_PROBE   1u
#define CMD_MODE_PLAIN   2u
#define CMD_MODE_LOCK    3u
#define CMD_MODE_AMO     4u
#define CMD_MODE_RESERVE 5u

struct shmem_cmd {
	volatile uint32_t go;
	volatile uint32_t loops;
	volatile uint32_t mode;
	volatile uint32_t poke;       /* RESERVE */
	volatile uint32_t start;      /* release both add loops */
	volatile uint32_t epoch;      /* bumps every kick — kills stale armed */
	volatile uint32_t cpu0_ready; /* CPU0 at barrier */
	volatile uint32_t add_done;   /* epoch when CPU0 worker finished */
	volatile uint32_t local_adds; /* CPU0 adds completed */
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
