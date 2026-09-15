#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>
#include "memmap.h"

/*
 * Shared print ring @ SHMEM+128 — both CPUs enqueue; only CPU0 drains to UART.
 *
 *   print_puts / print_printf  →  ring
 *   print_drain_isr()          →  CPU0 timer/ecall path (few lines)
 *   print_drain_all()          →  idle / menu wait loops
 */

#define PRINT_BASE   (SHMEM_BASE + 0x800UL)
#define PRINT_SLOTS  24u
#define PRINT_WIDTH  64u
#define PRINT_ISR_MAX 3u

void print_init(void);
void print_puts(const char *s);
void print_printf(const char *fmt, ...);
void print_drain_isr(void);  /* CPU0 only; budget PRINT_ISR_MAX lines */
void print_drain_all(void);  /* CPU0 only */
uint32_t print_pending(void);

#endif
