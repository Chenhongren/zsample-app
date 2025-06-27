/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_test, LOG_LEVEL_INF);

#include "spi_nor.h"

static struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8),
	.frequency = MHZ(48),
	.slave = 0,
};

#if CONFIG_SPI_TEST_ASYNC_TRANSFER
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
__maybe_unused static struct k_poll_event async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig);
static K_SEM_DEFINE(caller, 0, 1);
K_THREAD_STACK_DEFINE(spi_async_stack, CONFIG_SPI_TEST_ASYNC_STACK_SIZE);

static void spi_async_call_cb(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	struct k_poll_event *evt = p1;
	struct k_sem *caller_sem = p2;
	int ret, result = 1;

	LOG_INF("pooling...");

	while (1) {
		ret = k_poll(evt, 1, K_MSEC(2000));

		if (!ret) {
			result = evt->signal->result;
			k_sem_give(caller_sem);

			/* Reinitializing for next call */
			evt->signal->signaled = 0U;
			evt->state = K_POLL_STATE_NOT_READY;
			return;
		}
	}
}

static int read_jedec_id_async(const struct device *dev)
{
	int ret;
	uint8_t tx_data[1] = {SPI_NOR_CMD_RDID};
	uint8_t rx_data[3] = {0};
	struct spi_buf tx_buf[1] = {{
		.buf = tx_data,
		.len = sizeof(tx_data),
	}};
	struct spi_buf rx_buf[2] = {{
					    .buf = NULL,
					    .len = sizeof(tx_data),
				    },
				    {
					    .buf = rx_data,
					    .len = sizeof(rx_data),
				    }};

	const struct spi_buf_set tx_set = {.buffers = tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = rx_buf, .count = 2};

	ret = spi_transceive_signal(dev, &spi_cfg, &tx_set, &rx_set, &async_sig);
	if (ret) {
		LOG_ERR("failed to send spi data");
		return ret;
	}

	k_sem_take(&caller, K_FOREVER);

	LOG_HEXDUMP_INF(rx_buf[1].buf, rx_buf[1].len, "jedec id:");

	return 0;
}
#endif /* CONFIG_SPI_TEST_ASYNC_TRANSFER */

static int flash_access_test(const int len)
{
	const struct device *flash_dev = DEVICE_DT_GET(DT_ALIAS(spi_flash0));
	uint8_t *expected;
	uint8_t *buffer;
	int ret;

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("%s is not ready", flash_dev->name);
		return -ENODEV;
	}

	expected = malloc(len * sizeof(uint8_t));
	buffer = malloc(len * sizeof(uint8_t));
	if (!buffer || !expected) {
		LOG_ERR("failed to allocate memory");
		ret = -ENOMEM;
		goto out;
	}

	ret = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, SPI_FLASH_SECTOR_SIZE);
	if (ret != 0) {
		LOG_ERR("failed to erase flash, ret %d", ret);
		goto out;
	}

	memset(buffer, 0, len);
	ret = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buffer, len);
	if (ret != 0) {
		LOG_ERR("failed to read data, ret %d", ret);
		goto out;
	}

	for (int i = 0; i < len; i++) {
		expected[i] = (i / 10) * 0x10 + (i % 10);
	}

	LOG_INF("attempting to write %zu bytes", len);
	ret = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
	if (ret != 0) {
		LOG_ERR("failed to write data, ret %d", ret);
		goto out;
	}

	memset(buffer, 0, len);
	ret = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buffer, len);
	if (ret != 0) {
		LOG_ERR("failed to read data, ret %d", ret);
		goto out;
	}

	if (memcmp(expected, buffer, len) == 0) {
		LOG_INF("passed for data comparison");
	} else {
		LOG_ERR("failed for data comparison");
	}

out:
	free(expected);
	free(buffer);
	expected = NULL;
	buffer = NULL;

	return ret;
}

#if CONFIG_SPI_TEST_PM_CHECK
static bool start_pm_check = false;
static void pm_check_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int test_cnt = 0;

	do {
		if (!start_pm_check) {
			goto out;
		}

		flash_access_test(512);
out:
		k_sleep(K_MSEC(1000));
		test_cnt++;
	} while (test_cnt <= 10);
}

K_THREAD_STACK_DEFINE(pm_check_stack, CONFIG_SPI_TEST_PM_CHECK_STACK_SIZE);
K_THREAD_DEFINE(pm_check, CONFIG_SPI_TEST_PM_CHECK_STACK_SIZE, pm_check_thread, NULL, NULL, NULL, 7,
		0, 0);
#endif /* CONFIG_SPI_TEST_PM_CHECK */

__maybe_unused static int read_jedec_id(const struct device *dev)
{
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	int ret;

	ret = spi_nor_cmd_read(dev, spi_cfg, SPI_NOR_CMD_RDID, jedec_id, SPI_NOR_MAX_ID_LEN);
	if (ret) {
		LOG_ERR("failed to read spi data");
		return ret;
	}

	LOG_HEXDUMP_INF(jedec_id, sizeof(jedec_id), "jedec id:");

	return 0;
}

#if CONFIG_SPI_TEST_DUAL_MODE
static int dual_mode_test(const struct device *dev)
{
	uint8_t tx_data[] = {SPI_NOR_CMD_2READ, 0x0, 0x0, 0x0, 0xFF};
	uint8_t rx_data[16] = {0};
	struct spi_buf spi_buf[2] = {{
					     .buf = tx_data,
					     .len = sizeof(tx_data),
				     },
				     {
					     .buf = rx_data,
					     .len = sizeof(rx_data),
				     }};

	const struct spi_buf_set tx_set = {.buffers = spi_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = spi_buf + 1, .count = 1};
	int ret;

	ret = spi_transceive(dev, &spi_cfg, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("failed to send spi data: %d\n", ret);
		return ret;
	}
	LOG_HEXDUMP_INF(spi_buf[1].buf, spi_buf[1].len, "dual read:");

	return 0;
}
#endif /* CONFIG_SPI_TEST_DUAL_MODE */

#if CONFIG_SPI_TEST_QUAD_MODE
static int quad_enable(const struct device *dev, const bool enable)
{
	int ret;
	uint8_t reg;

	/* write enable */
	ret = spi_nor_cmd_write(dev, spi_cfg, SPI_NOR_CMD_WREN);
	do {
		ret = spi_nor_cmd_read(dev, spi_cfg, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
		k_sleep(K_MSEC(10));
	} while ((reg & SPI_NOR_WEL_BIT) != SPI_NOR_WEL_BIT);

	/* Quad enable or disable*/
	uint8_t tx_data[] = {SPI_NOR_CMD_WRSR, SPI_NOR_QUAD_BIT};
	struct spi_buf spi_buf[1] = {
		{
			.buf = tx_data,
			.len = sizeof(tx_data),
		},
	};

	if (enable) {
		tx_data[1] = SPI_NOR_QUAD_BIT;
	} else {
		tx_data[1] = 0;
	}

	const struct spi_buf_set tx_set = {.buffers = spi_buf, .count = 1};
	ret = spi_transceive(dev, &spi_cfg, &tx_set, NULL);
	if (ret) {
		LOG_ERR("failed to send spi data: %d\n", ret);
		return ret;
	}
	do {
		ret = spi_nor_cmd_read(dev, spi_cfg, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
		k_sleep(K_MSEC(10));
	} while ((reg & SPI_NOR_QUAD_BIT) != SPI_NOR_QUAD_BIT);

	/* Write disable */
	ret = spi_nor_cmd_write(dev, spi_cfg, SPI_NOR_CMD_WRDI);
	do {
		ret = spi_nor_cmd_read(dev, spi_cfg, SPI_NOR_CMD_RDSR, &reg, sizeof(reg));
		k_sleep(K_MSEC(10));
	} while ((reg & SPI_NOR_WEL_BIT) == SPI_NOR_WEL_BIT);

	return 0;
}

static int quad_mode_test(const struct device *dev)
{
	uint8_t tx_data[] = {SPI_NOR_CMD_4READ, 0x0, 0x0, 0x0, 0x0, 0xFF, 0xFF};
	uint8_t rx_data[16] = {0};
	struct spi_buf spi_buf[2] = {{
					     .buf = tx_data,
					     .len = sizeof(tx_data),
				     },
				     {
					     .buf = rx_data,
					     .len = sizeof(rx_data),
				     }};

	const struct spi_buf_set tx_set = {.buffers = spi_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = spi_buf + 1, .count = 1};
	int ret;

	ret = spi_transceive(dev, &spi_cfg, &tx_set, &rx_set);

	if (ret) {
		LOG_ERR("failed to send spi data: %d", ret);
		return ret;
	}

	LOG_HEXDUMP_INF(spi_buf[1].buf, spi_buf[1].len, "quad read:");

	return 0;
}
#endif /* CONFIG_SPI_TEST_QUAD_MODE */

#if CONFIG_SPI_TEST_RX_ONLY
static int test_rx_only(const struct device *dev, uint8_t *rx_data, const size_t rx_len)
{
	int ret;
	struct spi_buf spi_buf[1] = {{
		.buf = rx_data,
		.len = rx_len,
	}};

	const struct spi_buf_set rx_set = {.buffers = spi_buf, .count = 1};

	ret = spi_read(dev, &spi_cfg, &rx_set);
	if (ret) {
		LOG_ERR("failed to read spi data");
		return ret;
	}

	LOG_HEXDUMP_INF(spi_buf[0].buf, spi_buf[0].len, "rx:");

	return 0;
}

static int test_multi_rx(const struct device *dev)
{
	int ret;
	uint8_t rx_data[16] = {0xFF};
	uint8_t rx_data_2[16] = {0xFF};
	struct spi_buf spi_buf[2] = {{
					     .buf = rx_data,
					     .len = sizeof(rx_data),
				     },
				     {
					     .buf = rx_data_2,
					     .len = sizeof(rx_data_2),
				     }};

	const struct spi_buf_set rx_set = {.buffers = spi_buf, .count = 2};

	ret = spi_read(dev, &spi_cfg, &rx_set);
	if (ret) {
		LOG_ERR("failed to read spi data");
		return ret;
	}

	LOG_HEXDUMP_INF(spi_buf[0].buf, spi_buf[0].len, "rx 0:");
	LOG_HEXDUMP_INF(spi_buf[1].buf, spi_buf[1].len, "rx 1:");

	return 0;
}
#endif /* CONFIG_SPI_TEST_RX_ONLY*/

#if CONFIG_SPI_TEST_TX_ONLY
static int test_tx_only(const struct device *dev, uint8_t *tx_data, const size_t tx_len)
{
	int ret;
	struct spi_buf spi_buf = {
		.buf = tx_data,
		.len = tx_len,
	};

	const struct spi_buf_set tx_set = {.buffers = &spi_buf, .count = 1};

	ret = spi_write(dev, &spi_cfg, &tx_set);
	if (ret) {
		LOG_ERR("failed to send spi data, ret %d", ret);
		return ret;
	}

	return 0;
}

static int test_multi_tx(const struct device *dev)
{
	int ret;
	uint8_t tx_data[4] = {0x2, 0x0, 0x0, 0x10};
	uint8_t tx_data_1[4] = {0x10, 0x11, 0x12, 0x13};
	struct spi_buf tx_buf[2] = {{
					    .buf = tx_data,
					    .len = sizeof(tx_data),
				    },
				    {
					    .buf = tx_data_1,
					    .len = sizeof(tx_data_1),
				    }};

	const struct spi_buf_set tx_set = {.buffers = tx_buf, .count = 2};

	ret = spi_write(dev, &spi_cfg, &tx_set);
	if (ret) {
		LOG_ERR("failed to send spi data, ret %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_SPI_TEST_TX_ONLY */

#if CONFIG_SPI_TEST_TX_RX
static int test_tx_rx(const struct device *dev, uint8_t *tx_data, const size_t tx_len,
		      uint8_t *rx_data, const size_t rx_len)
{
	int ret;
	struct spi_buf tx_buf[1] = {{
		.buf = tx_data,
		.len = tx_len,
	}};
	struct spi_buf rx_buf[1] = {{
		.buf = rx_data,
		.len = rx_len,
	}};

	const struct spi_buf_set tx_set = {.buffers = tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = rx_buf, .count = 1};

	ret = spi_transceive(dev, &spi_cfg, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("failed to send spi data, ret %d", ret);
		return ret;
	}

	LOG_HEXDUMP_INF(rx_buf[0].buf, rx_buf[0].len, "rx:");

	return 0;
}
#endif /* CONFIG_SPI_TEST_TX_RX */

int spi_test(void)
{
	struct device *spi_device = (struct device *)DEVICE_DT_GET(DT_NODELABEL(spi0));
	int ret;
	uint32_t source_clk;

	source_clk = KHZ(CONFIG_SPI_TEST_CLOCK_SOURCE_KHZ);

	spi_cfg.operation |= SPI_LINES_SINGLE;
	spi_cfg.frequency = source_clk;
	LOG_INF("default spi frequency %dHz", spi_cfg.frequency);

	if (!device_is_ready(spi_device)) {
		LOG_ERR("%s is not ready", spi_device->name);
		return -ENODEV;
	}

#if CONFIG_SPI_TEST_JEDEC_ID
	spi_cfg.slave = 0;
	LOG_INF("start spi jedec id test (cs = %d)", spi_cfg.slave);

	ret = read_jedec_id(spi_device);
	if (ret) {
		return ret;
	}

	spi_cfg.slave = 1;
	LOG_INF("start spi jedec id test (cs = %d)", spi_cfg.slave);
	ret = read_jedec_id(spi_device);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi jedec id test");
#endif /* CONFIG_SPI_TEST_JEDEC_ID */

#if CONFIG_SPI_TEST_FREQUENCY
	spi_cfg.slave = 0;
	LOG_INF("start spi frequency test (cs = %d)", spi_cfg.slave);

	for (int i = 0; i <= 7; i++) {
		if (i == 0) {
			spi_cfg.frequency = source_clk;
		} else {
			spi_cfg.frequency = source_clk / (i * 2);
		}
		LOG_INF("frequency %dHz", spi_cfg.frequency);
		ret = read_jedec_id(spi_device);
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(10));
	}
	spi_cfg.frequency = source_clk / 14;
#else
	LOG_WRN("disabled spi frequency test");
#endif /* CONFIG_SPI_TEST_FREQUENCY */

#if CONFIG_SPI_TEST_ASYNC_TRANSFER
	spi_cfg.slave = 1;
	LOG_INF("start spi async transfer test (cs = %d)", spi_cfg.slave);

	if (IS_ENABLED(CONFIG_SPI_ASYNC)) {
		struct k_thread async_thread;
		k_tid_t async_thread_id;

		async_thread_id = k_thread_create(&async_thread, spi_async_stack,
						  CONFIG_SPI_TEST_ASYNC_STACK_SIZE,
						  spi_async_call_cb, &async_evt, &caller,
						  spi_device, K_PRIO_COOP(7), 0, K_NO_WAIT);
		k_thread_name_set(&async_thread, "async-jedec-id");
		ret = read_jedec_id_async(spi_device);
		if (ret) {
			return ret;
		}
	} else {
		LOG_ERR("kconfig SPI_ASYNC is disabled");
	}
#else
	LOG_WRN("disabled spi async transfer test");
#endif /* CONFIG_SPI_TEST_ASYNC_TRANSFER */

#if CONFIG_SPI_TEST_FLASH_ACCESS
	spi_cfg.slave = 0;
	LOG_INF("start spi flash access test (cs = %d)", spi_cfg.slave);

	ret = flash_access_test(128);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi flash access test");
#endif /* CONFIG_SPI_TEST_FLASH_ACCESS */

#if CONFIG_SPI_TEST_DUAL_MODE
	spi_cfg.slave = 1;
	LOG_INF("start spi dual mode test (cs = %d)", spi_cfg.slave);

	spi_cfg.operation &= ~(SPI_LINES_SINGLE | SPI_LINES_QUAD);
	spi_cfg.operation |= SPI_LINES_DUAL;
	ret = dual_mode_test(spi_device);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi dual mode test");
#endif /* CONFIG_SPI_TEST_DUAL_MODE */

#if CONFIG_SPI_TEST_QUAD_MODE
	spi_cfg.slave = 1;
	LOG_INF("start spi quad mode test (cs = %d)", spi_cfg.slave);

	spi_cfg.operation |= SPI_LINES_SINGLE;
	spi_cfg.operation &= ~(SPI_LINES_DUAL | SPI_LINES_QUAD);
	ret = quad_enable(spi_device, true);
	if (ret) {
		return ret;
	}

	spi_cfg.operation |= SPI_LINES_QUAD;
	spi_cfg.operation &= ~(SPI_LINES_SINGLE | SPI_LINES_DUAL);
	ret = quad_mode_test(spi_device);
	if (ret) {
		return ret;
	}

	spi_cfg.operation |= SPI_LINES_SINGLE;
	spi_cfg.operation &= ~(SPI_LINES_DUAL | SPI_LINES_QUAD);
	ret = quad_enable(spi_device, false);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi quad mode test");
#endif /* CONFIG_SPI_TEST_QUAD_MODE */

#if CONFIG_SPI_TEST_LINE_MODE
	spi_cfg.slave = 0;
	LOG_INF("start spi line mode test (cs = %d)", spi_cfg.slave);

	spi_cfg.operation |= (SPI_MODE_CPHA | SPI_MODE_CPOL);
	ret = read_jedec_id(spi_device);
	if (ret) {
		LOG_ERR("failed to read jedec id, ret %d", ret);
		return ret;
	}

	/* for IT51xxx test, driver need to switch to pio mode,
	 * since fifo mode is supported only under spi mode 0.
	 */
	// uint8_t line_mode_4_tx_data[4] = {0x3, 0x0, 0x0, 0x10};
	// uint8_t line_mode_rx_data[8] = {0};

	// ret = test_tx_rx(spi_device, line_mode_4_tx_data, sizeof(line_mode_4_tx_data),
	// 		 line_mode_rx_data, 8);
	// if (ret) {
	// 	return ret;
	// }

	spi_cfg.operation &= ~(SPI_MODE_CPHA | SPI_MODE_CPOL);
#else
	LOG_WRN("disabled spi line mode test");
#endif /* CONFIG_SPI_TEST_LINE_MODE */

#if CONFIG_SPI_TEST_RX_ONLY
	spi_cfg.slave = 0;
	LOG_INF("start spi rx only test (cs = %d)", spi_cfg.slave);

	uint8_t rx_3_rx_data[3] = {0x0, 0x1, 0x2};
	uint8_t rx_4_rx_data[4] = {0x3, 0x4, 0x5, 0x6};
	uint8_t rx_8_rx_data[8] = {0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe};
	ret = test_rx_only(spi_device, rx_3_rx_data, sizeof(rx_3_rx_data));
	if (ret) {
		return ret;
	}

	ret = test_rx_only(spi_device, rx_8_rx_data, sizeof(rx_8_rx_data));
	if (ret) {
		return ret;
	}

	ret = test_rx_only(spi_device, rx_4_rx_data, sizeof(rx_4_rx_data));
	if (ret) {
		return ret;
	}

	ret = test_multi_rx(spi_device);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi rx only test");
#endif /* CONFIG_SPI_TEST_RX_ONLY */

#if CONFIG_SPI_TEST_TX_ONLY
	spi_cfg.slave = 0;
	LOG_INF("start spi tx only test (cs = %d)", spi_cfg.slave);

	uint8_t tx_3_tx_data[3] = {0x0, 0x1, 0x2};
	uint8_t tx_4_tx_data[4] = {0x3, 0x4, 0x5, 0x6};
	uint8_t tx_8_tx_data[8] = {0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe};
	ret = test_tx_only(spi_device, tx_3_tx_data, sizeof(tx_3_tx_data));
	if (ret) {
		return ret;
	}

	ret = test_tx_only(spi_device, tx_4_tx_data, sizeof(tx_4_tx_data));
	if (ret) {
		return ret;
	}

	ret = test_tx_only(spi_device, tx_8_tx_data, sizeof(tx_8_tx_data));
	if (ret) {
		return ret;
	}

	ret = test_multi_tx(spi_device);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi tx only test");
#endif /* CONFIG_SPI_TEST_TX_ONLY */

#if CONFIG_SPI_TEST_TX_RX
	spi_cfg.frequency = MHZ(12);
	spi_cfg.slave = 0;
	LOG_INF("start spi tx-then-rx test (cs = %d)", spi_cfg.slave);

	uint8_t txrx_3_tx_data[3] = {0x0, 0x1, 0x2};
	uint8_t txrx_4_tx_data[4] = {0x3, 0x0, 0x0, 0x10};
	uint8_t txrx_8_tx_data[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
	uint8_t rx_data[8] = {0};
	ret = test_tx_rx(spi_device, txrx_3_tx_data, sizeof(txrx_3_tx_data), rx_data, 3);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_3_tx_data, sizeof(txrx_3_tx_data), rx_data, 8);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_3_tx_data, sizeof(txrx_3_tx_data), rx_data, 4);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_8_tx_data, sizeof(txrx_8_tx_data), rx_data, 8);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_8_tx_data, sizeof(txrx_8_tx_data), rx_data, 4);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_4_tx_data, sizeof(txrx_4_tx_data), rx_data, 8);
	if (ret) {
		return ret;
	}

	ret = test_tx_rx(spi_device, txrx_4_tx_data, sizeof(txrx_4_tx_data), rx_data, 6);
	if (ret) {
		return ret;
	}
#else
	LOG_WRN("disabled spi tx-then-rx test");
#endif /* CONFIG_SPI_TEST_TX_RX */

#if CONFIG_SPI_TEST_PM_CHECK
	LOG_INF("start spi pm check test");
	start_pm_check = true;
#else
	LOG_WRN("disabled spi pm check test");
#endif /* CONFIG_SPI_TEST_PM_CHECK */

	return 0;
}
