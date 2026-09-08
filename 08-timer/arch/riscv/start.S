/* BootSYS 08-timer early boot (CPU0 only). */

.equ CSR_INVALID_DCACHE, 0x5c1

.section .init, "ax"
.globl _start
.align 2
_start:
	fence.i
	sfence.vma x0, x0
	csrrw x0, CSR_INVALID_DCACHE, x0

	li x1, 0
	li x2, 0
	li x3, 0
	li x4, 0
	li x5, 0
	li x6, 0
	li x7, 0
	li x8, 0
	li x9, 0
	li x10, 0
	li x11, 0
	li x12, 0
	li x13, 0
	li x14, 0
	li x15, 0
	li x16, 0
	li x17, 0
	li x18, 0
	li x19, 0
	li x20, 0
	li x21, 0
	li x22, 0
	li x23, 0
	li x24, 0
	li x25, 0
	li x26, 0
	li x27, 0
	li x28, 0
	li x29, 0
	li x30, 0
	li x31, 0

	la sp, _sp

	la a0, __data_load_start
	la a1, __data_start
	la a2, __data_end
	bgeu a1, a2, 1f
0:
	lw t0, 0(a0)
	sw t0, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	bltu a1, a2, 0b
1:
	la t0, _bss_start
	la t1, _bss_end
	bgeu t0, t1, 2f
0:
	sw zero, 0(t0)
	addi t0, t0, 4
	bltu t0, t1, 0b
2:
	call thead_cpu_init
	call main

3:
	wfi
	j 3b
