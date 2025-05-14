/*
 * Copyright (c) 2025 ITE Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FLASH_H_
#define FLASH_H_

int flash_update(const struct device *fru_dev, off_t w_addr, void *buf_array, size_t buf_len);

#endif /* FLASH_H_ */
