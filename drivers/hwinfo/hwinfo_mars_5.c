/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <chip_chipregs.h>
#include <string.h>

struct it51xxx_device_id {
	uint8_t chip_version;
	uint8_t chip_id[3];
};

#define GCTRL_BASE_ADDR      DT_REG_ADDR(DT_NODELABEL(gctrl))
#define GCTRL02_CHIP_VERSION 0x02
#define GCTRL85_CHIP_ID_1    0x85
#define GCTRL86_CHIP_ID_2    0x86
#define GCTRL87_CHIP_ID_3    0x87

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct it51xxx_device_id dev_id;

	if (!buffer) {
		return -EINVAL;
	}

	if (length > sizeof(dev_id)) {
		length = sizeof(dev_id);
	}

	dev_id.chip_version = sys_read8(GCTRL_BASE_ADDR + GCTRL02_CHIP_VERSION);
	dev_id.chip_id[0] = sys_read8(GCTRL_BASE_ADDR + GCTRL85_CHIP_ID_1);
	dev_id.chip_id[1] = sys_read8(GCTRL_BASE_ADDR + GCTRL86_CHIP_ID_2);
	dev_id.chip_id[2] = sys_read8(GCTRL_BASE_ADDR + GCTRL87_CHIP_ID_3);

	memcpy(buffer, &dev_id, sizeof(dev_id));

	return length;
}
