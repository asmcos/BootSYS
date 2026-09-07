#ifndef PRIV_H
#define PRIV_H

#include <stdint.h>

enum priv_mode {
	PRIV_M = 0,
	PRIV_S = 1,
	PRIV_U = 2,
};

/* ecall a0：切模式 / 仅测试 ecall */
#define ECALL_GOTO_M  1
#define ECALL_GOTO_S  2
#define ECALL_GOTO_U  3
#define ECALL_TEST    4

extern volatile enum priv_mode g_cur_mode;

const char *priv_mode_name(enum priv_mode m);

/* 从 M 直接 mret 进入目标 shell（不返回） */
void priv_enter(void (*entry)(void), unsigned long mpp_bits);

#define MSTATUS_MPP_M  (3UL << 11)
#define MSTATUS_MPP_S  (1UL << 11)
#define MSTATUS_MPP_U  (0UL << 11)

void shell_m(void);
void shell_s(void);
void shell_u(void);

/* 在当前模式试读 CSR；异常由 trap 打印后回到同一特权级 */
void test_csrr_mhartid(void);
void test_csrr_sstatus(void);
void test_csrr_mstatus(void);
void test_ecall(void);
/* Prove current privilege via ecall cause + mstatus.MPP seen in M trap. */
void prove_current_mode(void);

/* Last trap snapshot (filled when trap_expect). */
extern volatile uint64_t trap_last_mpp; /* mstatus.MPP at trap: 3=M,1=S,0=U */

/* S/U 里请求切模式（ecall → M 处理） */
void priv_request_mode(int ecall_code);

/*
 * C908 实现了 PMP：未配置时 S/U 默认禁止一切物理访问。
 * 进入 S/U 前必须放开（与加载地址 0x80200000 无关）。
 */
void pmp_allow_all(void);

#endif
