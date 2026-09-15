#include "k230.h"
#include "thread.h"
#include "worker.h"
#include "shmem_log.h"
#include "print.h"
#include "mbox.h"

static uint32_t g_loops = WORKER_LOOPS_DEFAULT;

void worker_set_loops(uint32_t n)
{
	if (n == 0)
		n = WORKER_LOOPS_DEFAULT;
	g_loops = n;
}

uint32_t worker_get_loops(void)
{
	return g_loops;
}

void worker_delay(uint32_t loops)
{
	volatile uint32_t n = loops;

	while (n--)
		__asm__ volatile("" ::: "memory");
}

static void yield_wait(uint64_t due)
{
	unsigned cpu = bootsys_cpuid();

	while (clint_mtime() < due) {
		if (cpu == 0)
			__asm__ volatile("wfi");
		else
			__asm__ volatile("ecall" ::: "memory");
	}
}

void worker_sender(void *arg)
{
	int cohere = (int)(uintptr_t)arg;
	uint32_t max = g_loops;
	uint32_t n;
	volatile struct mbox *m = &mbox_pair()->c0_to_c1;
	uint64_t t0 = clint_mtime();

	if (max == 0)
		max = WORKER_LOOPS_DEFAULT;

	for (n = 1; n <= max; n++) {
		char buf[MBOX_PAYLOAD];
		uint32_t p = 0;
		const char *pref = "ping-";
		uint32_t x = n;
		char tmp[10];
		int nd = 0;

		while (*pref && p + 1 < MBOX_PAYLOAD)
			buf[p++] = *pref++;
		if (x == 0)
			tmp[nd++] = '0';
		else {
			while (x) {
				tmp[nd++] = (char)('0' + (x % 10u));
				x /= 10u;
			}
		}
		while (nd--)
			if (p + 1 < MBOX_PAYLOAD)
				buf[p++] = tmp[nd];
		buf[p] = '\0';

		yield_wait(t0 + (uint64_t)(n - 1u) * WORKER_PERIOD_MTIME);

		if (mbox_send(m, buf, p, cohere) < 0)
			print_printf("[cpu0] send FULL drop n=%u", n);
		else
			print_printf("[cpu0] send %s cohere=%d", buf, cohere);
	}
}

void worker_receiver(void *arg)
{
	int cohere = (int)(uintptr_t)arg;
	uint32_t expect = shmem_cmd_loops();
	uint32_t got = 0;
	uint32_t spins = 0;
	volatile struct mbox *m = &mbox_pair()->c0_to_c1;
	volatile struct shmem_ctrl *c = shmem_ctrl();

	if (expect == 0)
		expect = WORKER_LOOPS_DEFAULT;
	c->rx_ok = 0;
	c->rx_bad = 0;
	fence_rw();
	thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);

	while (got < expect && spins < 5000000u) {
		char buf[MBOX_PAYLOAD + 1];
		int n;

		spins++;
		n = mbox_recv(m, buf, MBOX_PAYLOAD, cohere);
		if (n > 0) {
			buf[n] = '\0';
			got++;
			c->rx_ok = got;
			fence_rw();
			thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
			print_printf("[cpu1] recv %s", buf);
		} else if (n < 0) {
			c->rx_bad = c->rx_bad + 1;
			fence_rw();
			thead_flush_range(SHMEM_BASE, SHMEM_BASE + 64);
			__asm__ volatile("ecall" ::: "memory");
		} else {
			__asm__ volatile("ecall" ::: "memory");
		}
	}
	print_printf("[cpu1] recv done ok=%u bad=%u", c->rx_ok, c->rx_bad);
}

void worker_ping_c0(void *arg)
{
	uint32_t max = g_loops;
	uint32_t n;
	volatile struct mbox *tx = &mbox_pair()->c0_to_c1;
	volatile struct mbox *rx = &mbox_pair()->c1_to_c0;
	(void)arg;

	if (max == 0)
		max = WORKER_LOOPS_DEFAULT;

	for (n = 1; n <= max; n++) {
		char buf[MBOX_PAYLOAD];
		char reply[MBOX_PAYLOAD + 1];
		uint32_t spins = 0;
		int rn;

		buf[0] = 'Q';
		buf[1] = (char)('0' + (n % 10u));
		buf[2] = '\0';
		mbox_send(tx, buf, 2, 1);
		print_printf("[cpu0] ping %s", buf);

		for (;;) {
			rn = mbox_recv(rx, reply, MBOX_PAYLOAD, 1);
			if (rn > 0) {
				reply[rn] = '\0';
				print_printf("[cpu0] got %s", reply);
				break;
			}
			if (++spins > 2000000u) {
				print_printf("[cpu0] ping timeout");
				break;
			}
			__asm__ volatile("wfi");
		}
	}
}

void worker_pong_c1(void *arg)
{
	uint32_t expect = shmem_cmd_loops();
	uint32_t got = 0;
	uint32_t spins = 0;
	volatile struct mbox *rx = &mbox_pair()->c0_to_c1;
	volatile struct mbox *tx = &mbox_pair()->c1_to_c0;
	(void)arg;

	if (expect == 0)
		expect = WORKER_LOOPS_DEFAULT;

	while (got < expect && spins < 5000000u) {
		char buf[MBOX_PAYLOAD + 1];
		char out[MBOX_PAYLOAD];
		int n;

		spins++;
		n = mbox_recv(rx, buf, MBOX_PAYLOAD, 1);
		if (n <= 0) {
			__asm__ volatile("ecall" ::: "memory");
			continue;
		}
		buf[n] = '\0';
		got++;
		out[0] = 'A';
		out[1] = buf[1];
		out[2] = '\0';
		mbox_send(tx, out, 2, 1);
		print_printf("[cpu1] pong %s", out);
	}
}
