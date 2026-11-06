/*
 * SD1 PIO driver. Command sequence follows riscvboot/drivers/sd/sd.c
 * (CMD0/8/55/41/2/3/7, ACMD6 width, CMD17/24). Host reset, voltage,
 * clock and the 74 clocks come from the same DWC MSHC init used there.
 */
#include "k230.h"
#include "pinmux.h"
#include "sd.h"

#define SDHC_BLK            0x004u
#define SDHC_ARG            0x008u
#define SDHC_CMD            0x00Cu
#define SDHC_RESP0          0x010u
#define SDHC_RESP1          0x014u
#define SDHC_RESP2          0x018u
#define SDHC_RESP3          0x01Cu
#define SDHC_FIFO           0x020u
#define SDHC_STATE          0x024u
#define SDHC_HOST           0x028u
#define SDHC_CLK            0x02Cu
#define SDHC_IRQ            0x030u
#define SDHC_IRQ_EN         0x034u
#define SDHC_INT_EN         0x038u

#define ST_CMD_INHIBIT      (1u << 0)
#define ST_DAT_INHIBIT      (1u << 1)
#define ST_BUF_WR_EN        (1u << 10)
#define ST_CARD_INSERTED    (1u << 16)
#define ST_DAT0             (1u << 20)

#define HOST_WIDTH_MASK     0xFFFFFFDDu
#define HOST_WIDTH_1        0x00u
#define HOST_WIDTH_4        0x02u
#define HOST_PWR_ON         (1u << 8)
#define HOST_V33            (7u << 9)

#define CLK_INT_EN          (1u << 0)
#define CLK_INT_STABLE      (1u << 1)
#define CLK_CARD_EN         (1u << 2)
#define CLK_FREQ_MASK       (~(0x3FFu << 6))
#define CLK_TO_MASK         (~(0xFu << 16))
#define CLK_TO_DEFAULT      (0xEu << 16)
#define CLK_RST_ALL         (1u << 24)

#define IRQ_CMD_DONE        (1u << 0)
#define IRQ_XFER_DONE       (1u << 1)
#define IRQ_TX_READY        (1u << 4)
#define IRQ_RX_READY        (1u << 5)
#define IRQ_ERR             (1u << 15)
#define IRQ_DATA_CRC        (1u << 21)
#define IRQ_DATA_ENDBIT     (1u << 22)

#define CMD_DATA            (1u << 21)
#define CMD_READ            (1u << 4)
#define CMD_BLKCNT          (1u << 1)
#define CMD_CRC             (1u << 19)
#define CMD_INDEX           (1u << 20)
#define RESP_NONE           (0u << 16)
#define RESP_136            (1u << 16)
#define RESP_48             (2u << 16)
#define RESP_MASK           (3u << 16)

/* DWC MSHC vendor area (Linux sdhci-of-kendryte.c). SD1 has no PHY. */
#define MSHC_CTRL           0x508u
#define EMMC_CTRL           0x52Cu

#define OCR_SECTOR          (1u << 30)
#define OCR_BUSY            (1u << 31)

/*
 * sd_cclk is pll0/4 divided by 2 in sysctl (~200 MHz). CAP[15:8] reads
 * 0x64, but Linux marks that field broken, so it is not the divider base.
 * Using 100 MHz here made the run clock ~25 MHz and CMD24 failed data CRC.
 */
#define SD_CLK_IN_HZ        200000000u
#define SD_CLK_ID_HZ        400000u
#define SD_CLK_RUN_HZ       12500000u

enum {
	SD_OK = 0,
	SD_FAIL = -1,
	SD_TIMEOUT = -2,
	SD_NOCARD = -3,
};

static uint32_t g_rca;
static uint32_t g_ocr;
static int g_sector;
static uint32_t g_cid[4];
static uint32_t g_last_irq;

static void udelay(unsigned us)
{
	volatile unsigned n = us * 200u;

	while (n--)
		;
}

static uint32_t sd_rd(uint32_t off)
{
	return readl(SD1_BASE + off);
}

static void sd_wr(uint32_t off, uint32_t val)
{
	writel(val, SD1_BASE + off);
	fence_rw();
}

static void sd_wr16(uint32_t off, uint16_t val)
{
	*(volatile uint16_t *)(SD1_BASE + off) = val;
	fence_rw();
}

static void sd_pad(unsigned pin, int is_clk)
{
	pinmux_set_sel(pin, SD_PIN_SEL);
	pinmux_set_sl(pin, 0);
	pinmux_set_st(pin, 1);
	pinmux_set_ds(pin, 7);
	pinmux_set_oe(pin, 1);
	if (is_clk) {
		pinmux_set_ie(pin, 0);
		pinmux_set_pull(pin, PIN_PULL_NONE);
	} else {
		pinmux_set_ie(pin, 1);
		pinmux_set_pull(pin, PIN_PULL_UP);
	}
	/* IO50–61: MSC=0 is 3.3 V (K230_MSC_3V3). 1.8 V will not talk to a TF card. */
	{
		uint32_t v = pinmux_read(pin);

		v &= ~PIN_MSC;
		pinmux_write(pin, v);
	}
}

void sd_pins_setup(void)
{
	sd_pad(SD_PIN_CMD, 0);
	sd_pad(SD_PIN_CLK, 1);
	sd_pad(SD_PIN_D0, 0);
	sd_pad(SD_PIN_D1, 0);
	sd_pad(SD_PIN_D2, 0);
	sd_pad(SD_PIN_D3, 0);
}

/* Drop a stale read buffer. The first CMD24 after a read otherwise
 * shifts an empty FIFO and the card reports data CRC + a bad end bit. */
static void reset_cmd_dat(void)
{
	uint32_t clk, k;

	clk = sd_rd(SDHC_CLK);
	clk |= (1u << 25) | (1u << 26);
	sd_wr(SDHC_CLK, clk);
	for (k = 0; k < 100000u; k++) {
		if ((sd_rd(SDHC_CLK) & ((1u << 25) | (1u << 26))) == 0)
			break;
		udelay(1);
	}
}

static int wait_irq(uint32_t mask)
{
	uint32_t n;

	for (n = 0; n < 2000000u; n++) {
		uint32_t st = sd_rd(SDHC_IRQ);

		if (st & IRQ_ERR) {
			g_last_irq = st;
			sd_wr(SDHC_IRQ, 0xffffffffu);
			reset_cmd_dat();
			return SD_FAIL;
		}
		if ((st & mask) == mask) {
			sd_wr(SDHC_IRQ, mask);
			return SD_OK;
		}
		if ((n & 63u) == 63u)
			udelay(1);
	}
	return SD_TIMEOUT;
}

static int wait_inhibit(int data_too)
{
	uint32_t n;
	uint32_t bits = ST_CMD_INHIBIT;

	if (data_too)
		bits |= ST_DAT_INHIBIT;
	for (n = 0; n < 2000000u; n++) {
		if ((sd_rd(SDHC_STATE) & bits) == 0)
			return SD_OK;
		udelay(1);
	}
	return SD_TIMEOUT;
}

static uint32_t make_cmd(unsigned idx, uint32_t resp, int check)
{
	uint32_t cmd = ((idx & 0x3fu) << 24) | resp;

	if (check)
		cmd |= CMD_CRC | CMD_INDEX;
	return cmd;
}

static int send_cmd(uint32_t cmd, uint32_t arg, uint32_t resp[4])
{
	int r;

	r = wait_inhibit(0);
	if (r)
		return r;
	sd_wr(SDHC_IRQ, 0xffffffffu);
	sd_wr(SDHC_ARG, arg);
	sd_wr(SDHC_CMD, cmd);
	r = wait_irq(IRQ_CMD_DONE);
	if (r)
		return r;
	if ((cmd & RESP_MASK) == (3u << 16)) {
		r = wait_irq(IRQ_XFER_DONE);
		if (r)
			return r;
	}
	if (resp) {
		resp[0] = sd_rd(SDHC_RESP0);
		resp[1] = sd_rd(SDHC_RESP1);
		resp[2] = sd_rd(SDHC_RESP2);
		resp[3] = sd_rd(SDHC_RESP3);
	}
	return SD_OK;
}

static int send_app(unsigned idx, uint32_t arg, uint32_t resp[4], int check)
{
	int r;
	uint32_t cmd;

	cmd = make_cmd(55, RESP_48, 1);
	r = send_cmd(cmd, g_rca, resp);
	if (r)
		return r;
	cmd = make_cmd(idx, RESP_48, check);
	return send_cmd(cmd, arg, resp);
}

static uint32_t sdclk_base_hz(void)
{
	return SD_CLK_IN_HZ;
}

static int set_clock(uint32_t hz)
{
	uint32_t div, clk, n, base;

	base = sdclk_base_hz();
	if (hz == 0 || hz > base / 2u)
		return SD_FAIL;
	div = base / hz / 2u;
	if (div == 0)
		div = 1;
	if (div > 0x3ffu)
		div = 0x3ffu;

	clk = sd_rd(SDHC_CLK);
	clk &= ~CLK_CARD_EN;
	sd_wr(SDHC_CLK, clk);

	clk &= CLK_FREQ_MASK;
	clk |= ((div & 0xffu) << 8);
	clk |= ((div & 0x300u) >> 2);
	clk |= CLK_INT_EN;
	sd_wr(SDHC_CLK, clk);

	for (n = 0; n < 100000u; n++) {
		if (sd_rd(SDHC_CLK) & CLK_INT_STABLE)
			break;
		udelay(1);
	}
	if (n >= 100000u)
		return SD_TIMEOUT;

	clk = sd_rd(SDHC_CLK);
	clk |= CLK_CARD_EN;
	sd_wr(SDHC_CLK, clk);
	return SD_OK;
}

static void set_width(uint32_t width)
{
	uint32_t v = sd_rd(SDHC_HOST);

	v &= HOST_WIDTH_MASK;
	v |= width;
	sd_wr(SDHC_HOST, v);
}

/* Connect SD1 to the pads. Linux does this before the host is used. */
static void sd1_pad_enable(void)
{
	uint32_t v = readl(HS_SD1_CTRL);

	v |= (1u << 0) | (1u << 2);
	writel(v, HS_SD1_CTRL);
	fence_rw();
}

static void dwc_after_reset(void)
{
	uint32_t v;

	/* Not an eMMC. MSHC_CTRL 0 = launch CMD/DAT on posedge of cclk_tx. */
	v = sd_rd(EMMC_CTRL);
	v &= ~1u;
	sd_wr(EMMC_CTRL, v);
	v = sd_rd(MSHC_CTRL);
	v &= ~0xffffu;
	sd_wr(MSHC_CTRL, v);
}

static int host_init(void)
{
	uint32_t v, n;
	int r;

	sd1_pad_enable();

	v = sd_rd(SDHC_CLK);
	v |= CLK_RST_ALL;
	sd_wr(SDHC_CLK, v);
	for (n = 0; n < 100000u; n++) {
		if ((sd_rd(SDHC_CLK) & CLK_RST_ALL) == 0)
			break;
		udelay(1);
	}
	dwc_after_reset();

	v = sd_rd(SDHC_CLK);
	v &= CLK_TO_MASK;
	v |= CLK_TO_DEFAULT;
	sd_wr(SDHC_CLK, v);

	sd_wr(SDHC_INT_EN, 0);
	/* Command/data/error status only. Card-int (bit 8) stays off. */
	sd_wr(SDHC_IRQ_EN, 0xffff0000u | IRQ_CMD_DONE | IRQ_XFER_DONE |
	      IRQ_TX_READY | IRQ_RX_READY | IRQ_ERR);
	sd_wr(SDHC_IRQ, 0xffffffffu);

	v = sd_rd(SDHC_HOST);
	v &= ~(7u << 9);
	v |= HOST_V33 | HOST_PWR_ON;
	sd_wr(SDHC_HOST, v);
	udelay(2000);

	r = set_clock(SD_CLK_ID_HZ);
	if (r)
		return r;
	set_width(HOST_WIDTH_1);
	/* Card clock is running: 2 ms is well past the required 74 clocks. */
	udelay(2000);

	uart_puts("hs=");
	uart_puthex32(readl(HS_SD1_CTRL));
	uart_puts(" cap=");
	uart_puthex32(sd_rd(0x40u));
	uart_puts(" clk=");
	uart_puthex32(sd_rd(SDHC_CLK));
	uart_puts("\r\n");
	return SD_OK;
}

static uint32_t lba_arg(uint32_t lba)
{
	if (g_sector)
		return lba;
	return lba * SD_BLOCK_SIZE;
}

int sd_card_init(void)
{
	uint32_t resp[4];
	uint32_t cmd;
	int r, timeout;

	g_rca = 0;
	g_ocr = 0;
	g_sector = 0;

	sd_pins_setup();
	r = host_init();
	if (r)
		return r;

	cmd = make_cmd(0, RESP_NONE, 0);
	r = send_cmd(cmd, 0, resp);
	if (r) {
		uart_puts("CMD0 fail\r\n");
		return r;
	}

	cmd = make_cmd(8, RESP_48, 1);
	r = send_cmd(cmd, 0x1aau, resp);
	if (r) {
		uart_puts("CMD8 fail\r\n");
		return r;
	}
	if ((resp[0] & 0xffu) != 0xaau) {
		uart_puts("CMD8 echo ");
		uart_puthex32(resp[0]);
		uart_puts("\r\n");
		return SD_FAIL;
	}

	timeout = 2000;
	while (timeout--) {
		r = send_app(41, 0x40ff8000u, resp, 0);
		if (r) {
			uart_puts("ACMD41 fail\r\n");
			return r;
		}
		if (resp[0] & OCR_BUSY) {
			g_ocr = resp[0];
			g_sector = (resp[0] & OCR_SECTOR) ? 1 : 0;
			break;
		}
		udelay(1000);
	}
	if (timeout < 0)
		return SD_TIMEOUT;

	cmd = make_cmd(2, RESP_136, 0);
	r = send_cmd(cmd, 0, resp);
	if (r) {
		uart_puts("CMD2 fail\r\n");
		return r;
	}
	g_cid[0] = resp[0];
	g_cid[1] = resp[1];
	g_cid[2] = resp[2];
	g_cid[3] = resp[3];

	cmd = make_cmd(3, RESP_48, 1);
	r = send_cmd(cmd, 0, resp);
	if (r) {
		uart_puts("CMD3 fail\r\n");
		return r;
	}
	g_rca = resp[0] & 0xffff0000u;

	cmd = make_cmd(7, RESP_48, 1);
	r = send_cmd(cmd, g_rca, resp);
	if (r) {
		uart_puts("CMD7 fail\r\n");
		return r;
	}

	r = set_clock(SD_CLK_RUN_HZ);
	if (r)
		return r;
	uart_puts("run=");
	uart_puthex32(sd_rd(SDHC_CLK));
	uart_puts("\r\n");

	r = send_app(6, 2, resp, 1); /* ACMD6: 4-bit */
	if (r) {
		uart_puts("ACMD6 fail\r\n");
		return r;
	}
	set_width(HOST_WIDTH_4);
	return SD_OK;
}

static int pio_read(uint32_t lba, uint8_t *buf)
{
	uint32_t cmd, resp[4];
	uint32_t *p = (uint32_t *)buf;
	unsigned i;
	int r;

	r = wait_inhibit(1);
	if (r)
		return r;
	sd_wr(SDHC_BLK, (1u << 16) | SD_BLOCK_SIZE);
	cmd = make_cmd(17, RESP_48, 1);
	cmd |= CMD_DATA | CMD_READ | CMD_BLKCNT;
	r = send_cmd(cmd, lba_arg(lba), resp);
	if (r)
		return r;
	r = wait_irq(IRQ_RX_READY);
	if (r)
		return r;
	for (i = 0; i < SD_BLOCK_SIZE / 4u; i++)
		p[i] = sd_rd(SDHC_FIFO);
	return wait_irq(IRQ_XFER_DONE);
}

static unsigned fifo_prefill(const uint32_t *p, unsigned n, unsigned total)
{
	while (n < total && (sd_rd(SDHC_STATE) & ST_BUF_WR_EN)) {
		sd_wr(SDHC_FIFO, p[n]);
		n++;
	}
	return n;
}

static int pio_write_once(uint32_t lba, const uint8_t *buf, unsigned *filled)
{
	uint32_t cmd;
	const uint32_t *p = (const uint32_t *)buf;
	unsigned n, total;
	int r;

	*filled = 0;
	r = wait_inhibit(1);
	if (r)
		return r;
	reset_cmd_dat();
	/* Boundary 7 = 512 KiB, so a 512-byte block is not split. Count = 1. */
	sd_wr(SDHC_BLK, (7u << 12) | (1u << 16) | SD_BLOCK_SIZE);
	sd_wr(SDHC_IRQ, 0xffffffffu);
	/* Transfer mode first (write). Command at 0x0E is what starts CMD24. */
	sd_wr16(SDHC_CMD, (uint16_t)CMD_BLKCNT);

	/*
	 * With the direction set to write, Buffer Write Enable is 1.
	 * Load the FIFO before the command, or the host shifts an empty
	 * buffer and the card answers with data CRC + bad end bit.
	 */
	total = SD_BLOCK_SIZE / 4u;
	n = fifo_prefill(p, 0, total);
	*filled = n;

	cmd = make_cmd(24, RESP_48, 1);
	cmd |= CMD_DATA;
	sd_wr(SDHC_ARG, lba_arg(lba));
	sd_wr16(0x0Eu, (uint16_t)(cmd >> 16));
	r = wait_irq(IRQ_CMD_DONE);
	if (r)
		return r;

	while (n < total) {
		unsigned got = n;

		r = wait_irq(IRQ_TX_READY);
		if (r) {
			*filled = n;
			return r;
		}
		n = fifo_prefill(p, n, total);
		*filled = n;
		if (n == got)
			return SD_TIMEOUT;
	}
	r = wait_irq(IRQ_XFER_DONE);
	if (r)
		return r;
	/* Card busy on DAT0 after the CRC status. */
	for (n = 0; n < 2000000u; n++) {
		if (sd_rd(SDHC_STATE) & ST_DAT0)
			return SD_OK;
		if ((n & 63u) == 63u)
			udelay(1);
	}
	return SD_TIMEOUT;
}

static int pio_write(uint32_t lba, const uint8_t *buf)
{
	unsigned n = 0;
	int try, r;

	/*
	 * After reset the data buffer drops a prefill on the first CMD24
	 * (data CRC + end bit, fifo already 128). The error path resets
	 * DAT, and the same write then sticks. Retry once before reporting.
	 */
	for (try = 0; try < 2; try++) {
		r = pio_write_once(lba, buf, &n);
		if (r == SD_OK)
			return SD_OK;
		if (r != SD_FAIL)
			break;
		if ((g_last_irq & (IRQ_DATA_CRC | IRQ_DATA_ENDBIT)) == 0)
			break;
	}
	uart_puts("write fifo ");
	uart_putdec32(n);
	uart_puts("\r\n");
	return r;
}

int sd_read_block(uint32_t lba, uint8_t buf[SD_BLOCK_SIZE])
{
	return pio_read(lba, buf);
}

int sd_write_block(uint32_t lba, const uint8_t buf[SD_BLOCK_SIZE])
{
	return pio_write(lba, buf);
}

uint32_t sd_rca(void)
{
	return g_rca;
}

uint32_t sd_ocr(void)
{
	return g_ocr;
}

int sd_sector_mode(void)
{
	return g_sector;
}

const uint32_t *sd_cid(void)
{
	return g_cid;
}

uint32_t sd_present(void)
{
	return sd_rd(SDHC_STATE);
}

uint32_t sd_last_irq(void)
{
	return g_last_irq;
}
