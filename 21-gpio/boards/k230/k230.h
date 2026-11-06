#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE     0x91400000UL
#define UART_CLOCK_HZ  48600000U
#define UART_BAUD      115200U

#define SYSCTL_CLK_BASE       0x91100000UL
#define SYSCTL_CLK_GATE0      (SYSCTL_CLK_BASE + 0x24UL)
#define SYSCTL_CLK_GATE1      (SYSCTL_CLK_BASE + 0x50UL)
#define CLK_BIT_GPIO_APB      (1u << 11)
#define CLK_BIT_GPIO_DB       (1u << 27)
#define CLK_BIT_IOMUX_APB     (1u << 20)

#define IOMUX_BASE            0x91105000UL
#define PMU_IOMUX_BASE        0x91000080UL
#define IOMUX_NPINS           64u
#define PMU_IOMUX_NPINS       8u

#define PINMUX_DEMO_PIN       20u
#define PINMUX_UART0_TX       38u
#define PINMUX_UART0_RX       39u

/* DW_apb_gpio — GPIO0 pins 0–31, GPIO1 pins 32–63. */
#define GPIO0_BASE            0x9140b000UL
#define GPIO1_BASE            0x9140c000UL

#define PLIC_BASE             0x0f00000000ULL
#define PLIC_CTX_M            0u
#define GPIO0_PLIC_BASE       32u
#define GPIO1_PLIC_BASE       64u

#define MIE_MEIE              (1UL << 11)
#define MSTATUS_MIE           (1UL << 3)
#define MCAUSE_MEI            11u
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

void plic_enable(uint32_t src, int on);
void plic_set_priority(uint32_t src, uint32_t prio);
uint32_t plic_claim(void);
void plic_complete(uint32_t src);

#endif
