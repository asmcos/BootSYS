/*
 * 23-sd — SD1 TF card on 嘉立创庐山派 K230-CanMV.
 * Command flow follows riscvboot drivers/sd/sd.c. Sector 0 is read-only.
 * w writes the current LBA and leaves the bytes there.
 */
#include "k230.h"
#include "pinmux.h"
#include "sd.h"

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

static int card_ok;
static uint32_t cur_lba;
static uint8_t pat0 = 0xa0;
static uint8_t sec[SD_BLOCK_SIZE] __attribute__((aligned(4)));

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  23-sd                           │\r\n");
	uart_puts("│  SD1 TF card, sector read / write            │\r\n");
	uart_puts("├──────────────────────────────────────────────┤\r\n");
	uart_puts("│  build   " __DATE__ " " __TIME__ "                │\r\n");
	uart_puts("│  git     " BOOTSYS_GIT "                             │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
	uart_puts("pins: CMD=io54 CLK=io55 D0-D3=io56-59  SEL=2\r\n");
	uart_puts("slot: onboard TF (SD1 @ 0x91581000)\r\n");
}

static void put_hex8(uint8_t v)
{
	static const char h[] = "0123456789abcdef";

	uart_putc(h[(v >> 4) & 0xf]);
	uart_putc(h[v & 0xf]);
}

static void show_menu(void)
{
	uart_puts("\r\n======== 23-sd ========\r\n");
	uart_puts("  1  init card (pinmux + CMD0..ACMD6)\r\n");
	uart_puts("  2  read current LBA\r\n");
	uart_puts("  3  LBA 256\r\n");
	uart_puts("  +  next LBA     -  prev LBA     0  LBA 0\r\n");
	uart_puts("  ]  pat +1      [  pat -1      (byte i = pat+i, wraps at 256)\r\n");
	uart_puts("  w  write pattern at current LBA, left on the card\r\n");
	uart_puts("  4  dump pinmux io54-59\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("=======================\r\n");
}

static void prompt(void)
{
	uart_puts("[sd");
	if (card_ok)
		uart_puts(" ready");
	uart_puts(" lba=");
	uart_putdec32(cur_lba);
	uart_puts(" pat=");
	put_hex8(pat0);
	uart_puts("] > ");
}

static void show_err(const char *what, int r)
{
	uart_puts(what);
	uart_puts(" err ");
	if (r < 0) {
		uart_putc('-');
		uart_putdec32((uint32_t)(-r));
	} else {
		uart_putdec32((uint32_t)r);
	}
	uart_puts(" irq=");
	uart_puthex32(sd_last_irq());
	uart_puts(" state=");
	uart_puthex32(sd_present());
	uart_puts("\r\n");
}

static void dump16(const uint8_t *b)
{
	unsigned i;

	for (i = 0; i < 16u; i++) {
		put_hex8(b[i]);
		uart_putc(' ');
	}
	uart_puts("\r\n");
}

static void do_init(void)
{
	int r;
	const uint32_t *cid;
	unsigned i;

	uart_puts("init SD1...\r\n");
	r = sd_card_init();
	uart_puts("present=");
	uart_puthex32(sd_present());
	uart_puts("\r\n");
	if (r) {
		card_ok = 0;
		show_err("init", r);
		uart_puts("check TF card is inserted\r\n");
		return;
	}
	card_ok = 1;
	uart_puts("OCR=");
	uart_puthex32(sd_ocr());
	uart_puts(sd_sector_mode() ? "  sector\r\n" : "  byte\r\n");
	uart_puts("RCA=");
	uart_puthex32(sd_rca());
	uart_puts("\r\nCID=");
	cid = sd_cid();
	for (i = 0; i < 4u; i++) {
		uart_putc(' ');
		uart_puthex32(cid[i]);
	}
	uart_puts("\r\nPASS init\r\n");
}

static void do_read(void)
{
	int r;

	if (!card_ok) {
		uart_puts("init first (1)\r\n");
		return;
	}
	uart_puts("READ lba=");
	uart_putdec32(cur_lba);
	uart_puts("\r\n");
	r = sd_read_block(cur_lba, sec);
	if (r) {
		show_err("read", r);
		return;
	}
	dump16(sec);
	if (cur_lba == 0) {
		uart_puts("sig ");
		put_hex8(sec[510]);
		uart_putc(' ');
		put_hex8(sec[511]);
		uart_puts("\r\n");
		if (sec[510] == 0x55 && sec[511] == 0xaa)
			uart_puts("PASS boot signature\r\n");
		else
			uart_puts("no 55 AA (empty or not MBR/GPT)\r\n");
	} else {
		uart_puts("PASS\r\n");
	}
}

static void do_write(void)
{
	unsigned i;
	int r;

	if (!card_ok) {
		uart_puts("init first (1)\r\n");
		return;
	}
	if (cur_lba == 0) {
		uart_puts("LBA 0 is read-only. use + or - , 3 jumps to 256\r\n");
		return;
	}
	for (i = 0; i < SD_BLOCK_SIZE; i++)
		sec[i] = (uint8_t)(pat0 + i);
	uart_puts("write LBA ");
	uart_putdec32(cur_lba);
	uart_puts(" first=");
	put_hex8(pat0);
	uart_puts("\r\n");
	dump16(sec);
	r = sd_write_block(cur_lba, sec);
	if (r == 0)
		uart_puts("PASS wrote\r\n");
	else
		show_err("write", r);
}

static void do_dump(void)
{
	pinmux_dump(SD_PIN_CMD);
	pinmux_dump(SD_PIN_CLK);
	pinmux_dump(SD_PIN_D0);
	pinmux_dump(SD_PIN_D1);
	pinmux_dump(SD_PIN_D2);
	pinmux_dump(SD_PIN_D3);
}

int main(void)
{
	card_ok = 0;
	cur_lba = 0;

	sysctl_early_init();
	uart_init();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	show_menu();
	prompt();

	for (;;) {
		int c = uart_try_getc();

		if (c < 0)
			continue;
		uart_putc((char)c);
		uart_puts("\r\n");
		switch (c) {
		case '1':
			do_init();
			break;
		case '2':
		case 'r':
		case 'R':
			do_read();
			break;
		case '+':
		case '=':
			cur_lba++;
			do_read();
			break;
		case '-':
		case '_':
			if (cur_lba)
				cur_lba--;
			do_read();
			break;
		case '0':
			cur_lba = 0;
			do_read();
			break;
		case '3':
			cur_lba = SD_TEST_LBA;
			do_read();
			break;
		case ']':
			pat0++;
			break;
		case '[':
			pat0--;
			break;
		case 'w':
		case 'W':
			do_write();
			break;
		case '4':
			do_dump();
			break;
		case 'h':
		case 'H':
		case '?':
			show_menu();
			break;
		default:
			break;
		}
		prompt();
	}
}
