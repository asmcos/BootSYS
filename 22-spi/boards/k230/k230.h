#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE     0x91400000UL
#define UART_CLOCK_HZ  48600000U
#define UART_BAUD      115200U

#define SYSCTL_CLK_BASE       0x91100000UL
#define SYSCTL_CLK_GATE0      (SYSCTL_CLK_BASE + 0x24UL)
#define SYSCTL_CLK_GATE1      (SYSCTL_CLK_BASE + 0x50UL)
#define SYSCTL_CLK_GATE_HS    (SYSCTL_CLK_BASE + 0x18UL) /* hs_* gates */
#define CLK_BIT_GPIO_APB      (1u << 11)
#define CLK_BIT_GPIO_DB       (1u << 27)
#define CLK_BIT_IOMUX_APB     (1u << 20)
/* QSPI0 = SSI1 @ 0x91582000 */
#define CLK_BIT_SSI1_AHB      (1u << 7)
#define CLK_BIT_SSI1          (1u << 25)
#define CLK_BIT_QSPI_AXI_SRC  (1u << 28)
#define CLK_BIT_SSI1_AXI      (1u << 29)

#define IOMUX_BASE            0x91105000UL
#define PMU_IOMUX_BASE        0x91000080UL
#define IOMUX_NPINS           64u
#define PMU_IOMUX_NPINS       8u

#define PINMUX_UART0_TX       38u
#define PINMUX_UART0_RX       39u

/* Default flash wiring (CanMV / Lushan examples): io14–17 → QSPI0 */
#define SPI_PIN_CS            14u
#define SPI_PIN_CLK           15u
#define SPI_PIN_MOSI          16u /* QSPI0_D0 */
#define SPI_PIN_MISO          17u /* QSPI0_D1 */
#define SPI_PIN_SEL_QSPI0     3u  /* IO_SEL for QSPI0_* on io14–17 */

#define GPIO0_BASE            0x9140b000UL
#define GPIO1_BASE            0x9140c000UL

/* spi1 / QSPI0 — DW_apb_ssi */
#define QSPI0_BASE            0x91582000UL

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
