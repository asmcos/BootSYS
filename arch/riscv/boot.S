/* BootSYS RISC-V common early boot (arch/riscv) — adapted from riscvboot */

.equ CSR_INVALID_DCACHE, 0x5c1
.equ CSR_MHARTID, 0xf14
.equ REGBYTES, 8

.macro SREG reg, offset, base
	sd \reg, \offset(\base)
.endm

.macro LREG reg, offset, base
	ld \reg, \offset(\base)
.endm

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

	la t0, trap_entry
	csrw mtvec, t0
	csrw mie, zero

	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	la sp, _sp

load_data:
	la a0, __data_load_start
	la a1, __data_start
	la a2, __data_end
	bgeu a1, a2, clear_bss
load_data_loop:
	lw t0, 0(a0)
	sw t0, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	bltu a1, a2, load_data_loop

clear_bss:
	la t0, _bss_start
	la t1, _bss_end
	bgeu t0, t1, call_rust
clear_bss_loop:
	sw zero, 0(t0)
	addi t0, t0, 4
	bltu t0, t1, clear_bss_loop

call_rust:
	li a0, 0
	li a1, 0
	call rust_main

1:
	wfi
	j 1b

.align 2
trap_entry:
	addi sp, sp, -35 * REGBYTES
	SREG x1, 0 * REGBYTES, sp
	SREG x2, 1 * REGBYTES, sp
	SREG x3, 2 * REGBYTES, sp
	SREG x4, 3 * REGBYTES, sp
	SREG x5, 4 * REGBYTES, sp
	SREG x6, 5 * REGBYTES, sp
	SREG x7, 6 * REGBYTES, sp
	SREG x8, 7 * REGBYTES, sp
	SREG x9, 8 * REGBYTES, sp
	SREG x10, 9 * REGBYTES, sp
	SREG x11, 10 * REGBYTES, sp
	SREG x12, 11 * REGBYTES, sp
	SREG x13, 12 * REGBYTES, sp
	SREG x14, 13 * REGBYTES, sp
	SREG x15, 14 * REGBYTES, sp
	SREG x16, 15 * REGBYTES, sp
	SREG x17, 16 * REGBYTES, sp
	SREG x18, 17 * REGBYTES, sp
	SREG x19, 18 * REGBYTES, sp
	SREG x20, 19 * REGBYTES, sp
	SREG x21, 20 * REGBYTES, sp
	SREG x22, 21 * REGBYTES, sp
	SREG x23, 22 * REGBYTES, sp
	SREG x24, 23 * REGBYTES, sp
	SREG x25, 24 * REGBYTES, sp
	SREG x26, 25 * REGBYTES, sp
	SREG x27, 26 * REGBYTES, sp
	SREG x28, 27 * REGBYTES, sp
	SREG x29, 28 * REGBYTES, sp
	SREG x30, 29 * REGBYTES, sp
	SREG x31, 30 * REGBYTES, sp

	csrr a0, mepc
	SREG a0, 31 * REGBYTES, sp
	csrr a0, mstatus
	SREG a0, 32 * REGBYTES, sp
	csrr a0, mtval
	SREG a0, 33 * REGBYTES, sp
	csrr a0, mcause
	SREG a0, 34 * REGBYTES, sp

	mv a0, sp
	call handle_trap

	LREG a0, 31 * REGBYTES, sp
	csrw mepc, a0
	LREG a0, 32 * REGBYTES, sp
	csrw mstatus, a0

	LREG x1, 0 * REGBYTES, sp
	LREG x2, 1 * REGBYTES, sp
	LREG x3, 2 * REGBYTES, sp
	LREG x4, 3 * REGBYTES, sp
	LREG x5, 4 * REGBYTES, sp
	LREG x6, 5 * REGBYTES, sp
	LREG x7, 6 * REGBYTES, sp
	LREG x8, 7 * REGBYTES, sp
	LREG x9, 8 * REGBYTES, sp
	LREG x10, 9 * REGBYTES, sp
	LREG x11, 10 * REGBYTES, sp
	LREG x12, 11 * REGBYTES, sp
	LREG x13, 12 * REGBYTES, sp
	LREG x14, 13 * REGBYTES, sp
	LREG x15, 14 * REGBYTES, sp
	LREG x16, 15 * REGBYTES, sp
	LREG x17, 16 * REGBYTES, sp
	LREG x18, 17 * REGBYTES, sp
	LREG x19, 18 * REGBYTES, sp
	LREG x20, 19 * REGBYTES, sp
	LREG x21, 20 * REGBYTES, sp
	LREG x22, 21 * REGBYTES, sp
	LREG x23, 22 * REGBYTES, sp
	LREG x24, 23 * REGBYTES, sp
	LREG x25, 24 * REGBYTES, sp
	LREG x26, 25 * REGBYTES, sp
	LREG x27, 26 * REGBYTES, sp
	LREG x28, 27 * REGBYTES, sp
	LREG x29, 28 * REGBYTES, sp
	LREG x30, 29 * REGBYTES, sp
	LREG x31, 30 * REGBYTES, sp

	addi sp, sp, 35 * REGBYTES
	mret
