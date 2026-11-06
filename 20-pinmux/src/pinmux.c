#include "k230.h"
#include "pinmux.h"

static uintptr_t pin_reg(unsigned pin)
{
	if (pin < IOMUX_NPINS)
		return IOMUX_BASE + 4UL * pin;
	if (pin < IOMUX_NPINS + PMU_IOMUX_NPINS)
		return PMU_IOMUX_BASE + 4UL * (pin - IOMUX_NPINS);
	return 0;
}

int pinmux_valid(unsigned pin)
{
	return pin < (IOMUX_NPINS + PMU_IOMUX_NPINS);
}

uint32_t pinmux_read(unsigned pin)
{
	uintptr_t r = pin_reg(pin);

	if (!r)
		return 0;
	return readl(r);
}

int pinmux_write(unsigned pin, uint32_t val)
{
	uintptr_t r = pin_reg(pin);

	if (!r)
		return -1;
	/* Never clear DI (RO); mask it out on write. */
	writel(val & ~PIN_DI, r);
	fence_rw();
	return 0;
}

unsigned pinmux_get_sel(unsigned pin)
{
	return (pinmux_read(pin) & PIN_SEL_MASK) >> PIN_SEL_SHIFT;
}

int pinmux_set_sel(unsigned pin, unsigned sel)
{
	uint32_t v;

	if (!pinmux_valid(pin) || sel > 7u)
		return -1;
	v = pinmux_read(pin);
	v = (v & ~PIN_SEL_MASK) | ((sel & 7u) << PIN_SEL_SHIFT);
	return pinmux_write(pin, v);
}

int pinmux_set_ie(unsigned pin, int on)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	v = pinmux_read(pin);
	if (on)
		v |= PIN_IE;
	else
		v &= ~PIN_IE;
	return pinmux_write(pin, v);
}

int pinmux_set_oe(unsigned pin, int on)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	v = pinmux_read(pin);
	if (on)
		v |= PIN_OE;
	else
		v &= ~PIN_OE;
	return pinmux_write(pin, v);
}

int pinmux_set_pull(unsigned pin, enum pin_pull pull)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	v = pinmux_read(pin);
	v &= ~(PIN_PU | PIN_PD);
	if (pull == PIN_PULL_UP)
		v |= PIN_PU;
	else if (pull == PIN_PULL_DOWN)
		v |= PIN_PD;
	return pinmux_write(pin, v);
}

int pinmux_set_ds(unsigned pin, unsigned ds)
{
	uint32_t v;

	if (!pinmux_valid(pin) || ds > 15u)
		return -1;
	v = pinmux_read(pin);
	v = (v & ~PIN_DS_MASK) | ((ds & 0xfu) << PIN_DS_SHIFT);
	return pinmux_write(pin, v);
}

int pinmux_set_sl(unsigned pin, int on)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	v = pinmux_read(pin);
	if (on)
		v |= PIN_SL;
	else
		v &= ~PIN_SL;
	return pinmux_write(pin, v);
}

int pinmux_set_st(unsigned pin, int on)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	v = pinmux_read(pin);
	if (on)
		v |= PIN_ST;
	else
		v &= ~PIN_ST;
	return pinmux_write(pin, v);
}

int pinmux_as_gpio(unsigned pin, int output)
{
	uint32_t v;

	if (!pinmux_valid(pin))
		return -1;
	/* alt0 = GPIO for io0–63. Keep MSC; set IE, optional OE, ds=8. */
	v = pinmux_read(pin);
	v &= ~(PIN_SEL_MASK | PIN_PU | PIN_PD | PIN_OE | PIN_IE | PIN_DS_MASK);
	v |= (0u << PIN_SEL_SHIFT) | PIN_IE | (8u << PIN_DS_SHIFT);
	if (output)
		v |= PIN_OE;
	return pinmux_write(pin, v);
}

void pinmux_restore_uart0(void)
{
	uint32_t v;

	/* io38/39 alt1 = UART0_TXD/RXD; IE+OE+ds8+SL (match SDK dts). */
	v = (1u << PIN_SEL_SHIFT) | PIN_IE | PIN_OE | (8u << PIN_DS_SHIFT) | PIN_SL;
	pinmux_write(PINMUX_UART0_TX, v);
	pinmux_write(PINMUX_UART0_RX, v);
}

void pinmux_dump(unsigned pin)
{
	uint32_t v;
	unsigned sel, ds;
	int i;

	if (!pinmux_valid(pin)) {
		uart_putc('?');
		uart_putc('\r');
		uart_putc('\n');
		return;
	}
	v = pinmux_read(pin);
	sel = (v & PIN_SEL_MASK) >> PIN_SEL_SHIFT;
	ds = (v & PIN_DS_MASK) >> PIN_DS_SHIFT;

	/*
	 * Print with putc/putdec only. pinmux.o .rodata is linked near the
	 * end of the image; a 0x1400-byte JTAG load truncates "raw="/hex
	 * and looks like UTF-8 garbage on the console.
	 */
	uart_putc('i');
	uart_putc('o');
	uart_putdec32(pin);
	uart_putc(' ');
	uart_putc('r');
	uart_putc('=');
	uart_putc('0');
	uart_putc('x');
	for (i = 28; i >= 0; i -= 4) {
		unsigned n = (v >> (unsigned)i) & 0xfu;

		uart_putc((char)(n < 10u ? '0' + n : 'a' + n - 10u));
	}
	uart_putc(' ');
	uart_putc('s');
	uart_putc('=');
	uart_putdec32(sel);
	uart_putc(' ');
	uart_putc('I');
	uart_putc('=');
	uart_putc((v & PIN_IE) ? '1' : '0');
	uart_putc(' ');
	uart_putc('O');
	uart_putc('=');
	uart_putc((v & PIN_OE) ? '1' : '0');
	uart_putc(' ');
	uart_putc('U');
	uart_putc('=');
	uart_putc((v & PIN_PU) ? '1' : '0');
	uart_putc(' ');
	uart_putc('D');
	uart_putc('=');
	uart_putc((v & PIN_PD) ? '1' : '0');
	uart_putc(' ');
	uart_putc('d');
	uart_putc('s');
	uart_putc('=');
	uart_putdec32(ds);
	uart_putc(' ');
	uart_putc('L');
	uart_putc('=');
	uart_putc((v & PIN_SL) ? '1' : '0');
	uart_putc(' ');
	uart_putc('T');
	uart_putc('=');
	uart_putc((v & PIN_ST) ? '1' : '0');
	uart_putc(' ');
	uart_putc('i');
	uart_putc('=');
	uart_putc((v & PIN_DI) ? '1' : '0');
	uart_putc('\r');
	uart_putc('\n');
}
