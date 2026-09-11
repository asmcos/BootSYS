#ifndef SHMEM_LOG_H
#define SHMEM_LOG_H

#include <stdint.h>
#include "memmap.h"

/*
 * Line 0  SHMEM+0     alive / cpu1_done   (CPU1 stores only)
 * Line 1  SHMEM+64    go / loops          (CPU0 stores; CPU1 clears go)
 * Line 2+ SHMEM+128   log ring            (CPU1 write, CPU0 read)
 *
 * Consumers must thead_flush_caches() before read — range flush is clean-only.
 */
#define SHMEM_CMD_BASE (SHMEM_BASE + 64UL)
#define SHMEM_LOG_BASE (SHMEM_BASE + 128UL)

#define LOG_SLOTS 16
#define LOG_WIDTH 48

struct shmem_ctrl {
	volatile uint32_t alive;
	volatile uint32_t cpu1_done;
	volatile uint32_t cpuid; /* bootsys_cpuid: 1 on big core */
	volatile uint32_t go_seen;
};

#define CMD_MODE_BATCH  0u /* dual threads + timer/MSIP */
#define CMD_MODE_PROBE  1u /* message ring only, no scheduler */

struct shmem_cmd {
	volatile uint32_t go;
	volatile uint32_t loops;
	volatile uint32_t mode;
};

struct shmem_log {
	volatile uint32_t w;
	volatile uint32_t r;
	char slot[LOG_SLOTS][LOG_WIDTH];
};

static inline volatile struct shmem_ctrl *shmem_ctrl(void)
{
	return (volatile struct shmem_ctrl *)SHMEM_BASE;
}

static inline volatile struct shmem_cmd *shmem_cmd(void)
{
	return (volatile struct shmem_cmd *)SHMEM_CMD_BASE;
}

static inline volatile struct shmem_log *shmem_log(void)
{
	return (volatile struct shmem_log *)SHMEM_LOG_BASE;
}

void shmem_log_reset(void);
void shmem_log_putc_line(const char *s); /* raw enqueue (CPU1) */
void shmem_log_drain(void);              /* CPU0 → UART (few lines) */
void shmem_log_drain_all(void);          /* CPU0 → empty ring */
void shmem_cmd_kick(uint32_t loops);
void shmem_cmd_kick_mode(uint32_t loops, uint32_t mode);
void shmem_cmd_clear_go(void);
int shmem_cpu1_batch_done(void);
uint32_t shmem_cmd_loops(void);
uint32_t shmem_cmd_mode(void);

/*
 * CPU1 message channel — big core must NOT call uart_*.
 * Lines are prefixed [cpu1] and printed by CPU0 via drain.
 */
void cpu1_msg(const char *text);
void cpu1_msg_worker(uint32_t tid, uint32_t n, uint32_t max, uint32_t tick);

#endif
