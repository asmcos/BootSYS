#ifndef SPI_H
#define SPI_H

#include <stdint.h>

/* Mode 0 = CPOL0 CPHA0; baud_div must be even, >=2. */
int spi_init(unsigned baud_div, int mode);
void spi_set_loopback(int on);
void spi_cs(int assert); /* 1=select (CS low), 0=deselect */

/* Full-duplex xfer; tx may be NULL (sends 0xFF); rx may be NULL. */
int spi_xfer(const uint8_t *tx, uint8_t *rx, unsigned len);

/* JEDEC RDID 0x9F → id[0]=mfr, id[1], id[2]; returns 0 on ok. */
int spi_flash_read_id(uint8_t id[3]);

/* 0x03 read. len is byte count. */
int spi_flash_read(uint32_t addr, uint8_t *buf, unsigned len);

/*
 * Erase the 4 KiB sector at SPI_FLASH_TEST_ADDR, program 16 bytes, read back.
 * Returns 0 on match. Destroys that sector only.
 */
#define SPI_FLASH_TEST_ADDR  0x00010000u
#define SPI_FLASH_TEST_LEN   16u
int spi_flash_rw_test(uint8_t got[SPI_FLASH_TEST_LEN]);

void spi_pins_setup(void); /* pinmux + GPIO CS */

#endif
