#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE     0x91400000UL
#define UART_CLOCK_HZ  48600000U
#define UART_BAUD      115200U

/* Clock gates live in SYSCTL region 0x91100000 (Linux canaan,k230-clk res 1). */
#define SYSCTL_CLK_BASE       0x91100000UL
#define SYSCTL_CLK_GATE0      (SYSCTL_CLK_BASE + 0x24UL) /* ls_* gates */
#define SYSCTL_CLK_GATE1      (SYSCTL_CLK_BASE + 0x50UL) /* sysctl_* gates */
#define CLK_BIT_GPIO_APB      (1u << 11)
#define CLK_BIT_GPIO_DB       (1u << 27)
#define CLK_BIT_IOMUX_APB     (1u << 20)

#define IOMUX_BASE            0x91105000UL
#define PMU_IOMUX_BASE        0x91000080UL
#define IOMUX_NPINS           64u
#define PMU_IOMUX_NPINS       8u

/* Default demo pin: avoid UART0 (38/39) and JTAG (2–6). */
#define PINMUX_DEMO_PIN       20u
#define PINMUX_UART0_TX       38u
#define PINMUX_UART0_RX       39u

#define IER_ERBFI             (1u << 0)

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

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex64(uint64_t v);
void uart_puthex32(uint32_t v);
void uart_puthex32_val(uint32_t v);
void uart_putdec32(uint32_t v);
int uart_try_getc(void);

void sysctl_early_init(void);
void thead_cpu_init(void);
void thead_flush_caches(void);
void thead_flush_range(unsigned long start, unsigned long end);

#endif
