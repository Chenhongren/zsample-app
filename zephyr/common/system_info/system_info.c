/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "flash.h"
#include "plat_info.h"

LOG_MODULE_REGISTER(system_info, CONFIG_SYSTEM_INFO_LOG_LEVEL);

enum platform_info_index {
	PLAT_INFO_ID_PRJ_NAME = 0,
	PLAT_INFO_ID_FW_VERSION,
	PLAT_INFO_ID_MAX,
};

static const char *const platform_label[] = {
	[PLAT_INFO_ID_PRJ_NAME] = "project name",
	[PLAT_INFO_ID_FW_VERSION] = "firmware version",
};

static const char *const platform_info[] = {
	[PLAT_INFO_ID_PRJ_NAME] = PROJECT_NAME,
	[PLAT_INFO_ID_FW_VERSION] = FIRMWARE_VERSION,
};

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

	if (strlen(platform_info[PLAT_INFO_ID_PRJ_NAME]) > sizeof(fru.project_name.data) ||
	    strlen(platform_info[PLAT_INFO_ID_FW_VERSION]) > sizeof(fru.firmware_version.data)) {
		LOG_ERR("project name or firmware version string is oversize");
		return -ENOSPC;
	}

	memset(fru.project_name.data, 0xff, sizeof(fru.project_name.data));
	memset(fru.firmware_version.data, 0xff, sizeof(fru.firmware_version.data));

	fru.project_name.len = strlen(platform_info[PLAT_INFO_ID_PRJ_NAME]);
	memcpy(fru.project_name.data, platform_info[PLAT_INFO_ID_PRJ_NAME], fru.project_name.len);
	fru.firmware_version.len = strlen(platform_info[PLAT_INFO_ID_FW_VERSION]);
	memcpy(fru.firmware_version.data, platform_info[PLAT_INFO_ID_FW_VERSION],
	       fru.firmware_version.len);

	ret = flash_update(fru_dev, PLAT_FLASH_PRJ_INFO_ADDR, &fru, sizeof(fru));
	if (!ret) {
		LOG_INF("store platform information at EEPROM address 0x%x",
			PLAT_FLASH_PRJ_INFO_ADDR);
	} else if (ret == -EALREADY) {
		LOG_INF("platform information has already been set");
		ret = 0;
	} else {
		LOG_ERR("failed to update flash, ret %d", ret);
	}

	return ret;
}

/* platform info [<id>] */
static int cmd_platform_info(const struct shell *sh, size_t argc, char **argv)
{
	switch (argc) {
	case 1:
		shell_print(sh, "Platform Info:");
		for (uint8_t i = 0; i < PLAT_INFO_ID_MAX; i++) {
			shell_print(sh, "\t<ID: %d> %s: %s", i, platform_label[i],
				    platform_info[i]);
		}
		break;
	case 2:
		char *endptr;
		int id;

		id = strtol(argv[1], &endptr, 10);
		if (*endptr != '\0' || id >= PLAT_INFO_ID_MAX) {
			shell_error(sh, "invalid id(%s)", argv[1]);
			return -EINVAL;
		}
		shell_print(sh, "Platform Info (ID: %d) %s: %s", id, platform_label[id],
			    platform_info[id]);
		break;
	default:
		break;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_platform_cmds,
			       SHELL_CMD_ARG(info, NULL,
					     "Print platform infomation\n"
					     "Usage: info [<id>]",
					     cmd_platform_info, 1, 1),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(platform, &sub_platform_cmds, "platform infomation commands", NULL);
