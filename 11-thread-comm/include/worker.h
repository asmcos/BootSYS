#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>
#include "k230.h"

#define WORKER_LOOPS_DEFAULT 5u
#define WORKER_PERIOD_MTIME  (CLINT_TIMEBASE_HZ / 2u) /* 0.5s */
#define WORKER_PHASE_MTIME   (WORKER_PERIOD_MTIME / 2u)

void worker_delay(uint32_t loops);
void worker_set_loops(uint32_t n);
uint32_t worker_get_loops(void);

/* CPU0: send on c0_to_c1 (arg = cohere flag as uintptr). */
void worker_sender(void *arg);
/* CPU1: recv on c0_to_c1 */
void worker_receiver(void *arg);
/* Ping-pong roles */
void worker_ping_c0(void *arg);
void worker_pong_c1(void *arg);

#endif
