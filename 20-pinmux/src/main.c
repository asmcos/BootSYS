/*
 * 20-pinmux — progressive menu: pick pin → mux / pad / dump
 */
#include "k230.h"
#include "pinmux.h"

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

static unsigned cur_pin = PINMUX_DEMO_PIN;

enum {
	MENU_TOP = 0,
	MENU_PICK_PIN,
	MENU_MUX,
	MENU_PAD,
	MENU_PAD_DS,
};

static int menu_level = MENU_TOP;

static int pin_is_uart0(unsigned pin)
{
	return pin == PINMUX_UART0_TX || pin == PINMUX_UART0_RX;
}

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("|  BootSYS  -  20-pinmux                       |\r\n");
	uart_puts("|  IOMUX mux + pad (step-by-step menu)         |\r\n");
	uart_puts("|  build   " __DATE__ " " __TIME__ "              |\r\n");
	uart_puts("|  git     " BOOTSYS_GIT "                           |\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("\r\n");
}

static void show_pin_brief(void)
{
	uint32_t v = pinmux_read(cur_pin);

	uart_puts("current pin: io");
	uart_putdec32(cur_pin);
	uart_puts("  sel=");
	uart_putdec32((v & PIN_SEL_MASK) >> PIN_SEL_SHIFT);
	uart_puts(" IE=");
	uart_putc((v & PIN_IE) ? '1' : '0');
	uart_puts(" OE=");
	uart_putc((v & PIN_OE) ? '1' : '0');
	uart_puts(" PU=");
	uart_putc((v & PIN_PU) ? '1' : '0');
	uart_puts(" PD=");
	uart_putc((v & PIN_PD) ? '1' : '0');
	uart_puts(" DS=");
	uart_putdec32((v & PIN_DS_MASK) >> PIN_DS_SHIFT);
	uart_puts("\r\n");
}

static void menu_top(void)
{
	uart_puts("\r\n======== top ========\r\n");
	show_pin_brief();
	uart_puts("  1  pick pin\r\n");
	uart_puts("  2  dump (cur + UART0 38/39)\r\n");
	uart_puts("  3  set mux   (IO_SEL alt0..7)\r\n");
	uart_puts("  4  set pad   (IE/OE/pull/DS)\r\n");
	uart_puts("  5  as GPIO   (alt0 + IE + ds8)\r\n");
	uart_puts("  6  restore UART0 pads\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("=====================\r\n");
}

static void menu_pick_pin(void)
{
	uart_puts("\r\n======== pick pin ========\r\n");
	uart_puts("now io");
	uart_putdec32(cur_pin);
	uart_puts("\r\n");
	uart_puts("  +/-          nudge\r\n");
	uart_puts("  digits+Enter type io number (0..71)\r\n");
	uart_puts("  q/w/e/r      preset io20 / io0 / io32 / io54\r\n");
	uart_puts("  b            back to top\r\n");
	uart_puts("Avoid 38/39 (UART0), 2-6 (JTAG) unless intentional.\r\n");
	uart_puts("==========================\r\n");
}

static void menu_presets(void)
{
	uart_puts("\r\npresets: q=io20  w=io0  e=io32  r=io54\r\n");
}

static void menu_mux(void)
{
	uart_puts("\r\n======== set mux ========\r\n");
	show_pin_brief();
	uart_puts("  0..7  IO_SEL (0=GPIO/alt0)\r\n");
	uart_puts("  d     dump this pin\r\n");
	uart_puts("  b     back to top\r\n");
	uart_puts("=========================\r\n");
}

static void menu_pad(void)
{
	uart_puts("\r\n======== set pad ========\r\n");
	show_pin_brief();
	uart_puts("  i     toggle IE\r\n");
	uart_puts("  o     toggle OE\r\n");
	uart_puts("  u/w/n pull-up / pull-down / none\r\n");
	uart_puts("  s     set drive strength\r\n");
	uart_puts("  d     dump this pin\r\n");
	uart_puts("  b     back to top\r\n");
	uart_puts("=========================\r\n");
}

static void menu_pad_ds(void)
{
	unsigned ds = (pinmux_read(cur_pin) & PIN_DS_MASK) >> PIN_DS_SHIFT;

	uart_puts("\r\n======== drive strength ========\r\n");
	uart_puts("io");
	uart_putdec32(cur_pin);
	uart_puts(" DS=");
	uart_putdec32(ds);
	uart_puts("\r\n");
	uart_puts("  0..9 / a..f   set DS 0..15\r\n");
	uart_puts("  + / -         nudge\r\n");
	uart_puts("  b             back to pad menu\r\n");
	uart_puts("================================\r\n");
}

static void prompt(void)
{
	uart_puts("\r\n[");
	switch (menu_level) {
	case MENU_PICK_PIN:
		uart_puts("pin");
		break;
	case MENU_MUX:
		uart_puts("mux");
		break;
	case MENU_PAD:
		uart_puts("pad");
		break;
	case MENU_PAD_DS:
		uart_puts("ds");
		break;
	default:
		uart_puts("top");
		break;
	}
	uart_puts(" io");
	uart_putdec32(cur_pin);
	uart_puts("] > ");
}

static void show_menu(void)
{
	switch (menu_level) {
	case MENU_PICK_PIN:
		menu_pick_pin();
		break;
	case MENU_MUX:
		menu_mux();
		break;
	case MENU_PAD:
		menu_pad();
		break;
	case MENU_PAD_DS:
		menu_pad_ds();
		break;
	default:
		menu_top();
		break;
	}
	prompt();
}

static void warn_uart0(unsigned pin)
{
	if (!pin_is_uart0(pin))
		return;
	uart_puts("\r\n[warn] UART0 pad — menu 6 can restore\r\n");
}

static void do_dump(void)
{
	uart_puts("\r\n");
	pinmux_dump(cur_pin);
	pinmux_dump(PINMUX_UART0_TX);
	pinmux_dump(PINMUX_UART0_RX);
}

static void goto_level(int level)
{
	menu_level = level;
	show_menu();
}

/* Digits typed while in pick-pin; -1 means empty. */
static int pin_edit = -1;

static void pin_edit_reset(void)
{
	pin_edit = -1;
}

static void pin_edit_apply(void)
{
	if (pin_edit < 0)
		return;
	if (pin_edit > (int)(IOMUX_NPINS + PMU_IOMUX_NPINS - 1u)) {
		uart_puts(" out of range (0..71)\r\n");
		pin_edit_reset();
		return;
	}
	cur_pin = (unsigned)pin_edit;
	pin_edit_reset();
	uart_puts(" -> io");
	uart_putdec32(cur_pin);
	uart_puts("\r\n");
	show_pin_brief();
}

static int handle_top(int c)
{
	switch (c) {
	case '1':
		pin_edit_reset();
		goto_level(MENU_PICK_PIN);
		return 1;
	case '2':
		do_dump();
		prompt();
		return 1;
	case '3':
		goto_level(MENU_MUX);
		return 1;
	case '4':
		goto_level(MENU_PAD);
		return 1;
	case '5':
		warn_uart0(cur_pin);
		pinmux_as_gpio(cur_pin, 0);
		uart_puts("\r\nas GPIO done\r\n");
		show_pin_brief();
		prompt();
		return 1;
	case '6':
		pinmux_restore_uart0();
		uart_puts("\r\nUART0 pads restored\r\n");
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

static int handle_pick_pin(int c)
{
	if (c >= '0' && c <= '9') {
		int d = c - '0';

		if (pin_edit < 0)
			pin_edit = d;
		else if (pin_edit <= 7)
			pin_edit = pin_edit * 10 + d;
		else
			return 1; /* ignore extra digits */
		uart_putc((char)c);
		return 1;
	}
	if (c == 0x08 || c == 0x7f) {
		if (pin_edit >= 10) {
			pin_edit /= 10;
			uart_puts("\b \b");
		} else if (pin_edit >= 0) {
			pin_edit = -1;
			uart_puts("\b \b");
		}
		return 1;
	}
	if (c == '\r' || c == '\n') {
		uart_puts("\r\n");
		if (pin_edit >= 0)
			pin_edit_apply();
		prompt();
		return 1;
	}
	switch (c) {
	case '+':
	case '=':
		pin_edit_reset();
		cur_pin = (cur_pin + 1u) % (IOMUX_NPINS + PMU_IOMUX_NPINS);
		uart_puts("io");
		uart_putdec32(cur_pin);
		uart_puts("\r\n");
		prompt();
		return 1;
	case '-':
	case '_':
		pin_edit_reset();
		if (cur_pin == 0)
			cur_pin = IOMUX_NPINS + PMU_IOMUX_NPINS - 1u;
		else
			cur_pin--;
		uart_puts("io");
		uart_putdec32(cur_pin);
		uart_puts("\r\n");
		prompt();
		return 1;
	case 'p':
	case 'P':
		pin_edit_reset();
		menu_presets();
		prompt();
		return 1;
	case 'q':
	case 'Q':
		pin_edit_reset();
		cur_pin = PINMUX_DEMO_PIN;
		uart_puts("preset io20\r\n");
		show_pin_brief();
		prompt();
		return 1;
	case 'w':
		pin_edit_reset();
		cur_pin = 0;
		uart_puts("preset io0\r\n");
		show_pin_brief();
		prompt();
		return 1;
	case 'e':
		pin_edit_reset();
		cur_pin = 32;
		uart_puts("preset io32\r\n");
		show_pin_brief();
		prompt();
		return 1;
	case 'r':
		pin_edit_reset();
		cur_pin = 54;
		uart_puts("preset io54\r\n");
		show_pin_brief();
		prompt();
		return 1;
	case 'b':
	case 'B':
		pin_edit_reset();
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

static int handle_mux(int c)
{
	if (c >= '0' && c <= '7') {
		warn_uart0(cur_pin);
		pinmux_set_sel(cur_pin, (unsigned)(c - '0'));
		uart_puts("\r\nIO_SEL=");
		uart_putc((char)c);
		uart_puts("\r\n");
		show_pin_brief();
		prompt();
		return 1;
	}
	switch (c) {
	case 'd':
	case 'D':
		uart_puts("\r\n");
		pinmux_dump(cur_pin);
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

static int handle_pad(int c)
{
	uint32_t v;

	switch (c) {
	case 'i':
	case 'I':
		v = pinmux_read(cur_pin);
		pinmux_set_ie(cur_pin, !(v & PIN_IE));
		show_pin_brief();
		prompt();
		return 1;
	case 'o':
	case 'O':
		warn_uart0(cur_pin);
		v = pinmux_read(cur_pin);
		pinmux_set_oe(cur_pin, !(v & PIN_OE));
		show_pin_brief();
		prompt();
		return 1;
	case 'u':
	case 'U':
		pinmux_set_pull(cur_pin, PIN_PULL_UP);
		show_pin_brief();
		prompt();
		return 1;
	case 'w':
	case 'W':
		pinmux_set_pull(cur_pin, PIN_PULL_DOWN);
		show_pin_brief();
		prompt();
		return 1;
	case 'n':
	case 'N':
		pinmux_set_pull(cur_pin, PIN_PULL_NONE);
		show_pin_brief();
		prompt();
		return 1;
	case 's':
	case 'S':
		goto_level(MENU_PAD_DS);
		return 1;
	case 'd':
	case 'D':
		uart_puts("\r\n");
		pinmux_dump(cur_pin);
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

static int handle_pad_ds(int c)
{
	unsigned ds = (pinmux_read(cur_pin) & PIN_DS_MASK) >> PIN_DS_SHIFT;
	int set = -1;

	if (c >= '0' && c <= '9')
		set = c - '0';
	else if (c >= 'a' && c <= 'f')
		set = 10 + (c - 'a');
	else if (c >= 'A' && c <= 'F')
		set = 10 + (c - 'A');

	if (set >= 0) {
		pinmux_set_ds(cur_pin, (unsigned)set);
		uart_puts("DS=");
		uart_putdec32((unsigned)set);
		uart_puts("\r\n");
		show_pin_brief();
		prompt();
		return 1;
	}
	switch (c) {
	case '+':
	case '=':
		ds = (ds + 1u) & 0xfu;
		pinmux_set_ds(cur_pin, ds);
		uart_puts("DS=");
		uart_putdec32(ds);
		uart_puts("\r\n");
		prompt();
		return 1;
	case '-':
	case '_':
		ds = (ds + 15u) & 0xfu;
		pinmux_set_ds(cur_pin, ds);
		uart_puts("DS=");
		uart_putdec32(ds);
		uart_puts("\r\n");
		prompt();
		return 1;
	case 'b':
	case 'B':
		goto_level(MENU_PAD);
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
		case MENU_PICK_PIN:
			handled = handle_pick_pin(c);
			break;
		case MENU_MUX:
			handled = handle_mux(c);
			break;
		case MENU_PAD:
			handled = handle_pad(c);
			break;
		case MENU_PAD_DS:
			handled = handle_pad_ds(c);
			break;
		default:
			handled = handle_top(c);
			break;
		}
		if (!handled) {
			/* ignore unknown; stay put */
		}
	}
}
