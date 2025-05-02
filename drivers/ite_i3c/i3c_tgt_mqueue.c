/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_i3c_tgt_mqueue

#include <zephyr/drivers/i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tgt_mqueue, LOG_LEVEL_INF);

struct i3c_tgt_mqueue_config {
	void (*make_thread)(const struct device *dev);
	void (*test_ibi_thread)(const struct device *dev);
};

struct i3c_tgt_mqueue_data {
	const struct device *controller;

	struct i3c_target_config target_config;

	struct k_thread thread_data;

	struct {
		bool start;
		int test_cnt;
		int tx_cnt;
	} ibi_test;

	struct k_thread ibi_thread_data;

	struct {
		struct {
			uint8_t data[CONFIG_I3CS_IT51XXX_RX_FIFO_SIZE];
			size_t len;
		} rx;

		struct {
			uint8_t data[CONFIG_I3CS_IT51XXX_TX_FIFO_SIZE];
		} tx;
	} buffer;
};

enum event_table {
	priv_read_evt = 0,
	priv_write_evt
};

enum opcode_table {
	/* Qualcomm loopback test: place payload into tx fifo after reception */
	QCOM_LOOPBACK_OPCODE = 0x3C,
	/* ITE IBI Test: preare tx data and issue ibi */
	ITE_IBI_TEST_OPCODE = 0xBE,
	/* ITE Loopback Test: trigger an ibi after the payload has been received */
	ITE_LOOPBACK_OPCODE = 0xBF,
};

K_MSGQ_DEFINE(evt_msgq, sizeof(uint8_t), CONFIG_I3C_TGT_MQUEUE_EVENT_COUNT, sizeof(uint8_t));

static int prepare_tx_fifo(struct i3c_tgt_mqueue_data *data, size_t length)
{
	int ret;

	if (length == 0) {
		length = sizeof(data->buffer.tx.data);
		for (size_t i = 0; i < length; i++) {
			data->buffer.tx.data[i] = i;
		}
	}

	ret = i3c_target_tx_write(data->controller, data->buffer.tx.data, length, 0);
	if (ret < 0) {
		// LOG_ERR("%s: failed to write data to tx fifo, ret %d", data->controller->name,
		// ret);
		return ret;
	}
	return 0;
}

#ifdef CONFIG_I3C_USE_IBI
static int raise_ibi_hj(const struct device *dev)
{
	struct i3c_ibi request;
	int ret;

	request.ibi_type = I3C_IBI_HOTJOIN;

	ret = i3c_ibi_raise(dev, &request);
	if (ret != 0) {
		LOG_ERR("failed to issue ibi hj");
		return ret;
	}
	return 0;
}

static int raise_ibi_tir(const struct device *dev, uint8_t *buf, const uint16_t data_length)
{
	struct i3c_ibi request;
	int ret;

	request.ibi_type = I3C_IBI_TARGET_INTR;
	request.payload = buf;
	request.payload_len = data_length;

	ret = i3c_ibi_raise(dev, &request);
	if (ret != 0) {
		// LOG_ERR("unable to issue ibi tir");
		return ret;
	}
	return 0;
}
#else
static int raise_ibi_hj(const struct device *dev)
{
	LOG_ERR("KConfig IBI is disabled");
	return -ENOTSUP;
}

static int raise_ibi_tir(const struct device *dev, uint8_t *buf, const uint16_t data_length)
{
	LOG_ERR("KConfig IBI is disabled");
	return -ENOTSUP;
}
#endif /* CONFIG_I3C_USE_IBI */

static void tgt_mqueue_ibi_handler(const struct device *dev)
{
	struct i3c_tgt_mqueue_data *data = dev->data;

	while (true) {
		if (data->ibi_test.start) {
			uint8_t buf[3] = {1, 2, 3};
			uint8_t mdb = 0x1;
			int retries = 5, ret;

			memcpy(data->buffer.tx.data, buf, sizeof(buf));
			ret = prepare_tx_fifo(data, sizeof(buf));
			if (ret) {
				goto next;
			}

			for (int i = 0; i <= retries; i++) {
				ret = raise_ibi_tir(data->controller, &mdb, sizeof(mdb));
				if (ret != -EBUSY) {
					break;
				}
				if (i == retries) {
					LOG_ERR("bus is always busy(retry times: %d)", retries);
					return;
				}
			}

			if (ret == -EOVERFLOW) {
				goto next;
			}

			data->ibi_test.test_cnt++;
			if (data->ibi_test.test_cnt >= 1000) {
				LOG_INF("finished to issue %d ibis", data->ibi_test.test_cnt);
				data->ibi_test.start = false;
			}
		}
next:
		k_sleep(K_MSEC(1));
	}
}

static void tgt_mqueue_handler(const struct device *dev)
{
	struct i3c_tgt_mqueue_data *data = dev->data;

	while (true) {
		uint8_t evt;

		k_msgq_get(&evt_msgq, &evt, K_FOREVER);

		if (evt == priv_write_evt) {
			data->ibi_test.tx_cnt = 0;
			data->ibi_test.test_cnt = 0;
			data->ibi_test.start = false;

			LOG_INF("received %d bytes data", data->buffer.rx.len);
			LOG_HEXDUMP_DBG(data->buffer.rx.data, data->buffer.rx.len, "data:");
			switch (data->buffer.rx.data[0]) {
			case QCOM_LOOPBACK_OPCODE:
				memcpy(data->buffer.tx.data, data->buffer.rx.data,
				       data->buffer.rx.len);
				prepare_tx_fifo(data, data->buffer.rx.len);
				break;
			case ITE_LOOPBACK_OPCODE:
				/* drop the first byte(BFh) */
				raise_ibi_tir(data->controller, data->buffer.rx.data + 1,
					      data->buffer.rx.len - 1);
				break;
			case ITE_IBI_TEST_OPCODE:
				data->ibi_test.start = true;
			default:
				break;
			}
			data->buffer.rx.len = 0;
		}

		if (evt == priv_read_evt) {
			if (data->ibi_test.start || data->ibi_test.tx_cnt) {
				data->ibi_test.tx_cnt++;
				LOG_INF("%dth private read transfer", data->ibi_test.tx_cnt);
			} else {
				LOG_INF("finished tx data");
			}

#if 0
			prepare_tx_fifo(data, 0);
#endif
		}
	}
}

static void i3c_tgt_mq_buf_write_received_cb(struct i3c_target_config *config, uint8_t *ptr,
					     uint32_t len)
{
	struct i3c_tgt_mqueue_data *data =
		CONTAINER_OF(config, struct i3c_tgt_mqueue_data, target_config);
	uint8_t evt;

	data->buffer.rx.len = len;

	if (sizeof(data->buffer.rx.data) < len) {
		data->buffer.rx.len = sizeof(data->buffer.rx.data);
		LOG_WRN("received data(%d) is too much, only copy %d bytes", len,
			data->buffer.rx.len);
	}
	memcpy(data->buffer.rx.data, ptr, data->buffer.rx.len);

	evt = priv_write_evt;
	k_msgq_put(&evt_msgq, &evt, K_NO_WAIT);
}

static int i3c_tgt_mq_buf_read_requested_cb(struct i3c_target_config *config, uint8_t **ptr,
					    uint32_t *len, uint8_t *hdr_mode)
{
	ARG_UNUSED(config);
	ARG_UNUSED(ptr);
	ARG_UNUSED(len);
	ARG_UNUSED(hdr_mode);

	uint8_t evt;

	/* private read transfer is finished */
	evt = priv_read_evt;
	k_msgq_put(&evt_msgq, &evt, K_NO_WAIT);
	return 0;
}

static int i3c_tgt_mq_stop_cb(struct i3c_target_config *config)
{
	ARG_UNUSED(config);

	return 0;
}

static const struct i3c_target_callbacks i3c_target_callbacks = {
	.buf_write_received_cb = i3c_tgt_mq_buf_write_received_cb,
	.buf_read_requested_cb = i3c_tgt_mq_buf_read_requested_cb,
	.stop_cb = i3c_tgt_mq_stop_cb,
};

static int i3c_tgt_mqueue_init(const struct device *dev)
{
	const struct i3c_tgt_mqueue_config *config = dev->config;
	struct i3c_tgt_mqueue_data *data = dev->data;
	int ret;

	data->target_config.callbacks = &i3c_target_callbacks;

	ret = i3c_target_register(data->controller, &data->target_config);
	if (ret) {
		LOG_ERR("failed to register %s, ret %d", dev->name, ret);
		return ret;
	}

	config->make_thread(dev);
	config->test_ibi_thread(dev);
	LOG_INF("registered %s on %s controller", dev->name, data->controller->name);

	raise_ibi_hj(data->controller);

#if 0
	prepare_tx_fifo(data, 0);
#endif

	return 0;
}

#define MAX_NAME_LEN sizeof("ibi_test[xx]")

#define I3C_TARGET_MQUEUE_INIT(n)                                                                  \
                                                                                                   \
	K_KERNEL_STACK_DEFINE(i3c_tgt_mqueue_stack_##n, CONFIG_I3C_TGT_MQUEUE_STACK_SIZE);         \
	static void i3c_tgt_mqueue_thread_##n(void *dev, void *arg1, void *arg2)                   \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		tgt_mqueue_handler(dev);                                                           \
	}                                                                                          \
                                                                                                   \
	static void i3c_tgt_mqueue_make_thread_##n(const struct device *dev)                       \
	{                                                                                          \
		struct i3c_tgt_mqueue_data *data = dev->data;                                      \
                                                                                                   \
		k_thread_create(&data->thread_data, i3c_tgt_mqueue_stack_##n,                      \
				K_THREAD_STACK_SIZEOF(i3c_tgt_mqueue_stack_##n),                   \
				i3c_tgt_mqueue_thread_##n, (void *)dev, NULL, NULL,                \
				K_PRIO_PREEMPT(CONFIG_I3C_TGT_MQUEUE_THREAD_PRIORITY), 0,          \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&data->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	K_KERNEL_STACK_DEFINE(i3c_tgt_mqueue_ibi_stack_##n, CONFIG_I3C_TGT_MQUEUE_STACK_SIZE);     \
	static void i3c_tgt_mqueue_ibi_thread_##n(void *dev, void *arg1, void *arg2)               \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		tgt_mqueue_ibi_handler(dev);                                                       \
	}                                                                                          \
                                                                                                   \
	static void i3c_tgt_mqueue_test_ibi_thread_##n(const struct device *dev)                   \
	{                                                                                          \
		char name[MAX_NAME_LEN];                                                           \
		struct i3c_tgt_mqueue_data *data = dev->data;                                      \
                                                                                                   \
		k_thread_create(&data->ibi_thread_data, i3c_tgt_mqueue_ibi_stack_##n,              \
				K_THREAD_STACK_SIZEOF(i3c_tgt_mqueue_ibi_stack_##n),               \
				i3c_tgt_mqueue_ibi_thread_##n, (void *)dev, NULL, NULL,            \
				K_PRIO_PREEMPT(CONFIG_I3C_TGT_MQUEUE_THREAD_PRIORITY), 0,          \
				K_NO_WAIT);                                                        \
		snprintk(name, sizeof(name), "ibi_test[%02d]", n);                                 \
		k_thread_name_set(&data->ibi_thread_data, name);                                   \
	}                                                                                          \
                                                                                                   \
	static const struct i3c_tgt_mqueue_config i3c_tgt_mqueue_config_##n = {                    \
		.make_thread = i3c_tgt_mqueue_make_thread_##n,                                     \
		.test_ibi_thread = i3c_tgt_mqueue_test_ibi_thread_##n,                             \
	};                                                                                         \
                                                                                                   \
	static struct i3c_tgt_mqueue_data i3c_tgt_mqueue_data_##n = {                              \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, i3c_tgt_mqueue_init, NULL, &i3c_tgt_mqueue_data_##n,              \
			      &i3c_tgt_mqueue_config_##n, POST_KERNEL,                             \
			      CONFIG_ITE_I3C_SAMPLE_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(I3C_TARGET_MQUEUE_INIT)
