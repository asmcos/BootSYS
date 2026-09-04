#include "board.h"

#define RBR  0x00
#define THR  0x00
#define DLL  0x00
#define DLM  0x04
#define IER  0x04
#define FCR  0x08
#define LCR  0x0c
#define MCR  0x10
#define LSR  0x14

#define LCR_DLAB   (1u << 7)
#define LCR_WLEN8  0x03u
#define LSR_THRE   (1u << 5)
#define LSR_DR     (1u << 0)

static inline uint32_t reg_read(uintptr_t off)
{
	return readl(UART0_BASE + off);
}

static inline void reg_write(uintptr_t off, uint32_t v)
{
	writel(v, UART0_BASE + off);
}

void uart_init(void)
{
	uint32_t div = UART_CLOCK_HZ / (16U * UART_BAUD);
	if (div == 0)
		div = 1;

	reg_write(IER, 0);
	reg_write(FCR, 0x07);
	reg_write(LCR, LCR_DLAB);
	reg_write(DLL, div & 0xff);
	reg_write(DLM, (div >> 8) & 0xff);
	reg_write(LCR, LCR_WLEN8);
	reg_write(MCR, 0);
}

void uart_putc(char c)
{
	while ((reg_read(LSR) & LSR_THRE) == 0)
		;
	reg_write(THR, (uint8_t)c);
}

void uart_puts(const char *s)
{
	while (*s)
		uart_putc(*s++);
}

void uart_puthex32_val(uint32_t v)
{
	static const char hex[] = "0123456789abcdef";

	for (int i = 28; i >= 0; i -= 4)
		uart_putc(hex[(v >> i) & 0xf]);
}

void uart_puthex32(uint32_t v)
{
	uart_puts("0x");
	uart_puthex32_val(v);
}

void uart_putdec32(uint32_t v)
{
	char buf[11];
	int i = 0;

	if (v == 0) {
		uart_putc('0');
		return;
	}
	while (v) {
		buf[i++] = (char)('0' + (v % 10));
		v /= 10;
	}
	while (i--)
		uart_putc(buf[i]);
}

void uart_puthex64(uint64_t v)
{
	static const char hex[] = "0123456789abcdef";

	uart_puts("0x");
	for (int i = 60; i >= 0; i -= 4)
		uart_putc(hex[(v >> i) & 0xf]);
}

int uart_try_getc(void)
{
	if ((reg_read(LSR) & LSR_DR) == 0)
		return -1;
	return (int)(reg_read(RBR) & 0xff);
}
