#include "k230.h"

#define SYSCTL_STC_BASE 0x91108000UL

void sysctl_early_init(void)
{
	/* U-Boot harts_early_init() — enable CPU0/CPU1 timer clocks. */
	writel(0x1, SYSCTL_STC_BASE + 0x20);
	writel(0x1, SYSCTL_STC_BASE + 0x30);
	writel(0x69, SYSCTL_STC_BASE + 0x00);
}
