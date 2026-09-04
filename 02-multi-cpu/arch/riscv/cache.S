/* T-Head C908 — match examples/k230-minimal (U-Boot improving_cpu_performance). */

.equ CSR_INVALID_DCACHE, 0x5c1

.section .text
.globl thead_flush_caches
.type thead_flush_caches, @function
thead_flush_caches:
	.long	0x0ff0000f
	.long	0x0030000b
	.long	0x0000100f
	.long	0x0220000f
	.long	0x0100000b
	fence.i
	sfence.vma x0, x0
	csrrw	x0, CSR_INVALID_DCACHE, x0
	.long	0x0000100f
	.long	0x0220000f
	ret
.size thead_flush_caches, . - thead_flush_caches

.globl thead_flush_range
.type thead_flush_range, @function
thead_flush_range:
	addi	a1, a1, 63
	andi	a0, a0, -64
	andi	a1, a1, -64
	beq	a0, a1, 2f
1:
	.long	0x0295000b
	addi	a0, a0, 64
	bltu	a0, a1, 1b
2:
	fence.i
	ret
.size thead_flush_range, . - thead_flush_range

.globl thead_cpu_init
.type thead_cpu_init, @function
thead_cpu_init:
	li	t0, 0x70013
	csrw	0x7c2, t0
	li	t0, 0xe0000009
	csrw	0x7c3, t0
	li	t0, 0x11ff
	csrw	0x7c1, t0
	li	t0, 0x638000
	csrw	0x7c0, t0
	li	t0, 0x6e30c
	csrw	0x7c5, t0
	li	t0, 1
	csrw	0x7f3, t0
	ret
.size thead_cpu_init, . - thead_cpu_init

/* Optional: disable L1/SMPEN before returning to J-Link (menu 'q'). */
.globl thead_cache_off
.type thead_cache_off, @function
thead_cache_off:
	csrw	0x7c1, zero
	csrw	0x7f3, zero
	.long	0x0030000b
	.long	0x0100000b
	fence.i
	sfence.vma x0, x0
	ret
.size thead_cache_off, . - thead_cache_off
