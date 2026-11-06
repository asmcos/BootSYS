#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

enum gpio_dir {
	GPIO_IN = 0,
	GPIO_OUT = 1,
};

enum gpio_edge {
	GPIO_EDGE_NONE = 0,
	GPIO_EDGE_RISING,
	GPIO_EDGE_FALLING,
	GPIO_EDGE_BOTH, /* rising via polarity; both needs INT_BOTHEDGE if avail */
};

/* Global pin 0–63 (same numbering as IOMUX io0–63). PMU 64+ not in DW GPIO. */
int gpio_valid(unsigned pin);
void gpio_init_hw(void);

int gpio_set_dir(unsigned pin, enum gpio_dir dir);
int gpio_write(unsigned pin, int level);
int gpio_read(unsigned pin); /* 0/1, or -1 */

/* Edge IRQ: enables GPIO + PLIC; call trap_init first. */
int gpio_irq_config(unsigned pin, enum gpio_edge edge);
int gpio_irq_enable(unsigned pin, int on);
void gpio_irq_eoi(unsigned pin);

uint32_t gpio_irq_count(void);
int gpio_irq_last_pin(void);

/* Called from trap on MEI — returns 1 if handled. */
int gpio_on_mei(void);

#endif
