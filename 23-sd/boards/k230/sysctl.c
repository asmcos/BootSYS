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

	/* SD1: AHB + shared sd_* + per-controller a/b/c/tm clocks. */
	v = readl(SYSCTL_CLK_GATE_HS);
	v |= CLK_BIT_HS_HCLK | CLK_BIT_SD1_HCLK |
	     CLK_BIT_SD_ACLK | CLK_BIT_SD_BCLK | CLK_BIT_SD_CCLK | CLK_BIT_SD_TMCLK |
	     CLK_BIT_SD1_ACLK | CLK_BIT_SD1_BCLK | CLK_BIT_SD1_CCLK | CLK_BIT_SD1_TMCLK;
	writel(v, SYSCTL_CLK_GATE_HS);

	/*
	 * hs_SDCLK_CFG sd_cclk_div[14:12] reset 1 → divide by 2.
	 * pll0/4 (~400 MHz) / 2 ≈ 200 MHz into the SDHCI base clock.
	 */
	v = readl(SYSCTL_HS_SDCLK);
	v &= ~(7u << 12);
	v |= (1u << 12);
	v |= (1u << 31); /* sd_aclk update; cclk div is in the same reg */
	writel(v, SYSCTL_HS_SDCLK);

	fence_rw();
}
