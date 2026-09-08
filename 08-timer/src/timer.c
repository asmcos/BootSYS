#include "k230.h"
#include "timer.h"

volatile uint32_t g_ticks;
volatile int g_running;

static uint64_t g_cmp;

static void csr_set_mie_mtie(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(MIE_MTIE));
}

static void csr_clear_mie_mtie(void)
{
	__asm__ volatile("csrc mie, %0" ::"r"(MIE_MTIE));
}

static void csr_set_mstatus_mie(void)
{
	__asm__ volatile("csrs mstatus, %0" ::"r"(MSTATUS_MIE));
}

void timer_init(void)
{
	g_ticks = 0;
	g_running = 0;
	g_cmp = 0;
	/* mtimecmp often resets to 0 → MTIP already pending. Park it. */
	clint_mtimecmp_write(0, ~0ULL);
	csr_clear_mie_mtie();
}

void timer_start(void)
{
	if (g_running)
		return;
	g_cmp = clint_mtime() + TIMER_PERIOD;
	clint_mtimecmp_write(0, g_cmp);
	g_running = 1;
	csr_set_mie_mtie();
	csr_set_mstatus_mie();
}

void timer_pause(void)
{
	csr_clear_mie_mtie();
	g_running = 0;
}

void timer_reset(void)
{
	timer_pause();
	g_ticks = 0;
}

void timer_on_irq(void)
{
	g_cmp += TIMER_PERIOD;
	clint_mtimecmp_write(0, g_cmp);
	if (g_running)
		g_ticks++;
}

int timer_running(void)
{
	return g_running;
}

uint32_t timer_ticks(void)
{
	return g_ticks;
}
