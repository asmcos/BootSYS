#include "k230.h"
#include "pinmux.h"
#include "spi.h"

/* DW_apb_ssi */
#define SSI_CTRLR0   0x00
#define SSI_CTRLR1   0x04
#define SSI_SSIENR   0x08
#define SSI_SER      0x10
#define SSI_BAUDR    0x14
#define SSI_TXFTLR   0x18
#define SSI_RXFTLR   0x1c
#define SSI_SR       0x28
#define SSI_IMR      0x2c
#define SSI_DR0      0x60

#define SR_BUSY      (1u << 0)
#define SR_TFNF      (1u << 1)
#define SR_TFE       (1u << 2)
#define SR_RFNE      (1u << 3)

#define CTRLR0_DFS_8     (0x7u << 0)  /* 8-bit frame */
#define CTRLR0_FRF_SPI   (0u << 6)
#define CTRLR0_SCPH      (1u << 8)
#define CTRLR0_SCPOL     (1u << 9)
#define CTRLR0_TMOD_TR   (0u << 10)   /* tx & rx */
#define CTRLR0_SRL       (1u << 13)   /* loopback */

#define GPIO_DR    0x00
#define GPIO_DDR   0x04
#define GPIO_CTL   0x08

static unsigned g_baud_div = 100;
static int g_mode;
static int g_loopback;

static void ssi_wr(unsigned off, uint32_t v)
{
	writel(v, QSPI0_BASE + off);
}

static uint32_t ssi_rd(unsigned off)
{
	return readl(QSPI0_BASE + off);
}

static void gpio0_bit(unsigned bit, int out, int level)
{
	uint32_t ddr, dr;

	writel(0, GPIO0_BASE + GPIO_CTL);
	ddr = readl(GPIO0_BASE + GPIO_DDR);
	if (out)
		ddr |= (1u << bit);
	else
		ddr &= ~(1u << bit);
	writel(ddr, GPIO0_BASE + GPIO_DDR);
	dr = readl(GPIO0_BASE + GPIO_DR);
	if (level)
		dr |= (1u << bit);
	else
		dr &= ~(1u << bit);
	writel(dr, GPIO0_BASE + GPIO_DR);
	fence_rw();
}

void spi_pins_setup(void)
{
	/*
	 * io14 = GPIO CS (soft), io15/16/17 = QSPI0 CLK/D0/D1 (IO_SEL=3).
	 * Matches CanMV flash examples.
	 */
	pinmux_as_gpio(SPI_PIN_CS, 1);
	pinmux_set_ds(SPI_PIN_CS, 8);
	pinmux_set_pull(SPI_PIN_CS, PIN_PULL_UP);

	pinmux_set_sel(SPI_PIN_CLK, SPI_PIN_SEL_QSPI0);
	pinmux_set_ie(SPI_PIN_CLK, 1);
	pinmux_set_oe(SPI_PIN_CLK, 1);
	pinmux_set_ds(SPI_PIN_CLK, 8);

	pinmux_set_sel(SPI_PIN_MOSI, SPI_PIN_SEL_QSPI0);
	pinmux_set_ie(SPI_PIN_MOSI, 1);
	pinmux_set_oe(SPI_PIN_MOSI, 1);
	pinmux_set_ds(SPI_PIN_MOSI, 8);

	pinmux_set_sel(SPI_PIN_MISO, SPI_PIN_SEL_QSPI0);
	pinmux_set_ie(SPI_PIN_MISO, 1);
	pinmux_set_oe(SPI_PIN_MISO, 0);
	pinmux_set_ds(SPI_PIN_MISO, 8);
	pinmux_set_pull(SPI_PIN_MISO, PIN_PULL_UP);

	gpio0_bit(SPI_PIN_CS, 1, 1); /* CS high = idle */
}

void spi_cs(int assert)
{
	/* active low */
	gpio0_bit(SPI_PIN_CS, 1, assert ? 0 : 1);
}

static void spi_disable(void)
{
	ssi_wr(SSI_SSIENR, 0);
	fence_rw();
}

static void spi_enable(void)
{
	ssi_wr(SSI_SSIENR, 1);
	fence_rw();
}

int spi_init(unsigned baud_div, int mode)
{
	uint32_t ctrl;

	/* SCKDV is 16-bit even. Clamp so a bad caller cannot program 0. */
	if (baud_div < 2u)
		baud_div = 2u;
	if (baud_div > 65534u)
		baud_div = 65534u;
	if (baud_div & 1u)
		baud_div++;
	g_baud_div = baud_div;
	g_mode = mode & 3;

	spi_disable();
	ssi_wr(SSI_IMR, 0);

	ctrl = CTRLR0_DFS_8 | CTRLR0_FRF_SPI | CTRLR0_TMOD_TR;
	if (g_mode & 1)
		ctrl |= CTRLR0_SCPH;
	if (g_mode & 2)
		ctrl |= CTRLR0_SCPOL;
	if (g_loopback)
		ctrl |= CTRLR0_SRL;
	ssi_wr(SSI_CTRLR0, ctrl);
	ssi_wr(SSI_BAUDR, g_baud_div);
	ssi_wr(SSI_TXFTLR, 0);
	ssi_wr(SSI_RXFTLR, 0);
	ssi_wr(SSI_SER, 1); /* keep slave0 selected; CS is GPIO */
	fence_rw();
	spi_enable();
	return 0;
}

void spi_set_loopback(int on)
{
	g_loopback = on ? 1 : 0;
	spi_init(g_baud_div, g_mode);
}

int spi_xfer(const uint8_t *tx, uint8_t *rx, unsigned len)
{
	unsigned i;
	uint32_t spins;

	for (i = 0; i < len; i++) {
		spins = 0;
		while ((ssi_rd(SSI_SR) & SR_TFNF) == 0) {
			if (++spins > 5000000u)
				return -1;
		}
		ssi_wr(SSI_DR0, tx ? tx[i] : 0xffu);

		spins = 0;
		while ((ssi_rd(SSI_SR) & SR_RFNE) == 0) {
			if (++spins > 5000000u)
				return -2;
		}
		{
			uint32_t d = ssi_rd(SSI_DR0) & 0xffu;

			if (rx)
				rx[i] = (uint8_t)d;
		}
	}
	spins = 0;
	while (ssi_rd(SSI_SR) & SR_BUSY) {
		if (++spins > 5000000u)
			return -3;
	}
	return 0;
}

int spi_flash_read_id(uint8_t id[3])
{
	uint8_t tx[4] = { 0x9f, 0x00, 0x00, 0x00 };
	uint8_t rx[4];
	int r;

	spi_cs(1);
	r = spi_xfer(tx, rx, 4);
	spi_cs(0);
	if (r)
		return r;
	id[0] = rx[1];
	id[1] = rx[2];
	id[2] = rx[3];
	return 0;
}

static void addr24(uint8_t *b, uint32_t addr)
{
	b[0] = (uint8_t)((addr >> 16) & 0xffu);
	b[1] = (uint8_t)((addr >> 8) & 0xffu);
	b[2] = (uint8_t)(addr & 0xffu);
}

static int flash_rdsr(uint8_t *sr)
{
	uint8_t tx[2] = { 0x05, 0xff };
	uint8_t rx[2];
	int r;

	spi_cs(1);
	r = spi_xfer(tx, rx, 2);
	spi_cs(0);
	if (r)
		return r;
	*sr = rx[1];
	return 0;
}

static int flash_wait_ready(void)
{
	uint32_t n;

	for (n = 0; n < 2000000u; n++) {
		uint8_t sr;
		int r = flash_rdsr(&sr);

		if (r)
			return r;
		if ((sr & 1u) == 0)
			return 0;
	}
	return -4; /* WIP timeout */
}

static int flash_wren(void)
{
	uint8_t cmd = 0x06;
	int r;

	spi_cs(1);
	r = spi_xfer(&cmd, 0, 1);
	spi_cs(0);
	return r;
}

int spi_flash_read(uint32_t addr, uint8_t *buf, unsigned len)
{
	uint8_t cmd[4];
	int r;

	cmd[0] = 0x03;
	addr24(cmd + 1, addr);
	spi_cs(1);
	r = spi_xfer(cmd, 0, 4);
	if (r == 0)
		r = spi_xfer(0, buf, len);
	spi_cs(0);
	return r;
}

int spi_flash_rw_test(uint8_t got[SPI_FLASH_TEST_LEN])
{
	uint8_t pat[SPI_FLASH_TEST_LEN];
	uint8_t cmd[4];
	unsigned i;
	int r;

	for (i = 0; i < SPI_FLASH_TEST_LEN; i++)
		pat[i] = (uint8_t)(0xa0u + i);

	/* 4 KiB sector erase at the test address. */
	r = flash_wren();
	if (r)
		return r;
	cmd[0] = 0x20;
	addr24(cmd + 1, SPI_FLASH_TEST_ADDR);
	spi_cs(1);
	r = spi_xfer(cmd, 0, 4);
	spi_cs(0);
	if (r)
		return r;
	r = flash_wait_ready();
	if (r)
		return r;

	/* Page program 16 bytes. */
	r = flash_wren();
	if (r)
		return r;
	cmd[0] = 0x02;
	addr24(cmd + 1, SPI_FLASH_TEST_ADDR);
	spi_cs(1);
	r = spi_xfer(cmd, 0, 4);
	if (r == 0)
		r = spi_xfer(pat, 0, SPI_FLASH_TEST_LEN);
	spi_cs(0);
	if (r)
		return r;
	r = flash_wait_ready();
	if (r)
		return r;

	r = spi_flash_read(SPI_FLASH_TEST_ADDR, got, SPI_FLASH_TEST_LEN);
	if (r)
		return r;
	for (i = 0; i < SPI_FLASH_TEST_LEN; i++) {
		if (got[i] != pat[i])
			return -5;
	}
	return 0;
}
