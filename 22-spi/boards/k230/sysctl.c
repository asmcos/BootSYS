#include "k230.h"

#define SYSCTL_STC_BASE 0x91108000UL

void sysctl_early_init(void)
{
	uint32_t v;

	writel(0x1, SYSCTL_STC_BASE + 0x20);
	writel(0x1, SYSCTL_STC_BASE + 0x30);
	writel(0x69, SYSCTL_STC_BASE + 0x00);

	v = readl(SYSCTL_CLK_GATE1);
	v |= CLK_BIT_IOMUX_APB;
	writel(v, SYSCTL_CLK_GATE1);

	v = readl(SYSCTL_CLK_GATE0);
	v |= CLK_BIT_GPIO_APB | CLK_BIT_GPIO_DB;
	writel(v, SYSCTL_CLK_GATE0);

	/* QSPI0 / SSI1 clocks */
	v = readl(SYSCTL_CLK_GATE_HS);
	v |= CLK_BIT_SSI1_AHB | CLK_BIT_SSI1 | CLK_BIT_QSPI_AXI_SRC | CLK_BIT_SSI1_AXI;
	writel(v, SYSCTL_CLK_GATE_HS);

	fence_rw();
}
