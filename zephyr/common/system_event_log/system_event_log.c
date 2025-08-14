/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(system_event_log, CONFIG_SYSTEM_EVENT_LOG_LOG_LEVEL);

#include "system_event_log.h"

#define SEL_EVENT_COUNT 10

struct k_thread sel_thread_data;
K_THREAD_STACK_DEFINE(sel_stack_area, CONFIG_SYS_EVT_LOG_STACK_SIZE);

K_MSGQ_DEFINE(system_event_log_msgq, sizeof(struct system_event_log), SEL_EVENT_COUNT,
	      sizeof(uint32_t));

static uint8_t record_id = 0; /* max record id is 255 */
static struct system_event_log sel[CONFIG_SYS_EVT_LOG_MAX_RECORD];
static uint8_t sel_curr_id = 0;
static bool first = true;

static int add_rotate_event(void)
{
	struct system_event_log evt;
	int ret;

	evt.id = record_id++;
	evt.record_type = SEL_RECORD_TYPE_STORAGE;
	evt.event_type = SEL_STORAGE_EVENT_TYPE_RECORD_ID_ROTATE;
	evt.event_data[0] = 0;
	evt.event_data[1] = 0;
	evt.event_data[2] = 0;
	evt.asserted = true;
	evt.ts = get_uptime_timestamp();

	ret = k_msgq_put(&system_event_log_msgq, &evt, K_FOREVER);
	if (ret) {
		LOG_ERR("failed to add record id ratate(%d), ret %d", evt.id, ret);
	}

	return ret;
}

int add_system_event_log(const uint8_t record_type, const uint8_t event_type,
			 const uint32_t event_data, const bool asserted)
{
	struct system_event_log evt;
	int ret;

	if (record_id == 0 && !first) {
		LOG_DBG("the record id rotates");
		ret = add_rotate_event();
		if (ret) {
			LOG_ERR("failed to add rotate sel, ret %d", ret);
			return ret;
		}
	}

	if (first) {
		first = false;
	}

	evt.id = record_id++;
	evt.record_type = record_type;
	evt.event_type = event_type;
	evt.event_data[0] = BYTE_0(event_data);
	evt.event_data[1] = BYTE_1(event_data);
	evt.event_data[2] = BYTE_2(event_data);
	evt.asserted = asserted;
	evt.ts = get_uptime_timestamp();

	ret = k_msgq_put(&system_event_log_msgq, &evt, K_FOREVER);
	if (ret) {
		LOG_ERR("failed to add system event log(%d), ret %d", evt.id, ret);
	}

	return ret ? ret : (uint8_t)(record_id - 1);
}

struct system_event_log *find_system_event_log_by_record_id(const uint8_t id)
{
	for (int i = 0; i < sel_curr_id; i++) {
		if (id == sel[i].id) {
			return &sel[i];
		}
	}

	LOG_ERR("failed to find system event log with record id %d", id);

	return NULL;
}

struct system_event_log *get_system_event_log_by_index(const uint8_t index)
{
	if (index >= sel_curr_id) {
		LOG_ERR("invalid system event log index(index: %d, current count: %d)", index,
			sel_curr_id);
		return NULL;
	}

	return &sel[index];
}

int get_system_event_log_count(void)
{
	return sel_curr_id;
}

int clear_system_event_log(void)
{
	int ret = -1;

	/* clear system event log and add log_clear sel */
	memset(sel, 0, sizeof(sel));
	record_id = 0;
	sel_curr_id = 0;
	first = true;
	ret = add_system_event_log(SEL_RECORD_TYPE_STORAGE, SEL_STORAGE_EVENT_TYPE_EVENT_LOG_CLEAR,
				   0, true);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void sel_handler(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct system_event_log evt;

	while (true) {
		k_msgq_get(&system_event_log_msgq, &evt, K_FOREVER);

		LOG_DBG("adding system event log...");

		if (sel_curr_id >= ARRAY_SIZE(sel)) {
			memmove(sel, &sel[1],
				(ARRAY_SIZE(sel) - 1) * sizeof(struct system_event_log));
			sel_curr_id = ARRAY_SIZE(sel) - 1;
		}

		sel[sel_curr_id] = evt;
		sel_curr_id++;
	}
}

static int system_event_log_init(void)
{
	k_tid_t sel_tid;

	sel_tid = k_thread_create(&sel_thread_data, sel_stack_area,
				  K_THREAD_STACK_SIZEOF(sel_stack_area), sel_handler, NULL, NULL,
				  NULL, CONFIG_SYS_EVT_LOG_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(sel_tid, "system-event-log");

	return 0;
}
SYS_INIT(system_event_log_init, APPLICATION, CONFIG_SYS_EVT_LOG_PRIORITY);
