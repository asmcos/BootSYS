#include "k230.h"
#include "cache_tag.h"

#define CSR_MHCR 0x7c1

/*
 * T-Head C9xx MCINDEX (0x7d3):
 *   [31:28] RID  0=I-tag  1=I-data  2=D-tag  3=D-data
 *   [24:21] L2 way (C906 way; unused for L1 RID on C908)
 *   [20:17] L1 way (C908 / xuantie)
 *   [16:0]  index (byte address; [5:0] ignored for 64B line)
 *
 * Way is written in both fields so C906-style and C908-style decoders
 * both see the same way number.
 */
#define MCINDEX_RID_SHIFT 28
#define MCINDEX_WAY_C906  21
#define MCINDEX_WAY_C908  17
#define MCINDEX_IDX_MASK  0x1ffffUL

#define PROBE_MAGIC 0x43414348u /* 'CACH' */

static volatile uint32_t probe_line[16] __attribute__((aligned(64)));

static uint64_t csr_read(unsigned csr)
{
	uint64_t v;

	switch (csr) {
	case CSR_MHCR:
		__asm__ volatile("csrr %0, 0x7c1" : "=r"(v));
		break;
	default:
		v = 0;
		break;
	}
	return v;
}

static uint64_t mk_mcindex(unsigned rid, unsigned way, uint64_t addr)
{
	way &= 0xfu;
	return ((uint64_t)rid << MCINDEX_RID_SHIFT) |
	       ((uint64_t)way << MCINDEX_WAY_C906) |
	       ((uint64_t)way << MCINDEX_WAY_C908) |
	       (addr & MCINDEX_IDX_MASK);
}

static void cache_diag_read(unsigned rid, unsigned way, uint64_t addr,
			    uint64_t *d0, uint64_t *d1)
{
	uint64_t idx = mk_mcindex(rid, way, addr);

	__asm__ volatile("csrw 0x7d3, %0" ::"r"(idx));
	__asm__ volatile("csrw 0x7d2, %0" ::"r"(1UL));
	/*
	 * OpenC9xx latches MCDATA after the cache-array read returns.
	 * A short nop pad is enough on C906/C910; keep it generous.
	 */
	__asm__ volatile(
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
		::: "memory");
	__asm__ volatile("csrr %0, 0x7d4" : "=r"(*d0));
	__asm__ volatile("csrr %0, 0x7d5" : "=r"(*d1));
}

/* C9xx MCDATA0 tag: V=bit0, D=bit2, tag=bits[39:12] (PA[39:12]). */
static unsigned tag_valid(uint64_t d0)
{
	return (unsigned)(d0 & 1u);
}

static unsigned tag_dirty(uint64_t d0)
{
	return (unsigned)((d0 >> 2) & 1u);
}

static uint64_t tag_pa(uint64_t d0)
{
	return d0 & 0x000000fffffff000ULL;
}

static unsigned addr_set(uint64_t addr)
{
	return (unsigned)((addr >> 6) & (L1_SETS - 1));
}

static void print_way(unsigned way, uint64_t d0, uint64_t d1, uint64_t expect_line)
{
	unsigned v = tag_valid(d0);
	uint64_t pa = tag_pa(d0);

	uart_puts("  way ");
	uart_putdec32(way);
	uart_puts("  V=");
	uart_putdec32(v);
	uart_puts(" D=");
	uart_putdec32(tag_dirty(d0));
	uart_puts("  pa~=");
	uart_puthex64(pa);
	if (v && expect_line && (pa & ~0xfffULL) == (expect_line & ~0xfffULL))
		uart_puts("  <- match 4K");
	else if (v && expect_line && (pa & ~0x3fULL) == (expect_line & ~0x3fULL))
		uart_puts("  <- match line");
	uart_puts("\r\n      raw ");
	uart_puthex64(d0);
	uart_puts(" ");
	uart_puthex64(d1);
	uart_puts("\r\n");
}

static void dump_set(const char *what, unsigned rid, uint64_t addr)
{
	unsigned way;
	uint64_t d0, d1;
	uint64_t line = addr & ~(uint64_t)(L1_LINE - 1);

	uart_puts(what);
	uart_puts(" set=");
	uart_putdec32(addr_set(addr));
	uart_puts("  addr=");
	uart_puthex64(addr);
	uart_puts("\r\n");

	for (way = 0; way < L1_WAYS; way++) {
		cache_diag_read(rid, way, addr, &d0, &d1);
		print_way(way, d0, d1, line);
	}
}

void cache_tag_init(void)
{
	uart_puts("L1 32KiB 4-way 64B  sets=");
	uart_putdec32(L1_SETS);
	uart_puts("  mhcr=");
	uart_puthex64(csr_read(CSR_MHCR));
	uart_puts("\r\nprobe line @ ");
	uart_puthex64((uintptr_t)probe_line);
	uart_puts("\r\n");
}

static void touch_probe(void)
{
	probe_line[0] = PROBE_MAGIC;
	/* Force a load so the line is resident (store may already have filled). */
	if (probe_line[0] != PROBE_MAGIC)
		uart_puts("probe magic mismatch\r\n");
}

void test_dcache_touch_dump(void)
{
	uart_puts("\r\n>>> D-tag: touch probe, dump that set\r\n");
	touch_probe();
	dump_set("D-cache", RID_DCACHE_TAG, (uintptr_t)probe_line);
	uart_puts("expect: one way V=1, pa~ probe (or same 4K)\r\n");
}

static void icache_target(void)
{
	__asm__ volatile("nop; nop; nop; nop" ::: "memory");
}

void test_icache_dump(void)
{
	uintptr_t pc = (uintptr_t)icache_target;

	uart_puts("\r\n>>> I-tag: call target, dump its set\r\n");
	icache_target();
	dump_set("I-cache", RID_ICACHE_TAG, pc);
	uart_puts("expect: one way V=1 covering the function\r\n");
}

void test_dcache_flush_dump(void)
{
	uart_puts("\r\n>>> D-tag: flush L1, dump probe set (do not retouch)\r\n");
	touch_probe();
	thead_flush_caches();
	dump_set("D-cache", RID_DCACHE_TAG, (uintptr_t)probe_line);
	uart_puts("expect: probe line V=0 (UART/stack may fill other sets)\r\n");
}

void test_dcache_scan_valid(void)
{
	unsigned set, way, n = 0;
	uint64_t d0, d1;
	uint64_t addr;

	uart_puts("\r\n>>> D-tag scan (V=1 only)\r\n");
	for (set = 0; set < L1_SETS; set++) {
		addr = (uint64_t)set << 6;
		for (way = 0; way < L1_WAYS; way++) {
			cache_diag_read(RID_DCACHE_TAG, way, addr, &d0, &d1);
			if (!tag_valid(d0))
				continue;
			n++;
			uart_puts("  set ");
			uart_putdec32(set);
			uart_puts(" way ");
			uart_putdec32(way);
			uart_puts(" D=");
			uart_putdec32(tag_dirty(d0));
			uart_puts(" pa~=");
			uart_puthex64(tag_pa(d0));
			uart_puts("\r\n");
		}
	}
	uart_puts("valid lines: ");
	uart_putdec32(n);
	uart_puts("\r\n");
}
