#include "k230.h"

#define SYSCTL_STC_BASE 0x91108000UL

void sysctl_early_init(void)
{
	uint32_t v;

	/* U-Boot harts_early_init() — enable CPU0/CPU1 timer clocks. */
	writel(0x1, SYSCTL_STC_BASE + 0x20);
	writel(0x1, SYSCTL_STC_BASE + 0x30);
	writel(0x69, SYSCTL_STC_BASE + 0x00);

	/* IOMUX APB @ 0x91100050 bit20; GPIO APB/debounce @ 0x91100024. */
	v = readl(SYSCTL_CLK_GATE1);
	v |= CLK_BIT_IOMUX_APB;
	writel(v, SYSCTL_CLK_GATE1);

	v = readl(SYSCTL_CLK_GATE0);
	v |= CLK_BIT_GPIO_APB | CLK_BIT_GPIO_DB;
	writel(v, SYSCTL_CLK_GATE0);

	fence_rw();
}
