#ifndef MEMMAP_H
#define MEMMAP_H

/*
 * SRAM (default): J-Link / BOOTROM path, same as BootSYS @ 0x80200000.
 * DDR:           use after U-Boot has initialized DRAM (or your own DDR init).
 */
#if defined(LOAD_DDR)
#define LOAD_BASE   0x50000000UL
#define SHMEM_BASE  0x50200000UL
#else
#define LOAD_BASE   0x80200000UL
#define SHMEM_BASE  0x80220000UL
#endif

#define IMAGE_SIZE  0x40000UL

#endif
