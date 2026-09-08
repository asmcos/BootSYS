#include "k230.h"

/*
 * CLINT mtime / mtimecmp — 32-bit MMIO only (dts: clint,has-no-64bit-mmio).
 */

uint64_t clint_mtime(void)
{
	uint32_t hi, lo, hi2;
	uintptr_t base = (uintptr_t)CLINT_MTIME;

	do {
		hi = readl(base + 4);
		lo = readl(base);
		hi2 = readl(base + 4);
	} while (hi != hi2);

	return ((uint64_t)hi << 32) | lo;
}

void clint_mtimecmp_write(unsigned hart, uint64_t v)
{
	uintptr_t cmp = (uintptr_t)CLINT_MTIMECMP(hart);

	/* Avoid a torn compare that would fire immediately. */
	writel(0xffffffffu, cmp);
	writel((uint32_t)(v >> 32), cmp + 4);
	writel((uint32_t)v, cmp);
	fence_rw();
}
