/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#ifdef CONFIG_SPI_TEST
#include "spi_test.h"
#endif /* CONFIG_SPI_TEST */

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	int ret = 0;

	LOG_INF("Zephyr sample application %s\n", APP_VERSION_STRING);

#ifdef CONFIG_SPI_TEST
	ret = spi_test();
	if (ret) {
		return ret;
	}
#endif /* CONFIG_SPI_TEST */

	return ret;
}
