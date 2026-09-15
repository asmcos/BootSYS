#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>
#include "k230.h"

/* Default N per thread; change anytime with menu 'n'. */
#define WORKER_LOOPS_DEFAULT 2000u

void worker_delay(uint32_t loops);
void worker_set_loops(uint32_t n);
uint32_t worker_get_loops(void);

void worker_set_race_mode(uint32_t mode);
uint32_t worker_local_adds(unsigned tid);

/* CPU0 race thread; arg = tid 0 or 1. */
void worker_add(void *arg);

#endif
