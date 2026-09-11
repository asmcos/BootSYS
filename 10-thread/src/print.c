#include "k230.h"
#include "print.h"
#include <stdarg.h>

struct print_slot {
	volatile uint32_t ready;
	char text[PRINT_WIDTH];
};

struct print_ring {
	volatile uint32_t w;
	volatile uint32_t r;
	struct print_slot slot[PRINT_SLOTS];
};

static inline volatile struct print_ring *pring(void)
{
	return (volatile struct print_ring *)PRINT_BASE;
}

static uint32_t amoadd_u32(volatile uint32_t *p, uint32_t v)
{
	uint32_t old;

	__asm__ volatile("amoadd.w.aqrl %0, %2, (%1)"
			 : "=r"(old)
			 : "r"(p), "r"(v)
			 : "memory");
	return old;
}

static void push_ring(void)
{
	thead_flush_range(PRINT_BASE, PRINT_BASE + sizeof(struct print_ring));
}

static void pull_ring(void)
{
	thead_flush_caches();
}

static void copy_line(char *dst, const char *src)
{
	uint32_t i;

	for (i = 0; i + 1 < PRINT_WIDTH && src && src[i]; i++)
		dst[i] = src[i];
	dst[i] = '\0';
}

void print_init(void)
{
	volatile struct print_ring *l = pring();
	uint32_t i, j;

	l->w = 0;
	l->r = 0;
	for (i = 0; i < PRINT_SLOTS; i++) {
		l->slot[i].ready = 0;
		for (j = 0; j < PRINT_WIDTH; j++)
			l->slot[i].text[j] = 0;
	}
	fence_rw();
	push_ring();
	thead_flush_caches();
}

void print_puts(const char *s)
{
	volatile struct print_ring *l = pring();
	uint32_t idx, i;

	pull_ring();
	if ((l->w - l->r) >= PRINT_SLOTS)
		return;

	idx = amoadd_u32(&l->w, 1u);
	i = idx % PRINT_SLOTS;
	copy_line((char *)l->slot[i].text, s ? s : "");
	fence_rw();
	l->slot[i].ready = 1;
	fence_rw();
	push_ring();
}

static void append_char(char *buf, uint32_t *pos, char c)
{
	if (*pos + 1 < PRINT_WIDTH)
		buf[(*pos)++] = c;
}

static void append_str(char *buf, uint32_t *pos, const char *s)
{
	if (!s)
		s = "(null)";
	while (*s)
		append_char(buf, pos, *s++);
}

static void append_u(char *buf, uint32_t *pos, uint32_t v, unsigned base, int dig)
{
	char tmp[16];
	int n = 0;
	uint32_t x = v;

	if (x == 0)
		tmp[n++] = '0';
	else {
		while (x) {
			uint32_t d = x % base;

			tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + (d - 10));
			x /= base;
		}
	}
	while (dig > n) {
		append_char(buf, pos, '0');
		dig--;
	}
	while (n--)
		append_char(buf, pos, tmp[n]);
}

void print_printf(const char *fmt, ...)
{
	char buf[PRINT_WIDTH];
	uint32_t p = 0;
	va_list ap;
	const char *f;

	va_start(ap, fmt);
	for (f = fmt; f && *f && p + 1 < PRINT_WIDTH; f++) {
		if (*f != '%') {
			append_char(buf, &p, *f);
			continue;
		}
		f++;
		if (!*f)
			break;
		switch (*f) {
		case '%':
			append_char(buf, &p, '%');
			break;
		case 'c':
			append_char(buf, &p, (char)va_arg(ap, int));
			break;
		case 's':
			append_str(buf, &p, va_arg(ap, const char *));
			break;
		case 'd': {
			int v = va_arg(ap, int);

			if (v < 0) {
				append_char(buf, &p, '-');
				append_u(buf, &p, (uint32_t)(-v), 10, 0);
			} else {
				append_u(buf, &p, (uint32_t)v, 10, 0);
			}
			break;
		}
		case 'u':
			append_u(buf, &p, va_arg(ap, uint32_t), 10, 0);
			break;
		case 'x':
			append_u(buf, &p, va_arg(ap, uint32_t), 16, 0);
			break;
		default:
			append_char(buf, &p, '%');
			append_char(buf, &p, *f);
			break;
		}
	}
	va_end(ap);
	buf[p] = '\0';
	print_puts(buf);
}

static void drain_budget(uint32_t budget)
{
	volatile struct print_ring *l = pring();

	if (bootsys_cpuid() != 0)
		return;

	pull_ring();
	while (budget-- && l->r != l->w) {
		uint32_t r = l->r;
		uint32_t i = r % PRINT_SLOTS;

		if (!l->slot[i].ready)
			break;
		uart_puts((const char *)l->slot[i].text);
		uart_puts("\r\n");
		l->slot[i].ready = 0;
		fence_rw();
		l->r = r + 1;
		fence_rw();
		push_ring();
		pull_ring();
	}
}

void print_drain_isr(void)
{
	drain_budget(PRINT_ISR_MAX);
}

void print_drain_all(void)
{
	uint32_t guard = PRINT_SLOTS + 4u;

	while (guard--) {
		if (print_pending() == 0)
			return;
		drain_budget(PRINT_SLOTS);
	}
}

uint32_t print_pending(void)
{
	volatile struct print_ring *l = pring();
	uint32_t pending;

	if (bootsys_cpuid() != 0)
		return 0;
	pull_ring();
	pending = l->w - l->r;
	if (pending > PRINT_SLOTS)
		return 0;
	return pending;
}
