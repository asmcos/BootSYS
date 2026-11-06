#ifndef SD_H
#define SD_H

#include <stdint.h>

#define SD_BLOCK_SIZE     512u
/* After the GPT entries, before a typical first partition at LBA 2048. */
#define SD_TEST_LBA       256u

void sd_pins_setup(void);

/* Host reset, 3.3 V, ~400 kHz, 74 clocks, then CMD0..CMD7 and 4-bit. */
int sd_card_init(void);

int sd_read_block(uint32_t lba, uint8_t buf[SD_BLOCK_SIZE]);
int sd_write_block(uint32_t lba, const uint8_t buf[SD_BLOCK_SIZE]);

uint32_t sd_rca(void);
uint32_t sd_ocr(void);
int sd_sector_mode(void);
const uint32_t *sd_cid(void); /* 4 words, valid after init */
uint32_t sd_present(void);    /* PRESENT_STATE */
uint32_t sd_last_irq(void);

#endif
