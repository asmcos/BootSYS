#include "k230.h"

#define CPU1_PWR_LPI_STATE 0x9110301cUL

#define RST_BIT_RESET  (1u << 0)
#define RST_BIT_DONE   (1u << 12)
#define PWR_BIT_CPU1   (1u << 1)

int cpu1_is_in_hard_reset(void)
{
	return (readl(CPU1_RST_CTL) & RST_BIT_RESET) != 0;
}

/* Compact one-shot status (menu 's'). */
void cpu1_dump_status(const char *tag)
{
	uint32_t pwr = readl(CPU1_PWR_LPI_STATE);
	uint32_t ctl = readl(CPU1_RST_CTL);

	(void)tag;
	uart_puts("cpu1: rst_ctl=0x");
	uart_puthex32_val(ctl);
	uart_puts(cpu1_is_in_hard_reset() ? " reset" : " run");
	uart_puts((pwr & PWR_BIT_CPU1) ? " pwr=on" : " pwr=off");
	uart_puts("\r\n");
}

const char *cpu1_ctl_phase(uint32_t ctl)
{
	if (ctl & RST_BIT_RESET)
		return "in-reset";
	if (ctl == 0x2001u)
		return "cold-default";
	return "released";
}
