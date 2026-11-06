/*
 * 21-gpio — LED + pinmux prove on 嘉立创庐山派 K230-CanMV
 *
 *   R=62  G=20  B=63  active-low; KEY=53
 * Menu 5: mux GPIO vs non-GPIO — same gpio_write, lamp only works on alt0.
 */
#include "k230.h"
#include "pinmux.h"
#include "gpio.h"
#include "trap.h"

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

#define LED_R_PIN       62u
#define LED_G_PIN       20u
#define LED_B_PIN       63u
#define KEY_PIN         53u
#define LED_ACTIVE_LOW  1

enum {
	MENU_TOP = 0,
	MENU_LED,
	MENU_MUX,
	MENU_KEY,
	MENU_ADV,
};

static int menu_level = MENU_TOP;
static unsigned cur_led = LED_G_PIN;

static void mei_on(void)
{
	uint64_t mie, mstatus;

	__asm__ volatile("csrr %0, mie" : "=r"(mie));
	mie |= MIE_MEIE;
	__asm__ volatile("csrw mie, %0" ::"r"(mie));
	__asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
	mstatus |= MSTATUS_MIE;
	__asm__ volatile("csrw mstatus, %0" ::"r"(mstatus));
}

static void banner(void)
{
	uart_puts("\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("|  BootSYS  -  21-gpio                         |\r\n");
	uart_puts("|  LED + pinmux prove (20+21)                  |\r\n");
	uart_puts("|  build   " __DATE__ " " __TIME__ "              |\r\n");
	uart_puts("|  git     " BOOTSYS_GIT "                           |\r\n");
	uart_puts("+----------------------------------------------+\r\n");
	uart_puts("\r\n");
	uart_puts("board: JLC Lushan K230-CanMV\r\n");
	uart_puts("  R=io62  G=io20  B=io63  (0=ON)\r\n");
	uart_puts("  KEY=io53\r\n");
}

static const char *alt1_name(unsigned pin)
{
	if (pin == LED_G_PIN)
		return "OSPI_D4";
	if (pin == LED_R_PIN)
		return "M_CLK2";
	if (pin == LED_B_PIN)
		return "M_CLK3";
	return "alt1";
}

static const char *alt2_name(unsigned pin)
{
	if (pin == LED_G_PIN)
		return "QSPI1_CS0";
	if (pin == LED_R_PIN)
		return "UART3_DE";
	if (pin == LED_B_PIN)
		return "UART3_RE";
	return "alt2";
}

static void led_prepare_gpio(unsigned pin)
{
	pinmux_as_gpio(pin, 1);
	pinmux_set_ds(pin, 8);
	pinmux_set_pull(pin, PIN_PULL_NONE);
	gpio_set_dir(pin, GPIO_OUT);
}

/* Drive GPIO data reg only — does NOT change pinmux. */
static void led_gpio_level(unsigned pin, int on)
{
	int level;

	gpio_set_dir(pin, GPIO_OUT);
#if LED_ACTIVE_LOW
	level = on ? 0 : 1;
#else
	level = on ? 1 : 0;
#endif
	gpio_write(pin, level);
}

static void led_write_on(unsigned pin, int on)
{
	led_prepare_gpio(pin);
	led_gpio_level(pin, on);
}

static void led_all_off(void)
{
	led_write_on(LED_R_PIN, 0);
	led_write_on(LED_G_PIN, 0);
	led_write_on(LED_B_PIN, 0);
}

static void busy_delay(uint32_t n)
{
	volatile uint32_t i;

	for (i = 0; i < n; i++)
		;
}

static void show_mux(unsigned pin)
{
	uint32_t v = pinmux_read(pin);
	unsigned sel = (v & PIN_SEL_MASK) >> PIN_SEL_SHIFT;

	uart_puts("io");
	uart_putdec32(pin);
	uart_puts(" sel=");
	uart_putdec32(sel);
	uart_puts(sel == 0u ? " (GPIO)" : " (NOT gpio)");
	uart_puts(" IE=");
	uart_putc((v & PIN_IE) ? '1' : '0');
	uart_puts(" OE=");
	uart_putc((v & PIN_OE) ? '1' : '0');
	uart_puts("\r\n");
}

static void show_led_state(void)
{
	int r = gpio_read(LED_R_PIN);
	int g = gpio_read(LED_G_PIN);
	int b = gpio_read(LED_B_PIN);

	uart_puts("GPIO DR readback R=");
	uart_putc(r < 0 ? '?' : (r ? '1' : '0'));
	uart_puts(" G=");
	uart_putc(g < 0 ? '?' : (g ? '1' : '0'));
	uart_puts(" B=");
	uart_putc(b < 0 ? '?' : (b ? '1' : '0'));
	uart_puts("\r\n");
}

static void menu_top(void)
{
	uart_puts("\r\n======== top ========\r\n");
	uart_puts("  1  LED on/off/blink\r\n");
	uart_puts("  2  read user key\r\n");
	uart_puts("  3  all LEDs off\r\n");
	uart_puts("  4  rainbow demo\r\n");
	uart_puts("  5  pinmux prove (GPIO vs not)  <--- 20+21\r\n");
	uart_puts("  9  advanced IRQ\r\n");
	uart_puts("  h  help\r\n");
	uart_puts("=====================\r\n");
}

static void menu_led(void)
{
	uart_puts("\r\n======== LED ========\r\n");
	uart_puts("color: ");
	if (cur_led == LED_R_PIN)
		uart_puts("RED(io62)");
	else if (cur_led == LED_G_PIN)
		uart_puts("GREEN(io20)");
	else
		uart_puts("BLUE(io63)");
	uart_puts("\r\n");
	uart_puts("  r/g/u   select RED / GREEN / BLUE\r\n");
	uart_puts("  1 / 0   ON / OFF (mux GPIO first)\r\n");
	uart_puts("  t / s   toggle / blink\r\n");
	uart_puts("  m       jump to pinmux prove\r\n");
	uart_puts("  d       dump\r\n");
	uart_puts("  b       back\r\n");
	uart_puts("=====================\r\n");
}

static void menu_mux(void)
{
	uart_puts("\r\n======== pinmux prove ========\r\n");
	uart_puts("LED pin io");
	uart_putdec32(cur_led);
	uart_puts("  (same gpio_write; mux decides pad)\r\n");
	show_mux(cur_led);
	uart_puts("  r/g/u  pick LED color first\r\n");
	uart_puts("  0      mux alt0=GPIO, then ON   expect LIT\r\n");
	uart_puts("  1      mux alt1=");
	uart_puts(alt1_name(cur_led));
	uart_puts(", write ON  expect DARK\r\n");
	uart_puts("  2      mux alt2=");
	uart_puts(alt2_name(cur_led));
	uart_puts(", write ON  expect DARK\r\n");
	uart_puts("  s      auto: GPIO lit -> alt1 dark -> GPIO lit\r\n");
	uart_puts("  d      dump pinmux + gpio\r\n");
	uart_puts("  b      back\r\n");
	uart_puts("================================\r\n");
}

static void menu_key(void)
{
	uart_puts("\r\n======== KEY ========\r\n");
	uart_puts("  r   read once\r\n");
	uart_puts("  p   poll 3s\r\n");
	uart_puts("  b   back\r\n");
	uart_puts("=====================\r\n");
}

static void menu_adv(void)
{
	uart_puts("\r\n======== advanced ========\r\n");
	uart_puts("  s   IRQ self-test on cur LED\r\n");
	uart_puts("  b   back\r\n");
	uart_puts("==========================\r\n");
}

static void prompt(void)
{
	uart_puts("\r\n[");
	switch (menu_level) {
	case MENU_LED:
		uart_puts("led");
		break;
	case MENU_MUX:
		uart_puts("mux");
		break;
	case MENU_KEY:
		uart_puts("key");
		break;
	case MENU_ADV:
		uart_puts("adv");
		break;
	default:
		uart_puts("top");
		break;
	}
	uart_puts(" io");
	uart_putdec32(cur_led);
	uart_puts("] > ");
}

static void show_menu(void)
{
	switch (menu_level) {
	case MENU_LED:
		menu_led();
		break;
	case MENU_MUX:
		menu_mux();
		break;
	case MENU_KEY:
		menu_key();
		break;
	case MENU_ADV:
		menu_adv();
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

static void led_blink(unsigned pin, int times)
{
	int i;

	for (i = 0; i < times; i++) {
		led_write_on(pin, 1);
		busy_delay(800000u);
		led_write_on(pin, 0);
		busy_delay(800000u);
	}
}

static void rainbow(void)
{
	uart_puts("rainbow...\r\n");
	led_all_off();
	led_write_on(LED_R_PIN, 1);
	busy_delay(1200000u);
	led_write_on(LED_R_PIN, 0);
	led_write_on(LED_G_PIN, 1);
	busy_delay(1200000u);
	led_write_on(LED_G_PIN, 0);
	led_write_on(LED_B_PIN, 1);
	busy_delay(1200000u);
	led_write_on(LED_B_PIN, 0);
	uart_puts("done\r\n");
}

/*
 * Mux to sel, keep pad OE/IE for visibility, then gpio_write ON.
 * Only sel==0 connects pad to GPIO controller.
 */
static void prove_mux_then_on(unsigned pin, unsigned sel)
{
	uint32_t v;

	uart_puts("mux sel=");
	uart_putdec32(sel);
	if (sel == 0u)
		uart_puts(" GPIO");
	else if (sel == 1u) {
		uart_puts(" ");
		uart_puts(alt1_name(pin));
	} else if (sel == 2u) {
		uart_puts(" ");
		uart_puts(alt2_name(pin));
	}
	uart_puts(" -> drive ON\r\n");

	if (sel == 0u) {
		led_prepare_gpio(pin);
	} else {
		/* Leave GPIO controller asserting "on", but steal pad away. */
		led_prepare_gpio(pin);
		led_gpio_level(pin, 1);
		pinmux_set_sel(pin, sel);
		/* Pad OE often needed for alt; keep IE for dump DI. */
		v = pinmux_read(pin);
		v |= PIN_IE;
		pinmux_write(pin, v);
	}
	led_gpio_level(pin, 1);

	show_mux(pin);
	uart_puts(sel == 0u ? "LOOK: LED should be LIT\r\n"
			    : "LOOK: LED should be DARK (gpio write ignored by pad)\r\n");
}

static void prove_auto(unsigned pin)
{
	uart_puts("\r\n=== auto pinmux prove io");
	uart_putdec32(pin);
	uart_puts(" ===\r\n");

	uart_puts("[1/3] GPIO ON\r\n");
	prove_mux_then_on(pin, 0);
	busy_delay(2000000u);

	uart_puts("[2/3] alt1 ON (should go dark)\r\n");
	prove_mux_then_on(pin, 1);
	busy_delay(2000000u);

	uart_puts("[3/3] back GPIO ON (lit again)\r\n");
	prove_mux_then_on(pin, 0);
	busy_delay(1500000u);

	led_gpio_level(pin, 0);
	uart_puts("=== prove done: mux owns the pad ===\r\n");
}

static void key_setup(void)
{
	pinmux_as_gpio(KEY_PIN, 0);
	pinmux_set_pull(KEY_PIN, PIN_PULL_DOWN);
	gpio_set_dir(KEY_PIN, GPIO_IN);
}

static void key_read_once(void)
{
	int v;

	key_setup();
	v = gpio_read(KEY_PIN);
	uart_puts("KEY=");
	uart_putc(v < 0 ? '?' : (v ? '1' : '0'));
	uart_puts("\r\n");
}

static void key_poll(void)
{
	uint32_t n;

	key_setup();
	uart_puts("poll 3s...\r\n");
	for (n = 0; n < 30u; n++) {
		if (gpio_read(KEY_PIN) == 1) {
			uart_puts("pressed\r\n");
			led_write_on(LED_R_PIN, 1);
			busy_delay(400000u);
			led_write_on(LED_R_PIN, 0);
		}
		busy_delay(200000u);
	}
	uart_puts("end\r\n");
}

static void self_test_irq(void)
{
	uint32_t before = gpio_irq_count();

	pinmux_as_gpio(cur_led, 1);
	gpio_set_dir(cur_led, GPIO_OUT);
	gpio_write(cur_led, 0);
	gpio_irq_config(cur_led, GPIO_EDGE_RISING);
	gpio_irq_enable(cur_led, 1);
	mei_on();
	busy_delay(1000u);
	gpio_write(cur_led, 1);
	busy_delay(50000u);
	uart_puts("irq ");
	uart_putdec32(before);
	uart_puts(" -> ");
	uart_putdec32(gpio_irq_count());
	uart_puts(gpio_irq_count() > before ? " PASS\r\n" : " FAIL\r\n");
	led_write_on(cur_led, 0);
}

static int pick_color(int c)
{
	switch (c) {
	case 'r':
	case 'R':
		cur_led = LED_R_PIN;
		uart_puts("RED io62\r\n");
		return 1;
	case 'g':
	case 'G':
		cur_led = LED_G_PIN;
		uart_puts("GREEN io20\r\n");
		return 1;
	case 'u':
	case 'U':
		cur_led = LED_B_PIN;
		uart_puts("BLUE io63\r\n");
		return 1;
	default:
		return 0;
	}
}

static int handle_top(int c)
{
	switch (c) {
	case '1':
		goto_level(MENU_LED);
		return 1;
	case '2':
		goto_level(MENU_KEY);
		return 1;
	case '3':
		led_all_off();
		uart_puts("all OFF\r\n");
		prompt();
		return 1;
	case '4':
		rainbow();
		prompt();
		return 1;
	case '5':
		goto_level(MENU_MUX);
		return 1;
	case '9':
		goto_level(MENU_ADV);
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

static int handle_led(int c)
{
	int lv;

	if (pick_color(c)) {
		prompt();
		return 1;
	}
	switch (c) {
	case '1':
		led_write_on(cur_led, 1);
		uart_puts("ON\r\n");
		show_mux(cur_led);
		prompt();
		return 1;
	case '0':
		led_write_on(cur_led, 0);
		uart_puts("OFF\r\n");
		prompt();
		return 1;
	case 't':
	case 'T':
		led_prepare_gpio(cur_led);
		lv = gpio_read(cur_led);
		led_gpio_level(cur_led, lv != 0);
		uart_puts("toggled\r\n");
		prompt();
		return 1;
	case 's':
	case 'S':
		led_blink(cur_led, 5);
		prompt();
		return 1;
	case 'm':
	case 'M':
		goto_level(MENU_MUX);
		return 1;
	case 'd':
	case 'D':
		show_mux(cur_led);
		show_led_state();
		pinmux_dump(cur_led);
		prompt();
		return 1;
	case 'b':
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
	if (pick_color(c)) {
		show_menu();
		return 1;
	}
	switch (c) {
	case '0':
		prove_mux_then_on(cur_led, 0);
		prompt();
		return 1;
	case '1':
		prove_mux_then_on(cur_led, 1);
		prompt();
		return 1;
	case '2':
		prove_mux_then_on(cur_led, 2);
		prompt();
		return 1;
	case 's':
	case 'S':
		prove_auto(cur_led);
		prompt();
		return 1;
	case 'd':
	case 'D':
		show_mux(cur_led);
		show_led_state();
		pinmux_dump(cur_led);
		prompt();
		return 1;
	case 'b':
	case 'B':
		/* restore GPIO mux so later LED menu works */
		led_prepare_gpio(cur_led);
		led_gpio_level(cur_led, 0);
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

static int handle_key(int c)
{
	switch (c) {
	case 'r':
	case 'R':
		key_read_once();
		prompt();
		return 1;
	case 'p':
	case 'P':
		key_poll();
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

static int handle_adv(int c)
{
	switch (c) {
	case 's':
	case 'S':
		self_test_irq();
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
	sysctl_early_init();
	uart_init();
	gpio_init_hw();
	trap_init();
	mei_on();
	led_all_off();

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
		case MENU_LED:
			handled = handle_led(c);
			break;
		case MENU_MUX:
			handled = handle_mux(c);
			break;
		case MENU_KEY:
			handled = handle_key(c);
			break;
		case MENU_ADV:
			handled = handle_adv(c);
			break;
		default:
			handled = handle_top(c);
			break;
		}
		(void)handled;
	}
}
