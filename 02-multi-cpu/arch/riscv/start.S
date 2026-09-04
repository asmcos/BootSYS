/* BootSYS K230 early boot (C firmware). */

.equ CSR_INVALID_DCACHE, 0x5c1
.equ UART0,       0x91400000
.equ UART_LSR,    0x14
.equ UART_THR,    0x00
.equ LSR_THRE,    0x20
.equ SHMEM_BASE,  0x80220000
.equ ALIVE_MAGIC, 0xC001F00D

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

.globl _hart1_entry
.align 2
_hart1_entry:
	/* Enable cache/SMP first, then publish alive (CPU0 can see it). */
	call thead_cpu_init

	fence.i
	sfence.vma x0, x0
	csrrw x0, CSR_INVALID_DCACHE, x0

	li	t0, SHMEM_BASE
	li	t1, ALIVE_MAGIC
	sw	t1, 0(t0)
	fence rw, rw

	/* No UART here — avoids racing CPU0 boot prints. */

	la sp, _sp1
	call hart1_main

4:
	wfi
	j 4b
