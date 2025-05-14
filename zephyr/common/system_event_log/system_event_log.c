/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(system_event_log, CONFIG_SYSTEM_EVENT_LOG_LOG_LEVEL);

int system_event_log_init(void)
{

	/* TODO */
	LOG_INF("%s ITE Debug %d", __func__, __LINE__);

	return 0;
}