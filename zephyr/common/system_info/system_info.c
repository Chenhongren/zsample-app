/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "flash.h"
#include "plat_info.h"

LOG_MODULE_REGISTER(system_info, CONFIG_SYSTEM_INFO_LOG_LEVEL);

#define SOC_INFO_MAX_LENGTH 32 /* 16 bytes * 2 */

enum platform_info_index {
	PLAT_INFO_ID_PRJ_NAME = 0,
	PLAT_INFO_ID_FW_VERSION,
	PLAT_INFO_ID_SOC_INFO,
	PLAT_INFO_ID_MAX,
};

static const char *const platform_label[] = {
	[PLAT_INFO_ID_PRJ_NAME] = "project name",
	[PLAT_INFO_ID_FW_VERSION] = "firmware version",
	[PLAT_INFO_ID_SOC_INFO] = "soc info",
};

static char soc_info_buffer[SOC_INFO_MAX_LENGTH] = "Undefined";

static char *platform_info[] = {
	[PLAT_INFO_ID_PRJ_NAME] = PROJECT_NAME,
	[PLAT_INFO_ID_FW_VERSION] = FIRMWARE_VERSION,
	[PLAT_INFO_ID_SOC_INFO] = soc_info_buffer,
};

struct entry {
	uint8_t len;
	uint8_t data[63];
} __packed;

struct flash_layout {
	struct entry project_name;
	struct entry firmware_version;
	struct entry soc_info;
};

static int update_soc_info(void)
{
	uint8_t dev_id[SOC_INFO_MAX_LENGTH / 2];
	int offset = 0;
	ssize_t length;

	length = hwinfo_get_device_id(dev_id, sizeof(dev_id));
	if (length == -ENOSYS) {
		LOG_ERR("unsupported hwinfo by hardware");
		return -ENOSYS;
	}
	if (length < 0) {
		LOG_ERR("failed to get device id, ret %d", length);
		return length;
	}

	LOG_HEXDUMP_DBG(dev_id, length, "device id:");

	for (int i = 0; i < length; i++) {
		if (offset >= sizeof(soc_info_buffer) - 3) {
			LOG_ERR("soc info string truncated due to limited buffer size");
			return -ENOBUFS;
		}
		offset += snprintf(soc_info_buffer + offset, sizeof(soc_info_buffer) - offset,
				   "%02x", dev_id[i]);
	}

	return 0;
}

int store_project_info(void)
{
	const struct device *fru_dev = DEVICE_DT_GET(DT_ALIAS(fru_flash));
	struct flash_layout fru;
	int ret;

	update_soc_info();

	if (!device_is_ready(fru_dev)) {
		LOG_ERR("%s is not ready", fru_dev->name);
		return -ENODEV;
	}

	if (strlen(platform_info[PLAT_INFO_ID_PRJ_NAME]) > sizeof(fru.project_name.data) ||
	    strlen(platform_info[PLAT_INFO_ID_FW_VERSION]) > sizeof(fru.firmware_version.data)) {
		LOG_ERR("project name or firmware version string is oversize");
		return -ENOSPC;
	}

	memset(&fru, 0xff, sizeof(fru));

	fru.project_name.len = strlen(platform_info[PLAT_INFO_ID_PRJ_NAME]);
	memcpy(fru.project_name.data, platform_info[PLAT_INFO_ID_PRJ_NAME], fru.project_name.len);
	fru.firmware_version.len = strlen(platform_info[PLAT_INFO_ID_FW_VERSION]);
	memcpy(fru.firmware_version.data, platform_info[PLAT_INFO_ID_FW_VERSION],
	       fru.firmware_version.len);
	fru.soc_info.len = strlen(platform_info[PLAT_INFO_ID_SOC_INFO]);
	memcpy(fru.soc_info.data, platform_info[PLAT_INFO_ID_SOC_INFO], fru.soc_info.len);

	ret = flash_update(fru_dev, PLAT_FLASH_PRJ_INFO_ADDR, &fru, sizeof(fru));
	if (!ret) {
		LOG_INF("store platform information at flash address 0x%x",
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
