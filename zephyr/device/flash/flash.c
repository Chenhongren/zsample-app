/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flash_dev, CONFIG_FLASH_DEVICE_LOG_LEVEL);

#define SPI_FLASH_SECTOR_SIZE 0x1000

int flash_update(const struct device *fru_dev, off_t w_addr, void *buf_array, size_t buf_len)
{
	uint8_t *check_array;
	int ret;

	check_array = malloc(buf_len * sizeof(uint8_t));
	if (!check_array) {
		return -ENOMEM;
	}

	ret = flash_read(fru_dev, w_addr, check_array, buf_len);
	if (ret != 0) {
		goto out;
	}

	if (memcmp(buf_array, check_array, buf_len) == 0) {
		ret = -EALREADY;
		goto out;
	}

	ret = flash_erase(fru_dev, w_addr, SPI_FLASH_SECTOR_SIZE);
	if (ret != 0) {
		goto out;
	}

	ret = flash_write(fru_dev, w_addr, buf_array, buf_len);
	if (ret != 0) {
		goto out;
	}

	ret = flash_read(fru_dev, w_addr, check_array, buf_len);
	if (ret != 0) {
		goto out;
	}

	if (memcmp(buf_array, check_array, buf_len) != 0) {
		ret = -EINVAL;
	}

out:
	free(check_array);
	check_array = NULL;

	return ret;
}
