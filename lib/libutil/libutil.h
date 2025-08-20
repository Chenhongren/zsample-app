/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBUTIL_H_
#define LIBUTIL_H_

#define BYTE_0_MASK GENMASK(7, 0)
#define BYTE_1_MASK GENMASK(15, 8)
#define BYTE_2_MASK GENMASK(24, 16)
#define BYTE_0(x)   FIELD_GET(BYTE_0_MASK, x)
#define BYTE_1(x)   FIELD_GET(BYTE_1_MASK, x)
#define BYTE_2(x)   FIELD_GET(BYTE_2_MASK, x)

struct uptime_timestamp {
	uint32_t hours;
	uint32_t minutes;
	uint32_t seconds;
};

struct uptime_timestamp get_uptime_timestamp(void);

#endif /* LIBUTIL_H_ */
