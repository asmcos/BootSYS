#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define UART0_BASE    0x91400000UL
#define UART_CLOCK_HZ 48600000U
#define UART_BAUD     115200U
#define LOAD_BASE     0x80200000UL

static inline void writel(uint32_t val, uintptr_t addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static inline uint64_t mhartid(void)
{
	uint64_t id;
	__asm__ volatile("csrr %0, mhartid" : "=r"(id));
	return id;
}

void thead_cpu_init(void);

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex64(uint64_t v);
void uart_puthex32(uint32_t v);
void uart_puthex32_val(uint32_t v);
void uart_putdec32(uint32_t v);
int uart_try_getc(void);

#endif
