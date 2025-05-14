/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "plat_info.h"
#include "system_event_log.h"
#include "system_info.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	int ret;

	LOG_INF("Zephyr %s application, version %s\n", PROJECT_NAME, FIRMWARE_VERSION);

	ret = store_project_info();
	if (ret) {
		return ret;
	}

	system_event_log_init();

	return 0;
}
