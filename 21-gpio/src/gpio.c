#include "k230.h"
#include "gpio.h"
#include "pinmux.h"

/* Synopsys DW_apb_gpio (K230 TRM / QEMU model). */
#define GPIO_SWPORTA_DR      0x00
#define GPIO_SWPORTA_DDR     0x04
#define GPIO_SWPORTA_CTL     0x08
#define GPIO_INTEN           0x30
#define GPIO_INTMASK         0x34
#define GPIO_INTTYPE_LEVEL   0x38
#define GPIO_INT_POLARITY    0x3c
#define GPIO_INTSTATUS       0x40
#define GPIO_RAW_INTSTATUS   0x44
#define GPIO_PORTA_EOI       0x4c
#define GPIO_EXT_PORTA       0x50
#define GPIO_INT_BOTHEDGE    0x68

static volatile uint32_t g_irq_count;
static volatile int g_irq_last = -1;

static uintptr_t gpio_base(unsigned pin)
{
	if (pin < 32u)
		return GPIO0_BASE;
	if (pin < 64u)
		return GPIO1_BASE;
	return 0;
}

static unsigned gpio_bit(unsigned pin)
{
	return pin & 31u;
}

static uint32_t gpio_plic_src(unsigned pin)
{
	if (pin < 32u)
		return GPIO0_PLIC_BASE + pin;
	return GPIO1_PLIC_BASE + (pin - 32u);
}

static void gpio_wr(uintptr_t base, unsigned off, uint32_t v)
{
	writel(v, base + off);
}

static uint32_t gpio_rd(uintptr_t base, unsigned off)
{
	return readl(base + off);
}

int gpio_valid(unsigned pin)
{
	return pin < 64u;
}

void gpio_init_hw(void)
{
	/* Software control for all bits on both controllers. */
	gpio_wr(GPIO0_BASE, GPIO_SWPORTA_CTL, 0);
	gpio_wr(GPIO1_BASE, GPIO_SWPORTA_CTL, 0);
	fence_rw();
}

int gpio_set_dir(unsigned pin, enum gpio_dir dir)
{
	uintptr_t b = gpio_base(pin);
	uint32_t bit, ddr;

	if (!b)
		return -1;
	bit = 1u << gpio_bit(pin);
	ddr = gpio_rd(b, GPIO_SWPORTA_DDR);
	if (dir == GPIO_OUT)
		ddr |= bit;
	else
		ddr &= ~bit;
	gpio_wr(b, GPIO_SWPORTA_DDR, ddr);
	fence_rw();
	return 0;
}

int gpio_write(unsigned pin, int level)
{
	uintptr_t b = gpio_base(pin);
	uint32_t bit, dr;

	if (!b)
		return -1;
	bit = 1u << gpio_bit(pin);
	dr = gpio_rd(b, GPIO_SWPORTA_DR);
	if (level)
		dr |= bit;
	else
		dr &= ~bit;
	gpio_wr(b, GPIO_SWPORTA_DR, dr);
	fence_rw();
	return 0;
}

int gpio_read(unsigned pin)
{
	uintptr_t b = gpio_base(pin);
	uint32_t ext;

	if (!b)
		return -1;
	ext = gpio_rd(b, GPIO_EXT_PORTA);
	return (ext >> gpio_bit(pin)) & 1u;
}

int gpio_irq_config(unsigned pin, enum gpio_edge edge)
{
	uintptr_t b = gpio_base(pin);
	uint32_t bit, type, pol, both, inten, mask;
	uint32_t src;

	if (!b)
		return -1;

	bit = 1u << gpio_bit(pin);
	src = gpio_plic_src(pin);

	/* Start disabled. */
	inten = gpio_rd(b, GPIO_INTEN) & ~bit;
	gpio_wr(b, GPIO_INTEN, inten);

	type = gpio_rd(b, GPIO_INTTYPE_LEVEL);
	pol = gpio_rd(b, GPIO_INT_POLARITY);
	both = gpio_rd(b, GPIO_INT_BOTHEDGE);

	both &= ~bit;
	if (edge == GPIO_EDGE_NONE) {
		type &= ~bit;
		pol &= ~bit;
	} else {
		type |= bit; /* edge */
		if (edge == GPIO_EDGE_BOTH) {
			both |= bit;
			pol &= ~bit;
		} else if (edge == GPIO_EDGE_RISING) {
			pol |= bit;
		} else {
			pol &= ~bit; /* falling */
		}
	}

	gpio_wr(b, GPIO_INTTYPE_LEVEL, type);
	gpio_wr(b, GPIO_INT_POLARITY, pol);
	gpio_wr(b, GPIO_INT_BOTHEDGE, both);
	mask = gpio_rd(b, GPIO_INTMASK) & ~bit; /* unmask */
	gpio_wr(b, GPIO_INTMASK, mask);
	gpio_wr(b, GPIO_PORTA_EOI, bit); /* clear sticky */
	fence_rw();

	plic_set_priority(src, 1u);
	plic_enable(src, edge != GPIO_EDGE_NONE);
	return 0;
}

int gpio_irq_enable(unsigned pin, int on)
{
	uintptr_t b = gpio_base(pin);
	uint32_t bit, inten;
	uint32_t src;

	if (!b)
		return -1;
	bit = 1u << gpio_bit(pin);
	src = gpio_plic_src(pin);
	inten = gpio_rd(b, GPIO_INTEN);
	if (on)
		inten |= bit;
	else
		inten &= ~bit;
	gpio_wr(b, GPIO_INTEN, inten);
	if (on)
		gpio_wr(b, GPIO_PORTA_EOI, bit);
	fence_rw();
	plic_enable(src, on);
	return 0;
}

void gpio_irq_eoi(unsigned pin)
{
	uintptr_t b = gpio_base(pin);

	if (!b)
		return;
	gpio_wr(b, GPIO_PORTA_EOI, 1u << gpio_bit(pin));
	fence_rw();
}

uint32_t gpio_irq_count(void)
{
	return g_irq_count;
}

int gpio_irq_last_pin(void)
{
	return g_irq_last;
}

int gpio_on_mei(void)
{
	uint32_t src = plic_claim();
	unsigned pin;

	if (src == 0)
		return 0;

	if (src >= GPIO0_PLIC_BASE && src < GPIO0_PLIC_BASE + 32u)
		pin = src - GPIO0_PLIC_BASE;
	else if (src >= GPIO1_PLIC_BASE && src < GPIO1_PLIC_BASE + 32u)
		pin = 32u + (src - GPIO1_PLIC_BASE);
	else {
		plic_complete(src);
		return 0;
	}

	gpio_irq_eoi(pin);
	g_irq_last = (int)pin;
	g_irq_count++;
	plic_complete(src);
	return 1;
}
