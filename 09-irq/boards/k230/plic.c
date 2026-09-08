#include "k230.h"

/* SiFive / T-Head C900 PLIC: context 0 = hart0 machine-mode. */
#define PLIC_PRIORITY(s)   (PLIC_BASE + 4ULL * (s))
#define PLIC_ENABLE(ctx)   (PLIC_BASE + 0x2000ULL + 0x80ULL * (ctx))
#define PLIC_THRESHOLD(ctx) (PLIC_BASE + 0x200000ULL + 0x1000ULL * (ctx))
#define PLIC_CLAIM(ctx)    (PLIC_THRESHOLD(ctx) + 4ULL)

void plic_uart_init(void)
{
	uintptr_t en = (uintptr_t)PLIC_ENABLE(PLIC_CTX_M);
	uint32_t word = UART0_PLIC_SRC / 32u;
	uint32_t bit = UART0_PLIC_SRC % 32u;
	uint32_t v;

	writel(1u, (uintptr_t)PLIC_PRIORITY(UART0_PLIC_SRC));
	writel(0u, (uintptr_t)PLIC_THRESHOLD(PLIC_CTX_M));

	v = readl(en + 4u * word);
	v |= (1u << bit);
	writel(v, en + 4u * word);
	fence_rw();
}

uint32_t plic_claim(void)
{
	uint32_t src = readl((uintptr_t)PLIC_CLAIM(PLIC_CTX_M));

	fence_rw();
	return src;
}

void plic_complete(uint32_t src)
{
	writel(src, (uintptr_t)PLIC_CLAIM(PLIC_CTX_M));
	fence_rw();
}
