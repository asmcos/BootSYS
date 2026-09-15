#include "k230.h"
#include "cache_probe.h"
#include "print.h"
#include "shmem_log.h"
#include "worker.h"

static void push_data(void)
{
	thead_flush_range(CACHE_PROBE_DATA, CACHE_PROBE_DATA + 64);
}

static void push_sync(void)
{
	fence_rw();
	thead_flush_range(CACHE_PROBE_SYNC, CACHE_PROBE_SYNC + 64);
}

static void pull_sync(void)
{
	fence_rw();
	thead_invalidate_range(CACHE_PROBE_SYNC, CACHE_PROBE_SYNC + 64);
}

static void pull_data(void)
{
	fence_rw();
	thead_invalidate_range(CACHE_PROBE_DATA, CACHE_PROBE_DATA + 64);
}

static int wait_ack(uint32_t want, uint32_t spins_max)
{
	uint32_t i;

	for (i = 0; i < spins_max; i++) {
		pull_sync();
		if (cache_probe_sync()->c1_ack == want)
			return 1;
		if ((i & 0xffffu) == 0) {
			clint_msip_set(1, 1);
			print_drain_all();
		}
		worker_delay(50u);
	}
	return 0;
}

static int wait_phase(uint32_t want, uint32_t spins_max)
{
	uint32_t i;

	for (i = 0; i < spins_max; i++) {
		pull_sync();
		if (cache_probe_sync()->phase == want)
			return 1;
		worker_delay(50u);
	}
	return 0;
}

void cache_probe_run_cpu0(void)
{
	volatile uint32_t *word = cache_probe_word();
	volatile struct cache_probe_sync *s = cache_probe_sync();
	uint32_t c0_a, c0_b, c1_a, c1_raw, c1_pull;

	/*
	 * Plain story:
	 *   var @ addr
	 *   both read A
	 *   cpu0 writes B (no flush)
	 *   cpu1 read again — see B?  (HW coherent?)
	 *   flush+pull — see B?      (SW cohere)
	 */
	print_printf(">>> cache probe");
	print_printf("var  @ 0x%x", (uint32_t)CACHE_PROBE_DATA);
	print_drain_all();

	s->phase = CP_PHASE_IDLE;
	s->c1_ack = 0;
	s->c1_r1 = s->c1_r2 = s->c1_r3 = 0;
	s->c0_r_after_b = 0;
	*word = CACHE_PROBE_MAGIC_A;
	fence_rw();
	push_data();
	c0_a = *word;
	s->phase = CP_PHASE_HAVE_A;
	push_sync();

	if (!wait_ack(1, 2000000u)) {
		thead_flush_caches();
		print_printf("FAIL: cpu1 no reply (alive=%u go=%u)",
			     shmem_ctrl()->alive, shmem_cmd()->go);
		print_drain_all();
		return;
	}
	pull_sync();
	c1_a = s->c1_r1;
	print_printf("1) both read A=0x%x", CACHE_PROBE_MAGIC_A);
	print_printf("   cpu0=0x%x  cpu1=0x%x", c0_a, c1_a);
	print_drain_all();

	*word = CACHE_PROBE_MAGIC_B;
	fence_rw();
	c0_b = *word;
	s->c0_r_after_b = c0_b;
	s->phase = CP_PHASE_WROTE_B;
	push_sync();

	if (!wait_ack(2, 2000000u)) {
		print_printf("FAIL: cpu1 no raw read");
		print_drain_all();
		return;
	}
	pull_sync();
	c1_raw = s->c1_r2;
	print_printf("2) cpu0 write B=0x%x  (no flush)", CACHE_PROBE_MAGIC_B);
	print_printf("   cpu0=0x%x  cpu1=0x%x  (no pull)", c0_b, c1_raw);
	if (c1_raw == CACHE_PROBE_MAGIC_B)
		print_printf("   => cpu1 SAW it  (HW coherent)");
	else
		print_printf("   => cpu1 NOT see (HW not coherent)");
	print_drain_all();

	push_data();
	s->phase = CP_PHASE_FLUSHED_B;
	push_sync();

	if (!wait_ack(3, 2000000u)) {
		print_printf("FAIL: cpu1 no pull read");
		print_drain_all();
		return;
	}
	pull_sync();
	c1_pull = s->c1_r3;
	print_printf("3) cpu0 flush + cpu1 pull");
	print_printf("   cpu1=0x%x", c1_pull);
	if (c1_pull == CACHE_PROBE_MAGIC_B)
		print_printf("   => cpu1 SAW it  (SW flush/pull OK)");
	else
		print_printf("   => still wrong");
	print_drain_all();
}

void cache_probe_run_cpu1(void)
{
	volatile uint32_t *word = cache_probe_word();
	volatile struct cache_probe_sync *s = cache_probe_sync();
	uint32_t v;

	/* Quiet — CPU0 prints the summary from sync fields. */

	if (!wait_phase(CP_PHASE_HAVE_A, 2000000u))
		return;
	pull_data();
	v = *word;
	s->c1_r1 = v;
	s->c1_ack = 1;
	push_sync();

	if (!wait_phase(CP_PHASE_WROTE_B, 2000000u))
		return;
	/* No invalidate: keep stale L1 if HW does not snoop. */
	v = *word;
	s->c1_r2 = v;
	s->c1_ack = 2;
	push_sync();

	if (!wait_phase(CP_PHASE_FLUSHED_B, 2000000u))
		return;
	pull_data();
	v = *word;
	s->c1_r3 = v;
	s->c1_ack = 3;
	push_sync();
}
