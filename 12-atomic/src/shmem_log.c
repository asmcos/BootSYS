#include "k230.h"
#include "shmem_log.h"

static void push_cmd(void)
{
	thead_flush_range(SHMEM_CMD_BASE, SHMEM_CMD_BASE + 64);
}

void shmem_cmd_kick_mode(uint32_t loops, uint32_t mode)
{
	volatile struct shmem_cmd *c = shmem_cmd();

	c->loops = loops;
	c->mode = mode;
	c->poke = 0;
	c->start = 0;
	c->cpu0_ready = 0;
	c->add_done = 0;
	c->local_adds = 0;
	c->epoch = c->epoch + 1u;
	if (c->epoch == 0)
		c->epoch = 1;
	fence_rw();
	c->go = 1;
	fence_rw();
	push_cmd();
	thead_flush_caches();
}

void shmem_cmd_kick(uint32_t loops)
{
	shmem_cmd_kick_mode(loops, CMD_MODE_PLAIN);
}

void shmem_cmd_clear_go(void)
{
	shmem_cmd()->go = 0;
	shmem_cmd()->poke = 0;
	shmem_cmd()->start = 0;
	shmem_cmd()->cpu0_ready = 0;
	fence_rw();
	push_cmd();
}

int shmem_cpu1_batch_done(void)
{
	thead_flush_caches();
	return shmem_ctrl()->cpu1_done != 0;
}

uint32_t shmem_cmd_loops(void)
{
	thead_flush_caches();
	return shmem_cmd()->loops;
}

uint32_t shmem_cmd_mode(void)
{
	thead_flush_caches();
	return shmem_cmd()->mode;
}
