/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_EVENT_TABLE_H_
#define SYSTEM_EVENT_TABLE_H_

enum SEL_RECORD_TYPE_CODE {
	SEL_RECORD_TYPE_SYSTEM = 0,
	SEL_RECORD_TYPE_STORAGE,
	SEL_RECORD_TYPE_KSCAN,
	SEL_RECORD_TYPE_MAX,
};

enum SEL_SYSTEM_EVENT_TYPE_CODE {
	SEL_SYSTEM_EVENT_TYPE_BOOT = 0,
};

enum SEL_STORAGE_EVENT_TYPE_CODE {
	SEL_STORAGE_EVENT_TYPE_PROJ_INFO = 0,
	SEL_STORAGE_EVENT_TYPE_RECORD_ID_ROTATE,
	SEL_STORAGE_EVENT_TYPE_EVENT_LOG_CLEAR,
};

enum SEL_KSCAN_EVENT_TYPE_CODE {
	SEL_KSCAN_EVENT_TYPE_KEYCODE_CHANGED = 0,
};

static const char *const sel_record_type_names[] = {
	[SEL_RECORD_TYPE_SYSTEM] = "system",
	[SEL_RECORD_TYPE_STORAGE] = "storage",
	[SEL_RECORD_TYPE_KSCAN] = "kscan",
};

static const char *const sel_system_events[] = {
	[SEL_SYSTEM_EVENT_TYPE_BOOT] = "boot",
};

static const char *const sel_storage_events[] = {
	[SEL_STORAGE_EVENT_TYPE_PROJ_INFO] = "project_info",
	[SEL_STORAGE_EVENT_TYPE_RECORD_ID_ROTATE] = "record_id_rotate",
	[SEL_STORAGE_EVENT_TYPE_EVENT_LOG_CLEAR] = "event_log_clear",
};

static const char *const sel_kscan_events[] = {
	[SEL_KSCAN_EVENT_TYPE_KEYCODE_CHANGED] = "keycode_changed",
};

static const char *const *const sel_event_type_names[SEL_RECORD_TYPE_MAX] = {
	[SEL_RECORD_TYPE_SYSTEM] = sel_system_events,
	[SEL_RECORD_TYPE_STORAGE] = sel_storage_events,
	[SEL_RECORD_TYPE_KSCAN] = sel_kscan_events,
};

static const size_t sel_event_type_counts[SEL_RECORD_TYPE_MAX] = {
	[SEL_RECORD_TYPE_SYSTEM] = ARRAY_SIZE(sel_system_events),
	[SEL_RECORD_TYPE_STORAGE] = ARRAY_SIZE(sel_storage_events),
	[SEL_RECORD_TYPE_KSCAN] = ARRAY_SIZE(sel_kscan_events),
};

#endif /* SYSTEM_EVENT_TABLE_H_ */
