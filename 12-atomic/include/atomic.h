#ifndef ATOMIC_H
#define ATOMIC_H

#include <stddef.h>
#include <stdint.h>
#include "memmap.h"

/*
 * Atom arena (same shmem hole as 11's mbox):
 *   +128  lock | counter | reserved  (each own 64B line)
 *
 * Menu t/l/a run on CPU0 with TWO preemptible threads (10-style).
 * Same L1 → D-cache stays ON; amoswap / amoadd work.
 * Dual-hart races (little/big) are NOT coherent — see 11 menu 'c'.
 */

#define ATOM_BASE  (SHMEM_BASE + 128UL)
#define ATOM_BYTES 192u

struct atom_arena {
	volatile uint64_t lock;
	uint8_t pad0[56];
	volatile uint64_t counter;
	uint8_t pad1[56];
	volatile uint64_t reserved;
};

_Static_assert(offsetof(struct atom_arena, counter) == 64,
	       "counter must sit on its own 64B line");
_Static_assert(offsetof(struct atom_arena, reserved) == 128,
	       "reserved must sit on its own 64B line");

static inline volatile struct atom_arena *atom(void)
{
	return (volatile struct atom_arena *)ATOM_BASE;
}

void spin_lock(volatile uint64_t *lock);
void spin_unlock(volatile uint64_t *lock);

void atom_add_plain(volatile uint64_t *p);
void atom_add_locked(volatile uint64_t *lock, volatile uint64_t *p);
void atom_add_amo(volatile uint64_t *p);
void atom_run_adds(uint32_t mode, uint32_t n);

void atom_flush(void);
void atom_reset(void);
void atom_rng_seed(uint32_t seed);
int atom_sc_fails_after_foreign_store(void);

#endif
