#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>
#include "k230.h"

/* Default print loops per thread (menu `n` can change). */
#define WORKER_LOOPS_DEFAULT 5u

/* Wall-clock spacing via shared mtime (works even if MTIP is dead). */
#define WORKER_PERIOD_MTIME  (CLINT_TIMEBASE_HZ) /* 1s */
#define WORKER_PHASE_MTIME   (WORKER_PERIOD_MTIME / 2u)

void worker_delay(uint32_t loops);
void worker_set_loops(uint32_t n);
uint32_t worker_get_loops(void);
void worker_main(void *arg);

#endif
