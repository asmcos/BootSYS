#include "k230.h"
#include "memmap.h"

void thead_flush_range(uintptr_t start, uintptr_t end);

void flush_image_cache(void)
{
	thead_flush_range(LOAD_BASE, LOAD_BASE + IMAGE_SIZE);
	thead_flush_caches();
}
