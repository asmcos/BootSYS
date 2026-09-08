#ifndef CACHE_TAG_H
#define CACHE_TAG_H

#include <stdint.h>

/* K230 C908 L1: 32KiB, 4-way, 64B line → 128 sets. */
#define L1_WAYS  4
#define L1_SETS  128
#define L1_LINE  64

#define RID_ICACHE_TAG 0
#define RID_DCACHE_TAG 2

void cache_tag_init(void);
void test_dcache_touch_dump(void);
void test_icache_dump(void);
void test_dcache_flush_dump(void);
void test_dcache_scan_valid(void);

#endif
