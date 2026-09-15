/*
 * 12-atomic — atomics on CPU0 with two preemptible threads (10-style).
 *
 * Menu t/l/a: same hart, same L1 — D-cache ON; amoswap / amoadd work.
 * Menu r/p still use CPU1 (reservation / print probe).
 */
#include "k230.h"
#include "memmap.h"
#include "trap.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "atomic.h"

static int cpu1_booted;

#ifndef BOOTSYS_GIT
#define BOOTSYS_GIT "unknown"
#endif

static void banner(void)
{
	/* Direct UART — print ring truncates UTF-8 box drawing. */
	uart_puts("\r\n");
	uart_puts("╭──────────────────────────────────────────────╮\r\n");
	uart_puts("│  BootSYS  ·  12-atomic                       │\r\n");
	uart_puts("│  CPU0 × 2 threads: plain / lock / amoadd     │\r\n");
	uart_puts("├──────────────────────────────────────────────┤\r\n");
	uart_puts("│  build   " __DATE__ " " __TIME__ "                │\r\n");
	uart_puts("│  git     " BOOTSYS_GIT "                             │\r\n");
	uart_puts("╰──────────────────────────────────────────────╯\r\n");
	uart_puts("\r\n");
}

static void print_help(void)
{
	print_printf("======= menu =======");
	print_printf("  t  plain ld/add/sd (2 threads, expect lost)");
	print_printf("  l  amoswap lock + add (expect 2N)");
	print_printf("  a  amoadd.d (expect 2N)");
	print_printf("  r  other hart store between lr/sc (sc fail)");
	print_printf("  p  print probe (CPU1 → print ring)");
	print_printf("  n  set N (loops per thread, now %u)", worker_get_loops());
	print_printf("  s  status");
	print_printf("  h  help");
	print_printf("CPU0 only: 2 threads × N; expect shared == 2N for l/a");
	print_printf("====================");
	print_drain_all();
}

static uint32_t read_uint_line(void)
{
	uint32_t v = 0;
	int got = 0;

	for (;;) {
		int c;

		do {
			c = uart_try_getc();
		} while (c < 0);
		if (c == '\r' || c == '\n') {
			uart_puts("\r\n");
			break;
		}
		if (c == 0x08 || c == 0x7f) {
			if (got) {
				v /= 10u;
				got--;
				uart_puts("\b \b");
			}
			continue;
		}
		if (c >= '0' && c <= '9') {
			uint32_t d = (uint32_t)(c - '0');

			if (v > 100000000u)
				continue;
			v = v * 10u + d;
			got++;
			uart_putc((char)c);
		}
	}
	return got ? v : 0;
}

static void set_loops_menu(void)
{
	uint32_t v;

	print_printf("loops (Enter keeps %u; e.g. 100 / 2000 / 20000):",
		     worker_get_loops());
	print_drain_all();
	v = read_uint_line();
	if (v == 0) {
		print_printf("keep N=%u", worker_get_loops());
		print_drain_all();
		return;
	}
	worker_set_loops(v);
	print_printf("N=%u each thread, expect 2N=%u for l/a",
		     worker_get_loops(), worker_get_loops() * 2u);
	print_drain_all();
}

static void ensure_cpu1(void)
{
	uint32_t entry = (uint32_t)(uintptr_t)_hart1_entry;
	struct cpu1_boot_result br;
	uint32_t i;

	if (cpu1_booted)
		return;

	flush_image_cache();
	br = cpu1_boot(entry);
	if (!br.assert_ok)
		print_printf("cpu1: HW FAIL assert");
	else if (!br.released)
		print_printf("cpu1: HW FAIL release");
	print_drain_all();
	cpu1_dump_status("boot");

	for (i = 0; i < 2000u; i++) {
		thead_flush_caches();
		if (shmem_ctrl()->alive == CPU1_ALIVE_MAGIC)
			break;
		udelay(1000);
	}
	thead_flush_caches();
	print_printf("cpu1 alive=%s cpuid=%u",
		     shmem_ctrl()->alive == CPU1_ALIVE_MAGIC ? "yes" : "no",
		     shmem_ctrl()->cpuid);
	print_drain_all();
	cpu1_booted = 1;
}

static void wait_cpu1_and_drain(uint32_t seen_before)
{
	uint32_t spins = 0;

	for (;;) {
		thead_flush_caches();
		print_drain_all();
		clint_msip_set(1, 1);
		if (shmem_ctrl()->go_seen != seen_before &&
		    shmem_ctrl()->cpu1_done)
			break;
		if (++spins >= 5000000u) {
			print_printf("[cpu0] timeout go=%u done=%u seen=%u/%u",
				     shmem_cmd()->go, shmem_ctrl()->cpu1_done,
				     shmem_ctrl()->go_seen, seen_before);
			print_drain_all();
			shmem_cmd_clear_go();
			break;
		}
		worker_delay(1000u);
	}
	print_drain_all();
}

static const char *mode_name(uint32_t mode)
{
	switch (mode) {
	case CMD_MODE_PLAIN:
		return "plain ld/add/sd";
	case CMD_MODE_LOCK:
		return "amoswap lock+add";
	case CMD_MODE_AMO:
		return "amoadd.d";
	default:
		return "?";
	}
}

/* CPU0 × 2 threads (10-style). No CPU1, no D$ off. */
static void run_two_threads(uint32_t mode)
{
	uint32_t n = worker_get_loops();
	uint64_t expect = 2ULL * n;
	uint64_t got;
	uint32_t spins = 0;
	uint32_t last0 = 0;
	uint32_t last1 = 0;

	print_printf(">>> %s N=%u (CPU0 × 2 threads)", mode_name(mode), n);
	print_drain_all();

	print_init();
	atom_reset();
	atom_rng_seed((uint32_t)clint_mtime() ^ (n * 0x9E3779B9u));
	worker_set_race_mode(mode);

	sched_init_hart();
	sched_create(worker_add, (void *)(uintptr_t)0);
	sched_create(worker_add, (void *)(uintptr_t)1);
	sched_start();

	while (!sched_all_done()) {
		uint32_t t0 = worker_local_adds(0);
		uint32_t t1 = worker_local_adds(1);

		print_drain_all();
		if (t0 != last0 || t1 != last1) {
			if ((t0 / 100u) != (last0 / 100u) ||
			    (t1 / 100u) != (last1 / 100u) ||
			    t0 == n || t1 == n) {
				print_printf("  progress T0=%u T1=%u /%u ctr=%u sw=%u",
					     t0, t1, n, (uint32_t)atom()->counter,
					     sched_switches());
			}
			last0 = t0;
			last1 = t1;
		}
		if (++spins >= 8000000u) {
			print_printf("[cpu0] timeout T0=%u T1=%u sw=%u",
				     t0, t1, sched_switches());
			print_drain_all();
			break;
		}
		__asm__ volatile("wfi");
	}
	sched_stop();
	print_drain_all();

	got = atom()->counter;
	print_printf("  ---- per-thread ----");
	print_printf("  T0 local_adds=%u  (want N=%u)", worker_local_adds(0), n);
	print_printf("  T1 local_adds=%u  (want N=%u)", worker_local_adds(1), n);
	print_printf("  ---- shared --------");
	print_printf("  counter=%u  expect=2N=%u  lost=%u",
		     (uint32_t)got, (uint32_t)expect,
		     got <= expect ? (uint32_t)(expect - got) : 0u);
	print_printf("  switches=%u  timer_ticks=%u",
		     sched_switches(), sched_ticks());

	if (worker_local_adds(0) != n || worker_local_adds(1) != n) {
		print_printf("  verdict: FAIL (a thread did not finish N)");
		print_drain_all();
		return;
	}
	if (mode == CMD_MODE_PLAIN)
		print_printf(got < expect ? "  verdict: PASS (lost updates)"
					  : "  verdict: no loss this run (try again)");
	else
		print_printf(got == expect ? "  verdict: PASS"
					   : "  verdict: FAIL");
	print_drain_all();
}

static void run_reservation(void)
{
	int rc;

	print_printf(">>> reservation: CPU0 lr, CPU1 sd, CPU0 sc");
	print_drain_all();
	ensure_cpu1();
	print_init();

	rc = atom_sc_fails_after_foreign_store();
	print_drain_all();

	if (rc < 0)
		print_printf("  timeout waiting cpu1  FAIL");
	else if (rc)
		print_printf("  sc failed  PASS (other hart broke reservation)");
	else
		print_printf("  sc succeeded  FAIL (expected fail)");
	print_drain_all();
}

static void test_print_probe(void)
{
	uint32_t n = worker_get_loops();
	uint32_t seen;

	if (n > 20u)
		n = 5u;
	print_printf(">>> print probe N=%u", n);
	print_drain_all();
	ensure_cpu1();
	print_init();
	thead_flush_caches();
	seen = shmem_ctrl()->go_seen;
	shmem_cmd_kick_mode(n, CMD_MODE_PROBE);
	wait_cpu1_and_drain(seen);
	print_printf("[cpu0] probe done");
	print_drain_all();
}

int main(void)
{
	sysctl_early_init();
	uart_init();
	trap_init();
	print_init();
	atom_reset();
	sched_init_hart();
	worker_set_loops(WORKER_LOOPS_DEFAULT);
	shmem_cmd_clear_go();

	uart_puts("\x1b[r\x1b[0m\x1b[2J\x1b[H");
	banner();
	print_help();

	for (;;) {
		int c;

		print_drain_all();
		uart_puts("\r\n[M] > ");
		do {
			c = uart_try_getc();
		} while (c < 0);
		uart_putc((char)c);
		uart_puts("\r\n");

		switch (c) {
		case 't':
		case 'T':
			run_two_threads(CMD_MODE_PLAIN);
			break;
		case 'l':
		case 'L':
			run_two_threads(CMD_MODE_LOCK);
			break;
		case 'a':
		case 'A':
			run_two_threads(CMD_MODE_AMO);
			break;
		case 'r':
		case 'R':
			run_reservation();
			break;
		case 'p':
		case 'P':
			test_print_probe();
			break;
		case 'n':
		case 'N':
			set_loops_menu();
			break;
		case 's':
		case 'S':
			thead_flush_caches();
			print_printf("N=%u ticks=%u cpuid=%u pend=%u",
				     worker_get_loops(), sched_ticks(),
				     bootsys_cpuid(), print_pending());
			print_printf("cpu1 done=%u seen=%u armed=%u go=%u",
				     shmem_ctrl()->cpu1_done, shmem_ctrl()->go_seen,
				     shmem_ctrl()->atom_armed, shmem_cmd()->go);
			print_drain_all();
			break;
		case 'h':
		case 'H':
		case '?':
			print_help();
			break;
		default:
			print_printf("unknown, press h");
			print_drain_all();
			break;
		}
	}
}
