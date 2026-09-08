#ifndef PRIV_H
#define PRIV_H

#include <stdint.h>

enum priv_mode {
	PRIV_M = 0,
	PRIV_S = 1,
	PRIV_U = 2,
};

enum pmp_policy {
	PMP_ON = 0,  /* secret denied to S/U */
	PMP_OFF = 1, /* allow-all (S/U can use the region) */
};

#define ECALL_GOTO_M   1
#define ECALL_GOTO_S   2
#define ECALL_GOTO_U   3
#define ECALL_ENTER_M  5 /* trap in M, mret back as M (raise U/S) */

void priv_force_m(void);

extern volatile enum priv_mode g_cur_mode;
extern volatile enum pmp_policy g_pmp_policy;

const char *priv_mode_name(enum priv_mode m);
const char *pmp_policy_name(enum pmp_policy p);

void priv_enter(void (*entry)(void), unsigned long mpp_bits);

#define MSTATUS_MPP_M  (3UL << 11)
#define MSTATUS_MPP_S  (1UL << 11)
#define MSTATUS_MPP_U  (0UL << 11)

void shell_m(void);
void shell_s(void);
void shell_u(void);

void priv_request_mode(int ecall_code);

int pmp_enable(void);
int pmp_clear(void);

void test_region_read(void);
void test_region_write(void);

extern char _pmp_secret[];

#endif
