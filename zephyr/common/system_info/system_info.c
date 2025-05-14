/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "flash.h"
#include "plat_info.h"

LOG_MODULE_REGISTER(system_info, CONFIG_SYSTEM_INFO_LOG_LEVEL);

struct entry {
	uint8_t len;
	uint8_t data[63];
} __packed;

struct eeprom_layout {
	struct entry project_name;
	struct entry firmware_version;
};

int store_project_info(void)
{
	const struct device *fru_dev = DEVICE_DT_GET(DT_ALIAS(fru_eeprom));
	struct eeprom_layout fru;
	int ret;

	if (!device_is_ready(fru_dev)) {
		LOG_ERR("%s is not ready", fru_dev->name);
		return -ENODEV;
	}

	if (strlen(PROJECT_NAME) > sizeof(fru.project_name.data) ||
	    strlen(FIRMWARE_VERSION) > sizeof(fru.firmware_version.data)) {
		LOG_ERR("project name or firmware version string is oversize");
		return -ENOSPC;
	}

	memset(fru.project_name.data, 0xff, sizeof(fru.project_name.data));
	memset(fru.firmware_version.data, 0xff, sizeof(fru.firmware_version.data));

	fru.project_name.len = strlen(PROJECT_NAME);
	memcpy(fru.project_name.data, PROJECT_NAME, fru.project_name.len);
	fru.firmware_version.len = strlen(FIRMWARE_VERSION);
	memcpy(fru.firmware_version.data, FIRMWARE_VERSION, fru.firmware_version.len);

	ret = flash_update(fru_dev, PLAT_EEPROM_PRJ_INFO_ADDR, &fru, sizeof(fru));
	if (!ret) {
		LOG_INF("store platform information at EEPROM address 0x%x",
			PLAT_EEPROM_PRJ_INFO_ADDR);
	} else if (ret == -EALREADY) {
		LOG_INF("platform information has already been set");
		ret = 0;
	} else {
		LOG_ERR("failed to update flash, ret %d", ret);
	}

	return ret;
}
