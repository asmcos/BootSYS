#include "k230.h"
#include "thread.h"
#include "print.h"

struct hart_sched {
	struct thread threads[THREAD_PER_HART];
	struct trap_frame idle_frame;
	uint64_t idle_mepc;
	int current; /* -1 = idle (boot stack) */
	int enabled;
	int ncreated;
	uint32_t ticks;
	uint32_t switches; /* times current thread identity changed */
	uint64_t cmp;
};

static struct hart_sched g_sched[HART_MAX];

static inline struct hart_sched *mine(void)
{
	return &g_sched[bootsys_cpuid()];
}

static inline uint64_t r_mepc(void)
{
	uint64_t v;
	__asm__ volatile("csrr %0, mepc" : "=r"(v));
	return v;
}

static inline void w_mepc(uint64_t v)
{
	__asm__ volatile("csrw mepc, %0" ::"r"(v));
}

static void csr_set_mie_mtie(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(MIE_MTIE));
}

static void csr_set_mie_msie(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(MIE_MSIE));
}

static void csr_set_mstatus_mie(void)
{
	__asm__ volatile("csrs mstatus, %0" ::"r"(MSTATUS_MIE));
}

static void thread_trampoline(void)
{
	struct hart_sched *s = mine();
	struct thread *t = &s->threads[s->current];

	t->entry(t->arg);
	sched_mark_done();
	/* Yield forever — big core may not take timer/MSIP IRQs (see 02 poll). */
	for (;;)
		__asm__ volatile("ecall" ::: "memory");
}

static void frame_zero(struct trap_frame *f)
{
	unsigned i;
	uint64_t *p = (uint64_t *)f;

	for (i = 0; i < sizeof(*f) / sizeof(uint64_t); i++)
		p[i] = 0;
}

void sched_init_hart(void)
{
	struct hart_sched *s = mine();
	int i;

	for (i = 0; i < THREAD_PER_HART; i++) {
		s->threads[i].state = THREAD_UNUSED;
		s->threads[i].id = i;
		s->threads[i].entry = 0;
		s->threads[i].arg = 0;
		s->threads[i].mepc = 0;
		frame_zero(&s->threads[i].frame);
	}
	frame_zero(&s->idle_frame);
	s->idle_mepc = 0;
	s->current = -1;
	s->enabled = 0;
	s->ncreated = 0;
	s->ticks = 0;
	s->switches = 0;
	s->cmp = 0;

	/* Park this hart's comparator until sched_start. */
	clint_mtimecmp_write(bootsys_cpuid(), ~0ULL);
}

int sched_create(void (*entry)(void *), void *arg)
{
	struct hart_sched *s = mine();
	struct thread *t;
	uint64_t *sp;
	int i;

	for (i = 0; i < THREAD_PER_HART; i++) {
		if (s->threads[i].state == THREAD_UNUSED)
			break;
	}
	if (i >= THREAD_PER_HART)
		return -1;

	t = &s->threads[i];
	t->entry = entry;
	t->arg = arg;
	t->state = THREAD_READY;
	t->id = i;

	sp = &t->stack[THREAD_STACK_SIZE / sizeof(uint64_t) - 1];
	sp = (uint64_t *)((uintptr_t)sp & ~0xful);

	frame_zero(&t->frame);
	t->frame.sp = (uint64_t)(uintptr_t)sp;
	/* Keep per-CPU tp across preempt (K230 mhartid is not unique). */
	t->frame.tp = bootsys_cpuid();
	t->mepc = (uint64_t)(uintptr_t)thread_trampoline;

	s->ncreated++;
	return i;
}

static int pick_next(struct hart_sched *s, int after)
{
	int i;
	int start = after + 1;

	for (i = 0; i < THREAD_PER_HART; i++) {
		int idx = (start + i) % THREAD_PER_HART;

		if (s->threads[idx].state == THREAD_READY)
			return idx;
	}
	return -1;
}

static void save_from_tf(struct hart_sched *s, struct trap_frame *tf)
{
	if (s->current < 0) {
		s->idle_frame = *tf;
		s->idle_mepc = r_mepc();
		return;
	}
	s->threads[s->current].frame = *tf;
	s->threads[s->current].mepc = r_mepc();
	/* DONE stays DONE; only RUNNING becomes READY again. */
	if (s->threads[s->current].state == THREAD_RUNNING)
		s->threads[s->current].state = THREAD_READY;
}

void sched_mark_done(void)
{
	struct hart_sched *s = mine();

	if (s->current >= 0)
		s->threads[s->current].state = THREAD_DONE;
}

int sched_all_done(void)
{
	struct hart_sched *s = mine();
	int i;

	if (s->ncreated == 0)
		return 1;
	for (i = 0; i < THREAD_PER_HART; i++) {
		if (s->threads[i].state == THREAD_UNUSED)
			continue;
		if (s->threads[i].state != THREAD_DONE)
			return 0;
	}
	return 1;
}

void sched_stop(void)
{
	struct hart_sched *s = mine();
	unsigned hart = bootsys_cpuid();

	s->enabled = 0;
	clint_mtimecmp_write(hart, ~0ULL);
	clint_msip_set(hart, 0);
	__asm__ volatile("csrc mie, %0" ::"r"(MIE_MTIE | MIE_MSIE));
}

static void load_to_tf(struct hart_sched *s, int next, struct trap_frame *tf)
{
	if (next < 0) {
		*tf = s->idle_frame;
		w_mepc(s->idle_mepc);
		s->current = -1;
		return;
	}
	*tf = s->threads[next].frame;
	w_mepc(s->threads[next].mepc);
	s->threads[next].state = THREAD_RUNNING;
	s->current = next;
}

void sched_on_timer(struct trap_frame *tf)
{
	struct hart_sched *s = mine();
	unsigned hart = bootsys_cpuid();
	uint64_t now;
	int next;

	/* Shared print ring: only CPU0 may touch UART. */
	if (hart == 0)
		print_drain_isr();

	/* Keep cmp strictly in the future or MTIP may stick / go silent. */
	now = clint_mtime();
	s->cmp += TIMER_PERIOD;
	if (s->cmp <= now)
		s->cmp = now + TIMER_PERIOD;
	clint_mtimecmp_write(hart, s->cmp);
	s->ticks++;

	if (!s->enabled)
		return;

	save_from_tf(s, tf);
	next = pick_next(s, s->current);
	if (next < 0) {
		if (s->current != -1)
			s->switches++;
		load_to_tf(s, -1, tf);
		return;
	}
	if (next != s->current)
		s->switches++;
	load_to_tf(s, next, tf);
}

void sched_start(void)
{
	struct hart_sched *s = mine();
	unsigned hart = bootsys_cpuid();

	s->enabled = 1;
	s->current = -1;
	s->ticks = 0;
	s->switches = 0;
	s->cmp = clint_mtime() + TIMER_PERIOD;
	clint_mtimecmp_write(hart, s->cmp);
	clint_msip_set(hart, 0);
	csr_set_mie_mtie();
	csr_set_mie_msie();
	csr_set_mstatus_mie();
	/* Kick once so we leave idle even if MTIP is late. */
	clint_msip_set(hart, 1);
}

/* Cooperative reschedule (ecall → same path as timer). */
void sched_yield(void)
{
	__asm__ volatile("ecall" ::: "memory");
}

int sched_current_id(void)
{
	return mine()->current;
}

uint32_t sched_ticks(void)
{
	return mine()->ticks;
}

uint32_t sched_switches(void)
{
	return mine()->switches;
}
