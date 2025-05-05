/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPI_TEST_H_
#define SPI_TEST_H_

#include <zephyr/drivers/spi.h>

#define SPI_FLASH_TEST_REGION_OFFSET 0x0

#if CONFIG_SPI_TEST_PM_CHECK
#define SPI_FLASH_SECTOR_SIZE 0x1000 * 3 /* base 0x3000 = 12288 bytes */
#else
#define SPI_FLASH_SECTOR_SIZE 0x1000 * 1 /* base 0x1000 = 4096 bytes */
#endif

int spi_test(void);

#endif /* SPI_TEST_H_ */
