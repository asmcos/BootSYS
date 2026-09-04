/* Boot logo + info panel: continuous strokes, arc corners (UTF-8). */
#include "k230.h"
#include "memmap.h"
#include "banner.h"

#ifndef BOOTSYS_VERSION
#define BOOTSYS_VERSION "0.1.0"
#endif

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

/* Display columns between "│ " and " │" (ASCII payload only). */
#define INNER 56

static void nl(void)
{
	uart_puts("\r\n");
}

static void stroke(unsigned n)
{
	while (n--)
		uart_puts("─");
}

static void hline_top(void)
{
	uart_puts("╭");
	stroke(INNER + 2);
	uart_puts("╮");
	nl();
}

static void hline_mid(void)
{
	uart_puts("├");
	stroke(INNER + 2);
	uart_puts("┤");
	nl();
}

static void hline_bot(void)
{
	uart_puts("╰");
	stroke(INNER + 2);
	uart_puts("╯");
	nl();
}

static void pad_rest(unsigned used)
{
	while (used < INNER) {
		uart_putc(' ');
		used++;
	}
}

static unsigned strlen_u(const char *s)
{
	unsigned n = 0;
	while (s[n])
		n++;
	return n;
}

static void row_text(const char *s)
{
	uart_puts("│ ");
	uart_puts(s);
	pad_rest(strlen_u(s));
	uart_puts(" │");
	nl();
}

static void row_kv(const char *key, const char *val)
{
	uart_puts("│ ");
	uart_puts(key);
	uart_puts(val);
	pad_rest(strlen_u(key) + strlen_u(val));
	uart_puts(" │");
	nl();
}

static unsigned dec_digits32(uint32_t v)
{
	unsigned n = 0;
	if (v == 0)
		return 1;
	while (v) {
		v /= 10;
		n++;
	}
	return n;
}

static void row_kv_hex(const char *key, uint32_t v)
{
	uart_puts("│ ");
	uart_puts(key);
	uart_puts("0x");
	uart_puthex32_val(v);
	pad_rest(strlen_u(key) + 2 + 8);
	uart_puts(" │");
	nl();
}

static void row_kv_dec(const char *key, uint32_t v)
{
	uart_puts("│ ");
	uart_puts(key);
	uart_putdec32(v);
	pad_rest(strlen_u(key) + dec_digits32(v));
	uart_puts(" │");
	nl();
}

/* Continuous rounded emblem — no lettermark, arc corners only. */
static void print_logo(void)
{
	nl();
	uart_puts("        ╭──────────────────────────────╮");
	nl();
	uart_puts("   ╭────╯                              ╰────╮");
	nl();
	uart_puts("   │                                        │");
	nl();
	uart_puts("   │       RISC-V  ·  bare-metal            │");
	nl();
	uart_puts("   │       low-level verification           │");
	nl();
	uart_puts("   │                                        │");
	nl();
	uart_puts("   ╰────╮                              ╭────╯");
	nl();
	uart_puts("        ╰──────────────────────────────╯");
	nl();
	nl();
}

void print_boot_banner(void)
{
	print_logo();

	hline_top();
	row_text("  bare-metal bring-up  (not OS / not bootloader)");
	hline_mid();
	row_kv("  board     ", "K230 (Canaan C908 dual-core)");
	row_kv_hex("  load      ", (uint32_t)LOAD_BASE);
	row_kv_hex("  shmem     ", (uint32_t)SHMEM_BASE);
	row_kv_hex("  uart0     ", (uint32_t)UART0_BASE);
	row_kv_dec("  baud      ", UART_BAUD);
	row_kv("  build     ", __DATE__ " " __TIME__);
	row_kv("  version   ", BOOTSYS_VERSION);
	row_kv("  git       ", BOOTSYS_GIT);
	row_kv("  arch      ", "rv64imac / M-mode / freestanding C");
	row_text("  status    ready");
	hline_bot();
	nl();
}
