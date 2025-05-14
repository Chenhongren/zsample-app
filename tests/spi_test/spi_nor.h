/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPI_NOR_H_
#define SPI_NOR_H_

#include <zephyr/drivers/spi.h>
#include "spi_test.h"

#define SPI_NOR_MAX_ID_LEN 3
#define SPI_NOR_CMD_WRSR   0x01   /* Write status register */
#define SPI_NOR_WEL_BIT    BIT(1) /* Write enable latch */
#define SPI_NOR_QUAD_BIT   BIT(6) /* Write enable latch */

#define SPI_NOR_CMD_WRDI  0x04 /* Write disable */
#define SPI_NOR_CMD_RDSR  0x05 /* Read status register */
#define SPI_NOR_CMD_WREN  0x06 /* Write enable */
#define SPI_NOR_CMD_RDID  0x9F /* Read JEDEC ID */
#define SPI_NOR_CMD_2READ 0xBB /* Read data (1-2-2) */
#define SPI_NOR_CMD_4READ 0xEB /* Read data (1-4-4) */

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NOR_ACCESS_WRITE BIT(7)

static int spi_nor_access(const struct device *const dev, struct spi_config spi_cfg, uint8_t opcode,
			  unsigned int access, off_t addr, void *data, size_t length)
{
	bool is_write = (access & NOR_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = {0};
	struct spi_buf spi_buf[2] = {{
					     .buf = buf,
					     .len = 1,
				     },
				     {.buf = data, .len = length}};

	buf[0] = opcode;
	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length != 0) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf,
		.count = 2,
	};

	if (is_write) {
		return spi_write(dev, &spi_cfg, &tx_set);
	}

	return spi_transceive(dev, &spi_cfg, &tx_set, &rx_set);
}

#define spi_nor_cmd_read(dev, spi_cfg, opcode, dest, length)                                       \
	spi_nor_access(dev, spi_cfg, opcode, 0, 0, dest, length)
#define spi_nor_cmd_write(dev, spi_cfg, opcode)                                                    \
	spi_nor_access(dev, spi_cfg, opcode, NOR_ACCESS_WRITE, 0, NULL, 0)

#endif /* SPI_NOR_H_ */
