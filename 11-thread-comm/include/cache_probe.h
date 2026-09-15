#ifndef CACHE_PROBE_H
#define CACHE_PROBE_H

#include <stdint.h>
#include "memmap.h"

/*
 * Dedicated lines between mbox and print (mbox ends ~+0x360):
 *   +0x700  shared word (the value under test)
 *   +0x740  phase / CPU1 samples / ack  (handshake only)
 *
 * Line-local invalidate on +0x740 lets CPU1 see phase changes without
 * dropping the cached copy of +0x700 — that is the whole point of the test.
 */

#define CACHE_PROBE_DATA (SHMEM_BASE + 0x700UL)
#define CACHE_PROBE_SYNC (SHMEM_BASE + 0x740UL)

#define CACHE_PROBE_MAGIC_A 0xA11A0001u
#define CACHE_PROBE_MAGIC_B 0xB22B0002u

#define CP_PHASE_IDLE   0u
#define CP_PHASE_HAVE_A 1u /* word==A in SRAM; CPU1 may pull+read */
#define CP_PHASE_WROTE_B 3u /* CPU0 wrote B, no data flush */
#define CP_PHASE_FLUSHED_B 5u /* CPU0 flushed B to SRAM */

struct cache_probe_sync {
	volatile uint32_t phase;
	volatile uint32_t c1_ack; /* 1=saw A, 2=raw-after-B, 3=after-pull */
	volatile uint32_t c1_r1;  /* after pull A */
	volatile uint32_t c1_r2;  /* raw load after CPU0 wrote B (no pull) */
	volatile uint32_t c1_r3;  /* after invalidate+load */
	volatile uint32_t c0_r_after_b; /* CPU0 local view after write B */
};

static inline volatile uint32_t *cache_probe_word(void)
{
	return (volatile uint32_t *)CACHE_PROBE_DATA;
}

static inline volatile struct cache_probe_sync *cache_probe_sync(void)
{
	return (volatile struct cache_probe_sync *)CACHE_PROBE_SYNC;
}

void cache_probe_run_cpu0(void);
void cache_probe_run_cpu1(void);

#endif
