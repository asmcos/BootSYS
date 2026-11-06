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
/* SD1 @ 0x91581000. Gates in hs_CLKen_CFG (TRM, reset 1). */
#define CLK_BIT_HS_HCLK       (1u << 0)
#define CLK_BIT_SD1_HCLK      (1u << 3)
#define CLK_BIT_SD_ACLK       (1u << 9)
#define CLK_BIT_SD_BCLK       (1u << 10)
#define CLK_BIT_SD_CCLK       (1u << 11)
#define CLK_BIT_SD_TMCLK      (1u << 12)
#define CLK_BIT_SD1_ACLK      (1u << 17)
#define CLK_BIT_SD1_BCLK      (1u << 18)
#define CLK_BIT_SD1_CCLK      (1u << 19)
#define CLK_BIT_SD1_TMCLK     (1u << 20)
#define SYSCTL_HS_SDCLK       (SYSCTL_CLK_BASE + 0x1cUL)

/* TF card on 嘉立创庐山派 / CanMV: MMC1, IO_SEL=2 (U-Boot k230_canmv_lckfb.dts). */
#define SD1_BASE              0x91581000UL
/* HS sys: Linux sdhci-of-kendryte.c ORs bits 0 and 2 for the SD1 host. */
#define HS_SYS_BASE           0x91585000UL
#define HS_SD1_CTRL           (HS_SYS_BASE + 0x08UL)
#define SD_PIN_CMD            54u
#define SD_PIN_CLK            55u
#define SD_PIN_D0             56u
#define SD_PIN_D1             57u
#define SD_PIN_D2             58u
#define SD_PIN_D3             59u
#define SD_PIN_SEL            2u

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
