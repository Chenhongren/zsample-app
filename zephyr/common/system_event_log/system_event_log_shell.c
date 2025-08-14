/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include "system_event_log.h"

#define CONF_ARGV_RECORD_TYPE 1
#define CONF_ARGV_EVENT_TYPE  2
#define CONF_ARGV_EVENT_DATA  3
#define CONF_ARGV_ASSERT      4

#define EVENT_DATA(evt)                                                                            \
	FIELD_PREP(BYTE_0_MASK, evt->event_data[0]) |                                              \
		FIELD_PREP(BYTE_1_MASK, evt->event_data[1]) |                                      \
		FIELD_PREP(BYTE_2_MASK, evt->event_data[2])

#define SHELL_PRINT_SEL_HEADER(sh)                                                                 \
	shell_print(sh, "%3s | %8s | %13s | %20s | %10s | %-10s", "ID", "Time", "Record Type",     \
		    "Event Type", "Event Data", "Assert")

static void print_single_sel(const struct shell *sh, const int record_id,
			     const struct uptime_timestamp ts, const uint8_t record_type,
			     const uint8_t event_type, const uint32_t event_data,
			     const uint8_t asserted)
{
	char str[128];

	if (record_type >= SEL_RECORD_TYPE_MAX) {
		snprintf(str, sizeof(str), "%3d | %2d:%2d:%2d | %13d | %20d | %10x | %-10d",
			 record_id, ts.hours, ts.minutes, ts.seconds, record_type, event_data,
			 event_type, asserted);
	} else {
		if (event_type >= sel_event_type_counts[record_type]) {
			snprintf(str, sizeof(str), "%3d | %2d:%2d:%2d | %13s | %20d | %10x | %-10d",
				 record_id, ts.hours, ts.minutes, ts.seconds,
				 sel_record_type_names[record_type], event_type, event_data,
				 asserted);
		} else {
			snprintf(str, sizeof(str), "%3d | %2d:%2d:%2d | %13s | %20s | %10x | %-10s",
				 record_id, ts.hours, ts.minutes, ts.seconds,
				 sel_record_type_names[record_type],
				 sel_event_type_names[record_type][event_type], event_data,
				 asserted ? "asserted" : "deasserted");
		}
	}

	shell_print(sh, "%s", str);
}

/* sel list [<id>] */
static int cmd_sel_list(const struct shell *sh, size_t argc, char **argv)
{
	struct system_event_log *evt;
	bool list_all = argc == 1 ? true : false;

	if (!list_all) {
		char *endptr;
		int id = -1;

		id = strtol(argv[1], &endptr, 10);
		if (*endptr != '\0') {
			shell_error(sh, "invalid record id");
			return -EINVAL;
		}

		evt = find_system_event_log_by_record_id(id);
		if (!evt) {
			return -EINVAL;
		}

		SHELL_PRINT_SEL_HEADER(sh);
		print_single_sel(sh, evt->id, evt->ts, evt->record_type, evt->event_type,
				 EVENT_DATA(evt), evt->asserted);
		return 0;
	}

	SHELL_PRINT_SEL_HEADER(sh);

	for (int i = 0; i < get_system_event_log_count(); i++) {
		evt = get_system_event_log_by_index(i);
		if (!evt) {
			break;
		}
		print_single_sel(sh, evt->id, evt->ts, evt->record_type, evt->event_type,
				 EVENT_DATA(evt), evt->asserted);
	}

	return 0;
}

/* sel add <record type> <event type> <event data> <assert> */
static int cmd_sel_add(const struct shell *sh, size_t argc, char **argv)
{
	struct system_event_log *evt;
	char *endptr;
	int id, record_type, event_type, asserted;
	uint32_t event_data;

	record_type = strtol(argv[CONF_ARGV_RECORD_TYPE], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "invalid record type");
		return -EINVAL;
	}
	event_type = strtol(argv[CONF_ARGV_EVENT_TYPE], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "invalid event type");
		return -EINVAL;
	}
	event_data = strtol(argv[CONF_ARGV_EVENT_DATA], &endptr, 16);
	if (*endptr != '\0') {
		shell_error(sh, "invalid event data");
		return -EINVAL;
	}
	asserted = strtol(argv[CONF_ARGV_ASSERT], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "invalid assert");
		return -EINVAL;
	}

	id = add_system_event_log(record_type, event_type, event_data, asserted ? true : false);
	if (id < 0) {
		return id;
	}

	evt = find_system_event_log_by_record_id(id);
	if (!evt) {
		return -EINVAL;
	}

	SHELL_PRINT_SEL_HEADER(sh);
	print_single_sel(sh, evt->id, evt->ts, evt->record_type, evt->event_type, EVENT_DATA(evt),
			 evt->asserted);

	return 0;
}

/* sel clear */
static int cmd_sel_clear(const struct shell *sh, size_t argc, char **argv)
{
	return clear_system_event_log();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sel_cmds,
	SHELL_CMD_ARG(list, NULL,
		      "List system event log\n"
		      "Usage: list [<id>]",
		      cmd_sel_list, 1, 1),
	SHELL_CMD_ARG(add, NULL,
		      "Add system event log\n"
		      "Usage: add <record type> <event type> <event data> <assert>",
		      cmd_sel_add, 5, 0),
	SHELL_CMD_ARG(clear, NULL,
		      "Remove system event log\n"
		      "Usage: clear",
		      cmd_sel_clear, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(sel, &sub_sel_cmds, "system event log commands", NULL);
