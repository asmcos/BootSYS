#include "k230.h"
#include "mbox.h"

static void push_mbox(volatile struct mbox *m)
{
	thead_flush_range((uintptr_t)m, (uintptr_t)m + sizeof(*m));
}

static void pull_mbox(void)
{
	thead_flush_caches();
}

void mbox_init(void)
{
	volatile struct mbox_pair *p = mbox_pair();
	uint32_t i, j;

	p->c0_to_c1.w = 0;
	p->c0_to_c1.r = 0;
	p->c0_to_c1.drops = 0;
	p->c1_to_c0.w = 0;
	p->c1_to_c0.r = 0;
	p->c1_to_c0.drops = 0;
	for (i = 0; i < MBOX_CAP; i++) {
		p->c0_to_c1.slot[i].ready = 0;
		p->c0_to_c1.slot[i].seq = 0;
		p->c0_to_c1.slot[i].len = 0;
		p->c1_to_c0.slot[i].ready = 0;
		p->c1_to_c0.slot[i].seq = 0;
		p->c1_to_c0.slot[i].len = 0;
		for (j = 0; j < MBOX_PAYLOAD; j++) {
			p->c0_to_c1.slot[i].data[j] = 0;
			p->c1_to_c0.slot[i].data[j] = 0;
		}
	}
	fence_rw();
	thead_flush_range(MBOX_BASE, MBOX_BASE + sizeof(struct mbox_pair));
	thead_flush_caches();
}

int mbox_send(volatile struct mbox *m, const void *data, uint32_t len,
	      int do_cohere)
{
	uint32_t w, i, n;
	const char *src = (const char *)data;

	if (do_cohere)
		pull_mbox();
	w = m->w;
	if ((w - m->r) >= MBOX_CAP) {
		m->drops++;
		if (do_cohere) {
			fence_rw();
			push_mbox(m);
		}
		return -1;
	}
	if (len > MBOX_PAYLOAD)
		len = MBOX_PAYLOAD;
	i = w % MBOX_CAP;
	m->slot[i].ready = 0;
	m->slot[i].seq = w + 1u;
	m->slot[i].len = len;
	for (n = 0; n < len; n++)
		m->slot[i].data[n] = src[n];
	for (; n < MBOX_PAYLOAD; n++)
		m->slot[i].data[n] = 0;
	fence_rw();
	m->slot[i].ready = 1;
	fence_rw();
	m->w = w + 1u;
	fence_rw();
	if (do_cohere)
		push_mbox(m);
	return (int)len;
}

int mbox_recv(volatile struct mbox *m, void *data, uint32_t max,
	      int do_cohere)
{
	uint32_t r, i, n, len;
	char *dst = (char *)data;

	if (do_cohere)
		pull_mbox();
	r = m->r;
	if (r == m->w)
		return 0;
	i = r % MBOX_CAP;
	if (!m->slot[i].ready)
		return -1;
	len = m->slot[i].len;
	if (len > MBOX_PAYLOAD)
		len = MBOX_PAYLOAD;
	if (len > max)
		len = max;
	for (n = 0; n < len; n++)
		dst[n] = m->slot[i].data[n];
	m->slot[i].ready = 0;
	fence_rw();
	m->r = r + 1u;
	fence_rw();
	if (do_cohere)
		push_mbox(m);
	return (int)len;
}

uint32_t mbox_pending(volatile struct mbox *m, int do_cohere)
{
	if (do_cohere)
		pull_mbox();
	return m->w - m->r;
}
