#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE     0x91400000UL
#define UART_CLOCK_HZ  48600000U
#define UART_BAUD      115200U

/* K230 PLIC (thead,c900-plic) + UART0 source 16. Context 0 = hart0 M (MEI). */
#define PLIC_BASE         0x0f00000000ULL
#define UART0_PLIC_SRC    16u
#define PLIC_CTX_M        0u

#define MIE_MEIE          (1UL << 11)
#define MSTATUS_MIE       (1UL << 3)
#define MCAUSE_MEI        11u

#define IER_ERBFI         (1u << 0)

static inline void writel(uint32_t val, uintptr_t addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static inline void fence_rw(void)
{
	__asm__ volatile("fence rw, rw" ::: "memory");
}

static inline uint64_t mhartid(void)
{
	uint64_t id;
	__asm__ volatile("csrr %0, mhartid" : "=r"(id));
	return id;
}

static inline uint64_t mcycle(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mcycle" : "=r"(v));
	return v;
}

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex64(uint64_t v);
void uart_puthex32(uint32_t v);
void uart_puthex32_val(uint32_t v);
void uart_putdec32(uint32_t v);
int uart_try_getc(void);
void uart_rx_irq_enable(int on);
int uart_rx_take(void); /* drain one byte; -1 if empty. ISR use. */

void sysctl_early_init(void);
void thead_cpu_init(void);
void thead_flush_caches(void);
void thead_flush_range(unsigned long start, unsigned long end);

void plic_uart_init(void);
uint32_t plic_claim(void);
void plic_complete(uint32_t src);

#endif
