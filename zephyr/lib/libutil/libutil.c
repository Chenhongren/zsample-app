/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

#include "libutil.h"

struct uptime_timestamp get_uptime_timestamp(void)
{
	struct uptime_timestamp ts;
	uint64_t uptime_ms = k_uptime_get();

	uint64_t total_sec = uptime_ms / 1000;
	ts.hours = total_sec / 3600;
	ts.minutes = (total_sec % 3600) / 60;
	ts.seconds = total_sec % 60;

	return ts;
}
