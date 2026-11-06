/*
 * 22-spi — QSPI0 (DW SSI) + pinmux; Flash JEDEC ID / loopback
 */
#include "k230.h"
#include "pinmux.h"
#include "spi.h"

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

enum {
	MENU_TOP = 0,
	MENU_XFER,
	MENU_CFG,
};

static int menu_level = MENU_TOP;
static unsigned baud_div = 100; /* SCK ≈ SSI_clk / baud_div */
static int spi_mode;
static int loopback;

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  22-spi                          │\r\n");
	uart_puts("│  QSPI0 pinmux, Flash ID / loopback           │\r\n");
	uart_puts("├──────────────────────────────────────────────┤\r\n");
	uart_puts("│  build   " __DATE__ " " __TIME__ "                │\r\n");
	uart_puts("│  git     " BOOTSYS_GIT "                             │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
	uart_puts("pins: CS=io14(GPIO) CLK=io15 MOSI=io16 MISO=io17\r\n");
	uart_puts("      (QSPI0 via IO_SEL=3; needs external Flash or loopback)\r\n");
}

static void show_cfg(void)
{
	uart_puts("cfg: mode=");
	uart_putdec32((uint32_t)spi_mode);
	uart_puts(" baud_div=");
	uart_putdec32(baud_div);
	uart_puts(" loopback=");
	uart_putc(loopback ? '1' : '0');
	uart_puts("\r\n");
}

static void menu_top(void)
{
	uart_puts("\r\n======== top ========\r\n");
	show_cfg();
	uart_puts("  1  setup pins + spi_init\r\n");
	uart_puts("  2  transfer (Flash ID / loopback)\r\n");
	uart_puts("  3  config (mode / baud / loopback)\r\n");
	uart_puts("  4  dump pinmux io14-17\r\n");
	uart_puts("  5  pinmux prove (GPIO vs QSPI on CLK)\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("=====================\r\n");
}

static void menu_xfer(void)
{
	uart_puts("\r\n======== xfer ========\r\n");
	uart_puts("  i  read Flash JEDEC ID (0x9F)\r\n");
	uart_puts("  r  read 16 bytes at 0x10000\r\n");
	uart_puts("  w  erase+program+read 16 bytes (destroys that 4K sector)\r\n");
	uart_puts("  l  loopback smoke (needs config t, or MOSI-MISO short)\r\n");
	uart_puts("  b  back\r\n");
	uart_puts("======================\r\n");
}

static void menu_cfg(void)
{
	uart_puts("\r\n======== config ========\r\n");
	show_cfg();
	uart_puts("  0..3  SPI mode (CPOL/CPHA)\r\n");
	uart_puts("  + / - baud_div nudge (+/- 2)\r\n");
	uart_puts("  t     toggle loopback (SRL)\r\n");
	uart_puts("  a     apply spi_init\r\n");
	uart_puts("  b     back\r\n");
	uart_puts("========================\r\n");
}

static void prompt(void)
{
	uart_puts("\r\n[");
	switch (menu_level) {
	case MENU_XFER:
		uart_puts("xfer");
		break;
	case MENU_CFG:
		uart_puts("cfg");
		break;
	default:
		uart_puts("top");
		break;
	}
	uart_puts("] > ");
}

static void show_menu(void)
{
	switch (menu_level) {
	case MENU_XFER:
		menu_xfer();
		break;
	case MENU_CFG:
		menu_cfg();
		break;
	default:
		menu_top();
		break;
	}
	prompt();
}

static void goto_level(int level)
{
	menu_level = level;
	show_menu();
}

static void do_setup(void)
{
	spi_pins_setup();
	spi_set_loopback(loopback);
	spi_init(baud_div, spi_mode);
	uart_puts("pins+spi ready\r\n");
	show_cfg();
}

static void do_dump_pins(void)
{
	uart_puts("\r\n");
	pinmux_dump(SPI_PIN_CS);
	pinmux_dump(SPI_PIN_CLK);
	pinmux_dump(SPI_PIN_MOSI);
	pinmux_dump(SPI_PIN_MISO);
}

static void put_hex8(uint8_t v)
{
	static const char h[] = "0123456789abcdef";

	uart_putc(h[(v >> 4) & 0xf]);
	uart_putc(h[v & 0xf]);
}

static void do_flash_id(void)
{
	uint8_t id[3];
	int r;

	uart_puts("RDID 0x9F...\r\n");
	r = spi_flash_read_id(id);
	if (r) {
		uart_puts("xfer err ");
		uart_putdec32((uint32_t)(r < 0 ? (unsigned)(-r) : (unsigned)r));
		uart_puts("\r\n");
		return;
	}
	uart_puts("ID: ");
	put_hex8(id[0]);
	uart_putc(' ');
	put_hex8(id[1]);
	uart_putc(' ');
	put_hex8(id[2]);
	uart_puts("\r\n");
	if (id[0] == 0x00 || id[0] == 0xff)
		uart_puts("hint: no Flash? try loopback or check wiring\r\n");
	else
		uart_puts("PASS (non-empty ID)\r\n");
}

static void do_loopback(void)
{
	uint8_t tx[2] = { 0xa5, 0x5a };
	uint8_t rx[2] = { 0, 0 };
	int r;

	if (!loopback) {
		uart_puts("enable loopback in config (t) first, or wire MOSI-MISO\r\n");
	}
	spi_cs(1);
	r = spi_xfer(tx, rx, 2);
	spi_cs(0);
	if (r) {
		uart_puts("err\r\n");
		return;
	}
	uart_puts("tx A5 5A  rx ");
	put_hex8(rx[0]);
	uart_putc(' ');
	put_hex8(rx[1]);
	uart_puts("\r\n");
	if (rx[0] == 0xa5 && rx[1] == 0x5a) {
		uart_puts("PASS\r\n");
		return;
	}
	if (!loopback && rx[0] == 0xff && rx[1] == 0xff) {
		uart_puts("expected: MISO idle high, no short and loopback off\r\n");
		return;
	}
	uart_puts("check wiring / loopback bit\r\n");
}

static void dump_bytes(const uint8_t *b, unsigned n)
{
	unsigned i;

	for (i = 0; i < n; i++) {
		if (i && (i % 8u) == 0)
			uart_puts("\r\n");
		put_hex8(b[i]);
		uart_putc(' ');
	}
	uart_puts("\r\n");
}

static void do_flash_read(void)
{
	uint8_t buf[SPI_FLASH_TEST_LEN];
	int r;

	uart_puts("READ 0x10000 x16\r\n");
	r = spi_flash_read(SPI_FLASH_TEST_ADDR, buf, SPI_FLASH_TEST_LEN);
	if (r) {
		uart_puts("read err\r\n");
		return;
	}
	dump_bytes(buf, SPI_FLASH_TEST_LEN);
}

static void do_flash_rw(void)
{
	uint8_t got[SPI_FLASH_TEST_LEN];
	int r;

	if (loopback) {
		uart_puts("turn loopback off (config t) before Flash write\r\n");
		return;
	}
	uart_puts("erase 4K @ 0x10000, program a0..af, read back\r\n");
	r = spi_flash_rw_test(got);
	if (r == 0 || r == -5)
		dump_bytes(got, SPI_FLASH_TEST_LEN);
	if (r == 0)
		uart_puts("PASS\r\n");
	else if (r == -5)
		uart_puts("FAIL mismatch\r\n");
	else if (r == -4)
		uart_puts("FAIL busy timeout\r\n");
	else
		uart_puts("FAIL xfer\r\n");
}

static void do_pinmux_prove(void)
{
	uart_puts("\r\nprove: CLK pin must be QSPI (sel=3), not GPIO\r\n");
	uart_puts("[1] mux CLK to GPIO, try RDID (expect fail/empty)\r\n");
	pinmux_as_gpio(SPI_PIN_CLK, 1);
	spi_init(baud_div, spi_mode);
	do_flash_id();

	uart_puts("[2] mux CLK back to QSPI0 (sel=3), RDID again\r\n");
	pinmux_set_sel(SPI_PIN_CLK, SPI_PIN_SEL_QSPI0);
	pinmux_set_ie(SPI_PIN_CLK, 1);
	pinmux_set_oe(SPI_PIN_CLK, 1);
	spi_init(baud_div, spi_mode);
	do_flash_id();
	uart_puts("(same idea as 21 LED prove — pad mux owns the wire)\r\n");
}

static int handle_top(int c)
{
	switch (c) {
	case '1':
		do_setup();
		prompt();
		return 1;
	case '2':
		goto_level(MENU_XFER);
		return 1;
	case '3':
		goto_level(MENU_CFG);
		return 1;
	case '4':
		do_dump_pins();
		prompt();
		return 1;
	case '5':
		do_setup();
		do_pinmux_prove();
		prompt();
		return 1;
	case 'h':
	case 'H':
	case '?':
		show_menu();
		return 1;
	default:
		return 0;
	}
}

static int handle_xfer(int c)
{
	switch (c) {
	case 'i':
	case 'I':
		do_flash_id();
		prompt();
		return 1;
	case 'r':
	case 'R':
		do_flash_read();
		prompt();
		return 1;
	case 'w':
	case 'W':
		do_flash_rw();
		prompt();
		return 1;
	case 'l':
	case 'L':
		do_loopback();
		prompt();
		return 1;
	case 'b':
	case 'B':
		goto_level(MENU_TOP);
		return 1;
	case 'h':
	case 'H':
	case '?':
		show_menu();
		return 1;
	default:
		return 0;
	}
}

static int handle_cfg(int c)
{
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
		spi_mode = c - '0';
		uart_puts("mode=");
		uart_putc((char)c);
		uart_puts("\r\n");
		prompt();
		return 1;
	case '+':
	case '=':
		baud_div += 2;
		if (baud_div > 65534u)
			baud_div = 65534u;
		show_cfg();
		prompt();
		return 1;
	case '-':
	case '_':
		if (baud_div > 2u)
			baud_div -= 2;
		show_cfg();
		prompt();
		return 1;
	case 't':
	case 'T':
		loopback = !loopback;
		show_cfg();
		prompt();
		return 1;
	case 'a':
	case 'A':
		spi_set_loopback(loopback);
		spi_init(baud_div, spi_mode);
		uart_puts("applied\r\n");
		prompt();
		return 1;
	case 'b':
	case 'B':
		goto_level(MENU_TOP);
		return 1;
	case 'h':
	case 'H':
	case '?':
		show_menu();
		return 1;
	default:
		return 0;
	}
}

int main(void)
{
	/* Defaults live in .data/.bss at 0x80220000. Re-state them here so a
	 * missed boot copy cannot leave mode/baud/loopback as SRAM garbage
	 * (that turned SRL on and made RDID look like 00 00 00). */
	baud_div = 100;
	spi_mode = 0;
	loopback = 0;
	menu_level = MENU_TOP;

	sysctl_early_init();
	uart_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	menu_level = MENU_TOP;
	show_menu();

	for (;;) {
		int c = uart_try_getc();
		int handled;

		if (c < 0)
			continue;
		switch (menu_level) {
		case MENU_XFER:
			handled = handle_xfer(c);
			break;
		case MENU_CFG:
			handled = handle_cfg(c);
			break;
		default:
			handled = handle_top(c);
			break;
		}
		(void)handled;
	}
}
