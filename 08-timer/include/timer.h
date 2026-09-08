#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void);
void timer_start(void);
void timer_pause(void);
void timer_reset(void);
void timer_on_irq(void);

int timer_running(void);
uint32_t timer_ticks(void); /* tenths of a second */

#endif
