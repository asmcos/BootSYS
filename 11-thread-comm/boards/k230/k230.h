#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE       0x91400000UL
#define UART_CLOCK_HZ    48600000U
#define UART_BAUD        115200U

#define CPU1_HART_RSTVEC 0x91102104UL
#define CPU1_RST_CTL     0x9110100cUL

#define CLINT_BASE            0x0f04000000ULL
#define CLINT_MTIME           (CLINT_BASE + 0xbff8ULL)
#define CLINT_MTIMECMP(hart)  (CLINT_BASE + 0x4000ULL + 8ULL * (hart))

#define CLINT_TIMEBASE_HZ     27000000ULL
/* Slice rate for preemptive thread switch (both harts). */
#define TIMER_HZ              100u
#define TIMER_PERIOD          (CLINT_TIMEBASE_HZ / TIMER_HZ)

#define CLINT_MSIP(hart)      (CLINT_BASE + 4ULL * (hart))

#define MIE_MSIE              (1UL << 3)
#define MIE_MTIE              (1UL << 7)
#define MSTATUS_MIE           (1UL << 3)
#define MCAUSE_MSI            3u
#define MCAUSE_MTI            7u
#define MCAUSE_ECALL_M        11u

#define CPU1_ALIVE_MAGIC      0xC001F00Du

struct cpu1_boot_result {
	uint32_t pre_ctl;
	uint32_t assert_ctl;
	uint32_t post_ctl;
	int pre_hold_ok;
	int assert_ok;
	int released;
};

static inline void writel(uint32_t val, uintptr_t addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static inline void fence_rw(void)
{
	__asm__ volatile("fence rw, rw" ::: "memory");
}

static inline uint64_t mhartid(void)
{
	uint64_t id;
	__asm__ volatile("csrr %0, mhartid" : "=r"(id));
	return id;
}

/*
 * K230 little/big both often report CSR mhartid==0 (separate clusters).
 * Boot sets tp=0 on CPU0 and tp=1 on CPU1; use that for sched/CLINT index.
 */
static inline unsigned bootsys_cpuid(void)
{
	unsigned long tp;

	__asm__ volatile("mv %0, tp" : "=r"(tp));
	if (tp >= 2)
		tp = 0;
	return (unsigned)tp;
}

static inline uint64_t mcycle(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mcycle" : "=r"(v));
	return v;
}

static inline void udelay(uint32_t us)
{
	uint64_t start = mcycle();
	uint64_t ticks = (uint64_t)us * 800ULL;
	while (mcycle() - start < ticks)
		;
}

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex64(uint64_t v);
void uart_puthex32(uint32_t v);
void uart_puthex32_val(uint32_t v);
void uart_putdec32(uint32_t v);
int uart_try_getc(void);

void sysctl_early_init(void);
void thead_cpu_init(void);
void thead_flush_caches(void);
void thead_flush_range(uintptr_t start, uintptr_t end);
void thead_invalidate_range(uintptr_t start, uintptr_t end);
void flush_image_cache(void);

struct cpu1_boot_result cpu1_boot(uint32_t entry);
void cpu1_dump_status(const char *tag);
int cpu1_is_in_hard_reset(void);

uint64_t clint_mtime(void);
void clint_mtimecmp_write(unsigned hart, uint64_t v);
void clint_msip_set(unsigned hart, int pending);
uint32_t clint_msip_get(unsigned hart);

extern char _hart1_entry[];

#endif
