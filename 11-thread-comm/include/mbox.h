#ifndef MBOX_H
#define MBOX_H

#include <stdint.h>
#include "memmap.h"

/*
 * Dual mailboxes in shmem (after ctrl/cmd, before print ring):
 *   c0_to_c1  — CPU0 producer, CPU1 consumer
 *   c1_to_c0  — CPU1 producer, CPU0 consumer
 *
 * Lesson point: payload + ready + D-cache push/pull. No AMO locks (→12).
 */

#define MBOX_BASE     (SHMEM_BASE + 128UL)
#define MBOX_CAP      8u
#define MBOX_PAYLOAD  32u

struct mbox_slot {
	volatile uint32_t ready; /* 1 = readable */
	uint32_t seq;
	uint32_t len;
	char data[MBOX_PAYLOAD];
};

struct mbox {
	volatile uint32_t w;
	volatile uint32_t r;
	volatile uint32_t drops; /* send failed (full) */
	volatile uint32_t pad;
	struct mbox_slot slot[MBOX_CAP];
};

struct mbox_pair {
	struct mbox c0_to_c1;
	struct mbox c1_to_c0;
};

static inline volatile struct mbox_pair *mbox_pair(void)
{
	return (volatile struct mbox_pair *)MBOX_BASE;
}

void mbox_init(void);

/* do_cohere: 1 = fence+flush/invalidate (good); 0 = skip (bad demo). */
int mbox_send(volatile struct mbox *m, const void *data, uint32_t len,
	      int do_cohere);
/* returns length, 0 if empty, -1 if slot not ready yet */
int mbox_recv(volatile struct mbox *m, void *data, uint32_t max,
	      int do_cohere);

uint32_t mbox_pending(volatile struct mbox *m, int do_cohere);

#endif
