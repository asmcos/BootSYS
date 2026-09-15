#include "k230.h"

/*
 * CLINT mtime / mtimecmp / MSIP — 32-bit MMIO only (dts: clint,has-no-64bit-mmio).
 * Big-core MTIP is unreliable on some K230 bring-ups; MSIP[1] is proven in 02.
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

void clint_msip_set(unsigned hart, int pending)
{
	writel(pending ? 1u : 0u, (uintptr_t)CLINT_MSIP(hart));
	fence_rw();
}

uint32_t clint_msip_get(unsigned hart)
{
	return readl((uintptr_t)CLINT_MSIP(hart));
}
