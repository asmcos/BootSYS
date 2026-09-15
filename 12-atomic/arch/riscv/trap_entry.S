/* Machine-mode trap — save GPRs; restore including sp from frame (for preempt). */

.section .text
.globl trap_entry
.align 2
trap_entry:
	addi	sp, sp, -272
	sd	ra,   0(sp)
	sd	t0,   8(sp)
	sd	t1,  16(sp)
	sd	t2,  24(sp)
	sd	s0,  32(sp)
	sd	s1,  40(sp)
	sd	a0,  48(sp)
	sd	a1,  56(sp)
	sd	a2,  64(sp)
	sd	a3,  72(sp)
	sd	a4,  80(sp)
	sd	a5,  88(sp)
	sd	a6,  96(sp)
	sd	a7, 104(sp)
	sd	s2, 112(sp)
	sd	s3, 120(sp)
	sd	s4, 128(sp)
	sd	s5, 136(sp)
	sd	s6, 144(sp)
	sd	s7, 152(sp)
	sd	s8, 160(sp)
	sd	s9, 168(sp)
	sd	s10,176(sp)
	sd	s11,184(sp)
	sd	t3, 192(sp)
	sd	t4, 200(sp)
	sd	t5, 208(sp)
	sd	t6, 216(sp)
	sd	gp, 224(sp)
	sd	tp, 232(sp)
	addi	t0, sp, 272
	sd	t0, 240(sp)

	mv	a0, sp
	call	trap_handler

	/* t6 = frame; load sp last so preempt can switch threads. */
	mv	t6, sp
	ld	ra,   0(t6)
	ld	t0,   8(t6)
	ld	t1,  16(t6)
	ld	t2,  24(t6)
	ld	s0,  32(t6)
	ld	s1,  40(t6)
	ld	a0,  48(t6)
	ld	a1,  56(t6)
	ld	a2,  64(t6)
	ld	a3,  72(t6)
	ld	a4,  80(t6)
	ld	a5,  88(t6)
	ld	a6,  96(t6)
	ld	a7, 104(t6)
	ld	s2, 112(t6)
	ld	s3, 120(t6)
	ld	s4, 128(t6)
	ld	s5, 136(t6)
	ld	s6, 144(t6)
	ld	s7, 152(t6)
	ld	s8, 160(t6)
	ld	s9, 168(t6)
	ld	s10,176(t6)
	ld	s11,184(t6)
	ld	t3, 192(t6)
	ld	t4, 200(t6)
	ld	t5, 208(t6)
	ld	tp, 232(t6)
	ld	gp, 224(t6)
	ld	sp, 240(t6)
	ld	t6, 216(t6)
	mret
