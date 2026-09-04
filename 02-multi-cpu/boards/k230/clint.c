#include "k230.h"

/*
 * CLINT MSIP — software IPI between harts.
 * K230: physical base 0xf04000000 (see linux k230.dtsi).
 *
 * Note: IPI only wakes a hart that is already out of RMU reset.
 * If CPU1 is in hard reset, MSIP is useless — must RMU deassert first.
 */

void clint_msip_set(unsigned hart, int pending)
{
	writel(pending ? 1u : 0u, (uintptr_t)CLINT_MSIP(hart));
	fence_rw();
}

uint32_t clint_msip_get(unsigned hart)
{
	return readl((uintptr_t)CLINT_MSIP(hart));
}

void clint_ipi_send_cpu1(void)
{
	clint_msip_set(1, 1);
}

void clint_ipi_clear_cpu1(void)
{
	clint_msip_set(1, 0);
}
