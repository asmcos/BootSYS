#ifndef UART_IRQ_H
#define UART_IRQ_H

#include <stdint.h>

void uart_irq_init(void);
void uart_irq_on(void);
void uart_irq_off(void);
void uart_irq_on_mei(void); /* trap: machine external interrupt */

int uart_irq_mode(void);
uint32_t uart_irq_count(void);
int uart_irq_getc(void); /* from ring; -1 empty */

#endif
