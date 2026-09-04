#ifndef K230_H
#define K230_H

#include <stdint.h>

#define UART0_BASE       0x91400000UL
#define UART_CLOCK_HZ    48600000U
#define UART_BAUD        115200U

#define CPU1_HART_RSTVEC 0x91102104UL
#define CPU1_RST_CTL     0x9110100cUL

/*
 * K230 CLINT (Linux k230.dtsi): reg = <0xf 0x04000000 ...>
 * MSIP[hart] @ base + 4 * hart
 */
#define CLINT_BASE       0x0f04000000ULL
#define CLINT_MSIP(hart) (CLINT_BASE + 4ULL * (hart))

#define CPU1_ALIVE_MAGIC 0xC001F00Du

/* Shared-memory CPU1 state (software protocol — no HW WFI flag). */
#define CPU1_STATE_RESET      0u
#define CPU1_STATE_RUNNING    1u
#define CPU1_STATE_SOFT_LOOP  2u
#define CPU1_STATE_WFI_SLEEP  3u

/* CPU0 → CPU1 commands via shmem.cmd */
#define CPU1_CMD_NONE         0u
#define CPU1_CMD_GO_RUNNING   1u
#define CPU1_CMD_GO_SOFT_LOOP 2u
#define CPU1_CMD_GO_WFI       3u

struct cpu1_boot_result {
	uint32_t pre_ctl;
	uint32_t assert_ctl;
	uint32_t post_ctl;
	int pre_hold_ok;
	int assert_ok;
	int released;
};

/*
 * Shared SRAM block — CPU1 writes state/wake_count/ipi_seen;
 * CPU0 writes cmd and polls.
 */
struct cpu1_shmem {
	volatile uint32_t alive;
	volatile uint32_t tick;
	volatile uint32_t entry_pc;
	volatile uint32_t state;
	volatile uint32_t cmd;
	volatile uint32_t wake_count;
	volatile uint32_t ipi_seen;
	volatile uint32_t seq;
	volatile uint32_t console_ready; /* CPU0=1 => CPU1 may print */
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
void thead_cache_off(void);
void flush_image_cache(void);

void cpu1_power_on(void);
void cpu1_l2_flush(void);
struct cpu1_boot_result cpu1_boot(uint32_t entry);
void cpu1_dump_status(const char *tag);
const char *cpu1_ctl_phase(uint32_t ctl);
void cpu1_hold_reset(void);

/*
 * Hardware-readable: CPU1 held in RMU reset?
 * Status is bit0 of CPU1_RST_CTL — NOT bit16 (bit16 is write-enable only).
 */
int cpu1_is_in_hard_reset(void);

void clint_msip_set(unsigned hart, int pending);
uint32_t clint_msip_get(unsigned hart);
void clint_ipi_send_cpu1(void);
void clint_ipi_clear_cpu1(void);

void shmem_init(void);
int shmem_cpu1_alive(void);
uint32_t shmem_cpu1_state(void);
const char *shmem_state_name(uint32_t st);
void shmem_cpu0_cmd(uint32_t cmd);
void shmem_cpu1_mark_alive(uint32_t entry_pc);
void shmem_cpu1_set_state(uint32_t st);
void shmem_cpu1_mark_tick(uint32_t n);
void shmem_cpu1_on_wake(void);
void shmem_cpu1_on_ipi_seen(void);
void shmem_cpu0_console_ready(void);
int shmem_console_ready(void);

extern volatile struct cpu1_shmem *const cpu1_shmem;
extern char _hart1_entry[];

#endif
