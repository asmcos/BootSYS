#include "k230.h"
#include "uart_irq.h"

#define RING_N 32

static volatile uint8_t ring[RING_N];
static volatile unsigned ring_h, ring_t;
static volatile uint32_t irq_count;
static volatile int irq_on;

static void csr_set_mie_meie(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(MIE_MEIE));
}

static void csr_clear_mie_meie(void)
{
	__asm__ volatile("csrc mie, %0" ::"r"(MIE_MEIE));
}

static void csr_set_mstatus_mie(void)
{
	__asm__ volatile("csrs mstatus, %0" ::"r"(MSTATUS_MIE));
}

void uart_irq_init(void)
{
	ring_h = ring_t = 0;
	irq_count = 0;
	irq_on = 0;
	plic_uart_init();
	uart_rx_irq_enable(0);
	csr_clear_mie_meie();
}

void uart_irq_on(void)
{
	irq_on = 1;
	uart_rx_irq_enable(1);
	csr_set_mie_meie();
	csr_set_mstatus_mie();
}

void uart_irq_off(void)
{
	uart_rx_irq_enable(0);
	csr_clear_mie_meie();
	irq_on = 0;
}

void uart_irq_on_mei(void)
{
	uint32_t src = plic_claim();

	if (src == UART0_PLIC_SRC) {
		int c;

		while ((c = uart_rx_take()) >= 0) {
			unsigned n = (ring_h + 1u) % RING_N;

			irq_count++;
			if (n != ring_t) {
				ring[ring_h] = (uint8_t)c;
				ring_h = n;
			}
		}
	}
	if (src)
		plic_complete(src);
}

int uart_irq_mode(void)
{
	return irq_on;
}

uint32_t uart_irq_count(void)
{
	return irq_count;
}

int uart_irq_getc(void)
{
	int c;
	unsigned t;

	if (ring_t == ring_h)
		return -1;
	t = ring_t;
	c = ring[t];
	ring_t = (t + 1u) % RING_N;
	return c;
}
