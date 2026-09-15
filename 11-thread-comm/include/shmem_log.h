#ifndef SHMEM_LOG_H
#define SHMEM_LOG_H

#include <stdint.h>
#include "memmap.h"

/*
 * +0      ctrl (CPU1)
 * +64     cmd  (CPU0)
 * +128    mbox (see mbox.h)
 * +0x800  print ring (see print.h)
 */

#define SHMEM_CMD_BASE (SHMEM_BASE + 64UL)

struct shmem_ctrl {
	volatile uint32_t alive;
	volatile uint32_t cpu1_done;
	volatile uint32_t cpuid;
	volatile uint32_t go_seen;
	volatile uint32_t rx_ok;   /* CPU1: good receives */
	volatile uint32_t rx_bad;  /* CPU1: empty/not-ready under bad mode */
};

#define CMD_MODE_PROBE   1u /* print ring only */
#define CMD_MODE_MBOX_OK 2u /* mailbox + flush/pull */
#define CMD_MODE_MBOX_BAD 3u /* mailbox without cohere */
#define CMD_MODE_PINGPONG 4u /* request/response */
#define CMD_MODE_CACHE_PROBE 5u /* HW L1 coherency probe */

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
