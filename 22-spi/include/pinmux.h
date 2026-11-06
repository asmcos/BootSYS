#ifndef PINMUX_H
#define PINMUX_H

#include <stdint.h>

/* IOMUX pad register bits (K230 TRM / Linux canaan,k230-iomux). */
#define PIN_ST          (1u << 0)
#define PIN_DS_SHIFT    1
#define PIN_DS_MASK     (0xfu << PIN_DS_SHIFT)
#define PIN_PD          (1u << 5)
#define PIN_PU          (1u << 6)
#define PIN_OE          (1u << 7)
#define PIN_IE          (1u << 8)
#define PIN_MSC         (1u << 9)
#define PIN_SL          (1u << 10)
#define PIN_SEL_SHIFT   11
#define PIN_SEL_MASK    (0x7u << PIN_SEL_SHIFT)
#define PIN_DI          (1u << 31)

/* Soft pull preference (maps to PU/PD bits). */
enum pin_pull {
	PIN_PULL_NONE = 0,
	PIN_PULL_UP,
	PIN_PULL_DOWN,
};

/* pin: 0–63 main IOMUX; 64–71 PMU IOMUX. Returns 0 on success. */
int pinmux_valid(unsigned pin);
uint32_t pinmux_read(unsigned pin);
int pinmux_write(unsigned pin, uint32_t val);

unsigned pinmux_get_sel(unsigned pin);
int pinmux_set_sel(unsigned pin, unsigned sel); /* sel 0..7 (alt0..alt7) */

int pinmux_set_ie(unsigned pin, int on);
int pinmux_set_oe(unsigned pin, int on);
int pinmux_set_pull(unsigned pin, enum pin_pull pull);
int pinmux_set_ds(unsigned pin, unsigned ds); /* 0..15 */
int pinmux_set_sl(unsigned pin, int on);
int pinmux_set_st(unsigned pin, int on);

/* Convenience: mux to GPIO (alt0) with IE, optional OE, ds=8, no pull. */
int pinmux_as_gpio(unsigned pin, int output);

/* Restore UART0 console pads (io38/io39 → alt1). */
void pinmux_restore_uart0(void);

void pinmux_dump(unsigned pin);

#endif
