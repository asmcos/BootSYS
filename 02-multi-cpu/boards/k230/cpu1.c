#include "k230.h"

#define CPU1_PWR_LPI_CTL   0x91103018UL
#define CPU1_PWR_LPI_STATE 0x9110301cUL

#define RST_BIT_RESET  (1u << 0)
#define RST_BIT_DONE   (1u << 12)

static uint32_t rst_masked(void)
{
	return readl(CPU1_RST_CTL) & 0x1;
}

static void rst_write(uint32_t data)
{
	writel(data, CPU1_RST_CTL);
}

void cpu1_power_on(void)
{
	if (readl(CPU1_PWR_LPI_STATE) & (1u << 1))
		return;

	writel((1u << 1) | (1u << 17), CPU1_PWR_LPI_CTL);
	for (int i = 0; i < 2000; i++) {
		if (readl(CPU1_PWR_LPI_STATE) & (1u << 1))
			break;
		udelay(50);
	}
	writel((1u << 5) | (1u << 21), CPU1_PWR_LPI_CTL);
	udelay(500);
}

void cpu1_l2_flush(void)
{
	uint32_t data = rst_masked() | (1u << 4) | (1u << 20);

	rst_write(data);
	for (int i = 0; i < 10000; i++) {
		if ((readl(CPU1_RST_CTL) & (1u << 4)) == 0)
			return;
		udelay(100);
	}
	uart_puts("cpu0: WARN cpu1 l2 flush timeout\r\n");
}

static int force_hold_reset(void)
{
	uint32_t data;
	int i;

	if (readl(CPU1_RST_CTL) & RST_BIT_RESET)
		return 1;

	data = rst_masked() | RST_BIT_RESET | (1u << 16);
	rst_write(data);

	for (i = 0; i < 500; i++) {
		if (readl(CPU1_RST_CTL) & RST_BIT_RESET)
			return 1;
		udelay(100);
	}
	return 0;
}

/*
 * U-Boot de_reset_big_core — no MMIO L2 flush (bit4 can stick / wedge).
 */
struct cpu1_boot_result cpu1_boot(uint32_t entry)
{
	struct cpu1_boot_result r = { 0 };
	uint32_t data;

	r.pre_ctl = readl(CPU1_RST_CTL);

	cpu1_power_on();
	r.pre_hold_ok = force_hold_reset();

	writel(entry, CPU1_HART_RSTVEC);
	udelay(100);

	data = rst_masked() | RST_BIT_DONE | (1u << 28);
	rst_write(data);
	udelay(100);

	data = rst_masked() | RST_BIT_RESET | (1u << 16);
	rst_write(data);
	udelay(2000);
	r.assert_ctl = readl(CPU1_RST_CTL);
	r.assert_ok = (r.assert_ctl & RST_BIT_RESET) != 0;

	data = (rst_masked() & ~RST_BIT_RESET) | (1u << 16);
	rst_write(data);
	udelay(2000);
	r.post_ctl = readl(CPU1_RST_CTL);
	r.released = (r.post_ctl & RST_BIT_RESET) == 0;

	return r;
}

void cpu1_hold_reset(void)
{
	uint32_t data = rst_masked() | RST_BIT_RESET | (1u << 16);

	rst_write(data);
	udelay(1000);
}
