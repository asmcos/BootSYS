#ifndef PRIV_H
#define PRIV_H

#include <stdint.h>

enum priv_mode {
	PRIV_M = 0,
	PRIV_S = 1,
	PRIV_U = 2,
};

#define ECALL_GOTO_M   1
#define ECALL_GOTO_S   2
#define ECALL_GOTO_U   3
#define ECALL_ENTER_M  5
#define ECALL_MMU_OFF  6

#define MSTATUS_MPP_M  (3UL << 11)
#define MSTATUS_MPP_S  (1UL << 11)
#define MSTATUS_MPP_U  (0UL << 11)

/* Probe VA: 4K, not identity. PA is tlb_page[]. */
#define TLB_PROBE_VA   0x00001000UL
#define TLB_UNMAP_VA   0x00004000UL /* never mapped */

#define SATP_MODE_SV39 (8ULL << 60)

extern volatile enum priv_mode g_cur_mode;
extern volatile int g_mmu_on;

const char *priv_mode_name(enum priv_mode m);

void priv_enter(void (*entry)(void), unsigned long mpp_bits);
void priv_request_mode(int ecall_code);
void priv_force_m(void);

void shell_m(void);
void shell_s(void);
void shell_u(void);

void pmp_allow_all(void);
void mxstatus_clear_maee(void);
void pgtbl_init(void);
int mmu_on(void);
void mmu_off(void);

void test_mapped_load(void);
void test_unmapped_load(void);
void test_tlb_stale(void);

#endif
