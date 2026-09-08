/* 特权级切换：mret 进入目标 shell（入口处使用干净栈）. */

.equ MSTATUS_MPP_MASK, (3 << 11)
.equ MSTATUS_MIE,      (1 << 3)

.section .text
.globl priv_enter
.globl trap_jump_shell

/*
 * a0 = entry PC
 * a1 = mstatus.MPP 字段（已移位）
 * 从 M 直接切换；不返回。
 */
priv_enter:
	la	sp, _sp
	j	.Ldo_mret

/*
 * 在 M-mode trap 里调用：抛弃当前 trap 栈帧，mret 进新 shell。
 * a0 = entry, a1 = MPP bits
 */
trap_jump_shell:
	la	sp, _sp
	j	.Ldo_mret

.Ldo_mret:
	/* keep satp: 06 walks the page table in S */
	csrr	t0, mstatus
	li	t1, ~MSTATUS_MPP_MASK
	and	t0, t0, t1
	or	t0, t0, a1
	li	t1, MSTATUS_MIE
	not	t1, t1
	and	t0, t0, t1
	csrw	mstatus, t0

	csrw	mepc, a0
	mret
