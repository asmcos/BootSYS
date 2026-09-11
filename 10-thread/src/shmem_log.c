#include "k230.h"
#include "shmem_log.h"

/*
 * K230 little/big cores are not coherent. thead_flush_range is dcache.cpa
 * (writeback only). Producers clean; consumers must invalidate or they keep
 * reading a stale L1 line (go=0 forever, or wipe remote w by cleaning 0).
 */
static void push_cmd(void)
{
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

static void push_log(void)
{
	thead_flush_range(SHMEM_LOG_BASE,
			  SHMEM_LOG_BASE + sizeof(struct shmem_log));
}

static void pull_shared(void)
{
	thead_flush_caches();
}

void shmem_log_reset(void)
{
	volatile struct shmem_log *l = shmem_log();
	uint32_t i, j;

	/* Do not store on ctrl line (CPU1 owns alive / cpu1_done). */
	l->w = 0;
	l->r = 0;
	for (i = 0; i < LOG_SLOTS; i++)
		for (j = 0; j < LOG_WIDTH; j++)
			l->slot[i][j] = 0;
	fence_rw();
	push_log();
	thead_flush_caches();
}

void shmem_log_putc_line(const char *s)
{
	volatile struct shmem_log *l = shmem_log();
	uint32_t w, i;

	pull_shared(); /* see CPU0's r */
	w = l->w;
	if ((w - l->r) >= LOG_SLOTS)
		return;
	for (i = 0; i < LOG_WIDTH - 1 && s[i]; i++)
		l->slot[w % LOG_SLOTS][i] = s[i];
	l->slot[w % LOG_SLOTS][i] = '\0';
	fence_rw();
	l->w = w + 1;
	fence_rw();
	push_log();
}

void shmem_log_drain(void)
{
	volatile struct shmem_log *l = shmem_log();
	uint32_t w, r, pending, n;

	pull_shared(); /* see CPU1's w / slots — never clean stale zeros first */
	w = l->w;
	r = l->r;
	pending = w - r;
	if (pending == 0 || pending > LOG_SLOTS)
		return;

	n = (pending > 2u) ? 2u : pending;
	while (n--) {
		__asm__ volatile("csrc mstatus, %0" ::"r"(MSTATUS_MIE));
		uart_puts((const char *)l->slot[r % LOG_SLOTS]);
		uart_puts("\r\n");
		__asm__ volatile("csrs mstatus, %0" ::"r"(MSTATUS_MIE));
		r++;
		l->r = r;
		fence_rw();
		push_log();
	}
}

void shmem_log_drain_all(void)
{
	uint32_t guard = LOG_SLOTS + 4u;

	while (guard--) {
		volatile struct shmem_log *l = shmem_log();
		uint32_t pending;

		pull_shared();
		pending = l->w - l->r;
		if (pending == 0 || pending > LOG_SLOTS)
			return;
		shmem_log_drain();
	}
}

void shmem_cmd_kick_mode(uint32_t loops, uint32_t mode)
{
	volatile struct shmem_cmd *c = shmem_cmd();

	/* CPU0 only stores on cmd line; cpu1_done is cleared by CPU1. */
	c->loops = loops;
	c->mode = mode;
	fence_rw();
	c->go = 1;
	fence_rw();
	push_cmd();
	thead_flush_caches();
}

void shmem_cmd_kick(uint32_t loops)
{
	shmem_cmd_kick_mode(loops, CMD_MODE_BATCH);
}

void shmem_cmd_clear_go(void)
{
	shmem_cmd()->go = 0;
	fence_rw();
	push_cmd();
}

int shmem_cpu1_batch_done(void)
{
	pull_shared();
	return shmem_ctrl()->cpu1_done != 0;
}

uint32_t shmem_cmd_loops(void)
{
	pull_shared();
	return shmem_cmd()->loops;
}

uint32_t shmem_cmd_mode(void)
{
	pull_shared();
	return shmem_cmd()->mode;
}

static void append_u32(char *buf, uint32_t *pos, uint32_t width, uint32_t v)
{
	char tmp[10];
	int n = 0;
	uint32_t x = v;

	if (x == 0) {
		tmp[n++] = '0';
	} else {
		while (x) {
			tmp[n++] = (char)('0' + (x % 10u));
			x /= 10u;
		}
	}
	while (n--) {
		if (*pos + 1 < width)
			buf[(*pos)++] = tmp[n];
	}
}

void cpu1_msg(const char *text)
{
	char buf[LOG_WIDTH];
	uint32_t p = 0;
	const char *s;
	static const char pref[] = "[cpu1] ";

	if (bootsys_cpuid() == 0)
		return;

	for (s = pref; *s && p + 1 < LOG_WIDTH; s++)
		buf[p++] = *s;
	if (text) {
		for (s = text; *s && p + 1 < LOG_WIDTH; s++)
			buf[p++] = *s;
	}
	buf[p] = '\0';
	shmem_log_putc_line(buf);
}

void cpu1_msg_worker(uint32_t tid, uint32_t n, uint32_t max, uint32_t tick)
{
	char buf[LOG_WIDTH];
	uint32_t p = 0;
	const char *s;
	static const char pref[] = "[cpu1] T";

	if (bootsys_cpuid() == 0)
		return;

	for (s = pref; *s && p + 1 < LOG_WIDTH; s++)
		buf[p++] = *s;
	append_u32(buf, &p, LOG_WIDTH, tid);
	if (p + 3 < LOG_WIDTH) {
		buf[p++] = ' ';
		buf[p++] = 'n';
		buf[p++] = '=';
	}
	append_u32(buf, &p, LOG_WIDTH, n);
	if (p + 1 < LOG_WIDTH)
		buf[p++] = '/';
	append_u32(buf, &p, LOG_WIDTH, max);
	if (p + 6 < LOG_WIDTH) {
		buf[p++] = ' ';
		buf[p++] = 't';
		buf[p++] = 'i';
		buf[p++] = 'c';
		buf[p++] = 'k';
		buf[p++] = '=';
	}
	append_u32(buf, &p, LOG_WIDTH, tick);
	buf[p] = '\0';
	shmem_log_putc_line(buf);
}
