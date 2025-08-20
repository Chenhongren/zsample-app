/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_EVENT_LOG_H_
#define SYSTEM_EVENT_LOG_H_

#include "app/lib/libutil.h"
#include "system_event_table.h"

struct system_event_log {
	uint8_t id;
	struct uptime_timestamp ts;
	uint8_t record_type;
	uint8_t event_type;
	uint8_t event_data[3];
	bool asserted;
};

/**
 * @brief Function to finds the system event log by its record ID.
 *
 * @param[in] id The unique record ID of the system event log..
 *
 * @retval A pointer to the system event log struct if found, otherwise NULL.
 */
struct system_event_log *find_system_event_log_by_record_id(const uint8_t id);

/**
 * @brief Function to retrieve system event log by its index in the log array.
 *
 * @param[in] index The zero-based index of the event log.
 *
 * @retval A pointer to the system event log struct at the given index,
 *         or NULL if the index is out of bounds.
 */
struct system_event_log *get_system_event_log_by_index(const uint8_t index);

/**
 * @brief Retrieves the current number of system event logs.
 *
 * @retval The current count of logs.
 */
int get_system_event_log_count(void);

/**
 * @brief Add an entry to the system event log.
 *
 * @param[in] record_type The type of record.
 * @param[in] event_type  The type of event.
 * @param[in] event_data  The data associated with the event.
 * @param[in] asserted    True if the event is an assertion, false for deassertion.
 *
 * @retval The new record id on success.
 * @retval A negative error code on failure.
 */
int add_system_event_log(const uint8_t record_type, const uint8_t event_typee,
			 const uint32_t event_data, const bool asserted);

/**
 * @brief Clear all entries from the system event log.
 *
 * @retval  0 on success.
 * @retval A negative error code on failure.
 */
int clear_system_event_log(void);

#endif /* SYSTEM_EVENT_LOG_H_ */
