#include <test_framework/test_framework.h>
/**
 * @file test_hal_can.c
 * @brief HAL CAN驱动单元测试
 */

#include "hal.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: CAN初始化 - 成功 */
static void _test_hal_can_init_success(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);

	/* 如果初始化失败，跳过所有CAN测试 */
	TEST_ASSERT_FALSE(ret !=
					  OSAL_SUCCESS); // CAN interface not available or down.
	// Please run: ip link set can0 up

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	/* 尝试发送一个测试帧，检查接口是否真正可用 */
	hal_can_frame_t test_frame = { .can_id = 0x001,
								   .dlc = 1,
								   .data = { 0x00 } };
	ret = hal_can_send(handle, &test_frame);

	/* 如果发送失败（Network is down），跳过所有CAN测试 */
	if (ret != OSAL_SUCCESS) {
		hal_can_deinit(handle);
		TEST_ASSERT_FALSE(
			true); // CAN interface is down. Please run: ip link set can0 type
		// can bitrate 500000 && ip link set can0 up
	}

	hal_can_deinit(handle);
}

/* 测试用例: CAN初始化 - 空配置 */
static void _test_hal_can_init_null_config(void)
{
	hal_can_handle_t handle = NULL;

	int32_t ret = hal_can_init(NULL, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN初始化 - 空句柄 */
static void _test_hal_can_init_null_handle(void)
{
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN初始化 - 无效接口 */
static void _test_hal_can_init_invalid_interface(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "invalid_can999",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN清理 */
static void _test_hal_can_deinit(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_deinit(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN清理 - 空句柄 */
static void _test_hal_can_deinit_null_handle(void)
{
	int32_t ret = hal_can_deinit(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 发送和接收测试
 *===========================================================================*/

/* 测试用例: CAN发送 - 成功 */
static void _test_hal_can_send_success(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };
	hal_can_frame_t frame = { .can_id = 0x123,
							  .dlc = 8,
							  .data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
										0x07, 0x08 } };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_send(handle, &frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN发送 - 空句柄 */
static void _test_hal_can_send_null_handle(void)
{
	hal_can_frame_t frame = { .can_id = 0x123,
							  .dlc = 8,
							  .data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
										0x07, 0x08 } };

	int32_t ret = hal_can_send(NULL, &frame);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN发送 - 空帧 */
static void _test_hal_can_send_null_frame(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_send(handle, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN接收 - 超时 */
static void _test_hal_can_recv_timeout(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 100,
								.tx_timeout = 1000 };
	hal_can_frame_t frame;

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_recv(handle, &frame, 100);
	TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN接收 - 空句柄 */
static void _test_hal_can_recv_null_handle(void)
{
	hal_can_frame_t frame;

	int32_t ret = hal_can_recv(NULL, &frame, 100);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN接收 - 空帧 */
static void _test_hal_can_recv_null_frame(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_recv(handle, NULL, 100);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN发送接收回环 */
static void _test_hal_can_loopback(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };
	hal_can_frame_t tx_frame = { .can_id = 0x456,
								 .dlc = 4,
								 .data = { 0xAA, 0xBB, 0xCC, 0xDD } };
	hal_can_frame_t rx_frame;

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 发送帧 */
	ret = hal_can_send(handle, &tx_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 接收帧（需要外部回环或另一个CAN节点） */
	ret = hal_can_recv(handle, &rx_frame, 1000);
	if (OSAL_SUCCESS == ret) {
		TEST_ASSERT_EQUAL(tx_frame.can_id, rx_frame.can_id);
		TEST_ASSERT_EQUAL(tx_frame.dlc, rx_frame.dlc);
		int32_t i;

		for (i = 0; i < tx_frame.dlc; i++) {
			TEST_ASSERT_EQUAL(tx_frame.data[i], rx_frame.data[i]);
		}
	}

	hal_can_deinit(handle);
}

/*===========================================================================
 * 过滤器测试
 *===========================================================================*/

/* 测试用例: 设置过滤器 - 成功 */
static void _test_hal_can_set_filter_success(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = hal_can_set_filter(handle, 0x100, 0x7FF);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: 设置过滤器 - 空句柄 */
static void _test_hal_can_set_filter_null_handle(void)
{
	int32_t ret = hal_can_set_filter(NULL, 0x100, 0x7FF);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 统计信息测试
 *===========================================================================*/

/* 测试用例: 获取统计信息 - 成功 */

/*===========================================================================
 * 配置参数测试
 *===========================================================================*/

/* 测试用例: 不同波特率 */
static void _test_hal_can_different_baudrate(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 250000, /* 250kbps */
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/*===========================================================================
 * 独立发送测试
 *===========================================================================*/

/* 测试用例: CAN仅发送 - 标准帧 */
static void _test_can_send_only(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	/* 初始化CAN */
	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	/* 测试标准帧发送 */
	hal_can_frame_t std_frame = { .can_id = 0x123,
								  .dlc = 8,
								  .data = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
											0x77, 0x88 } };
	ret = hal_can_send(handle, &std_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试扩展帧发送 (CAN_EFF_FLAG = 0x80000000) */
	hal_can_frame_t ext_frame = {
		.can_id = 0x80012345, /* Extended ID with EFF flag */
		.dlc = 6,
		.data = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 }
	};
	ret = hal_can_send(handle, &ext_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试RTR帧发送 (CAN_RTR_FLAG = 0x40000000) */
	hal_can_frame_t rtr_frame = { .can_id = 0x40000456, /* RTR frame */
								  .dlc = 0,
								  .data = { 0 } };
	ret = hal_can_send(handle, &rtr_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试不同数据长度 */
	hal_can_frame_t short_frame = { .can_id = 0x200,
									.dlc = 2,
									.data = { 0x12, 0x34 } };
	ret = hal_can_send(handle, &short_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试多帧连续发送 */
	int32_t i;
	for (i = 0; i < 10; i++) {
		hal_can_frame_t burst_frame = { .can_id = 0x300 + i,
										.dlc = 4,
										.data = { (uint8_t)i, (uint8_t)(i + 1),
												  (uint8_t)(i + 2),
												  (uint8_t)(i + 3) } };
		ret = hal_can_send(handle, &burst_frame);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	/* 清理 */
	ret = hal_can_deinit(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 独立接收测试
 *===========================================================================*/

/* 测试用例: CAN仅接收 - 带过滤器 */
static void _test_can_receive_only(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	/* 初始化CAN */
	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	/* 设置过滤器 - 只接收ID 0x100-0x1FF的帧 */
	ret = hal_can_set_filter(handle, 0x100, 0x700);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 尝试接收多个帧（带超时） */
	hal_can_frame_t rx_frame;
	int32_t received_count = 0;
	int32_t i;

	for (i = 0; i < 5; i++) {
		ret = hal_can_recv(handle, &rx_frame, 500);
		if (ret == OSAL_SUCCESS) {
			received_count++;

			/* 验证接收到的帧符合过滤器规则 */
			TEST_ASSERT_TRUE((rx_frame.can_id & 0x700) == 0x100);

			/* 验证数据长度有效 */
			TEST_ASSERT_TRUE(rx_frame.dlc <= 8);
		} else if (ret == OSAL_ERR_TIMEOUT) {
			/* 超时是正常的（没有外部发送者） */
			break;
		} else {
			/* 其他错误 */
			TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		}
	}

	/* 测试长超时接收（可能超时） */
	ret = hal_can_recv(handle, &rx_frame, 1000);
	if (ret == OSAL_SUCCESS) {
		/* 如果接收到帧，验证数据 */
		TEST_ASSERT_TRUE(rx_frame.dlc <= 8);
		received_count++;
	} else {
		/* 超时是预期的（除非有外部CAN流量） */
		TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);
	}

	/* 测试短超时接收 */
	ret = hal_can_recv(handle, &rx_frame, 100);
	if (ret != OSAL_SUCCESS) {
		TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);
	}

	/* 清理 */
	ret = hal_can_deinit(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 边界值和扩展测试
 *===========================================================================*/

/* 测试用例: CAN最大数据长度 */
static void _test_hal_can_max_data_length(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 发送最大长度帧 (DLC=8) */
	hal_can_frame_t frame = { .can_id = 0x123,
							  .dlc = 8,
							  .data = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
										0x77, 0x88 } };

	ret = hal_can_send(handle, &frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN最小数据长度 */
static void _test_hal_can_min_data_length(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 发送零长度帧 (DLC=0) */
	hal_can_frame_t frame = { .can_id = 0x456, .dlc = 0, .data = { 0 } };

	ret = hal_can_send(handle, &frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN扩展帧ID */
static void _test_hal_can_extended_id(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 发送扩展帧 (29位ID, 扩展帧标志 0x80000000) */
	hal_can_frame_t frame = {
		.can_id = 0x12345678 | 0x80000000U, /* CAN_EFF_FLAG = 0x80000000 */
		.dlc = 4,
		.data = { 0xAA, 0xBB, 0xCC, 0xDD }
	};

	ret = hal_can_send(handle, &frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN标准帧ID边界 */
static void _test_hal_can_standard_id_boundary(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 最小标准ID (0x000) */
	hal_can_frame_t frame1 = { .can_id = 0x000, .dlc = 1, .data = { 0x01 } };
	ret = hal_can_send(handle, &frame1);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 最大标准ID (0x7FF) */
	hal_can_frame_t frame2 = { .can_id = 0x7FF, .dlc = 1, .data = { 0x02 } };
	ret = hal_can_send(handle, &frame2);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN不同波特率 */
static void _test_hal_can_various_baudrates(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 250000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	/* 测试250kbps */
	int32_t ret = hal_can_init(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_can_deinit(handle);
	}

	/* 测试1Mbps */
	config.baudrate = 1000000;
	ret = hal_can_init(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_can_deinit(handle);
	}

	/* 测试125kbps */
	config.baudrate = 125000;
	ret = hal_can_init(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_can_deinit(handle);
	}
}

/* 测试用例: CAN连续发送多帧 */
static void _test_hal_can_burst_send(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 连续发送100帧 */
	for (uint32_t i = 0; i < 100; i++) {
		hal_can_frame_t frame = {
			.can_id = 0x200 + (i % 10),
			.dlc = 8,
			.data = { (uint8_t)i, (uint8_t)(i + 1), (uint8_t)(i + 2),
					  (uint8_t)(i + 3), (uint8_t)(i + 4), (uint8_t)(i + 5),
					  (uint8_t)(i + 6), (uint8_t)(i + 7) }
		};

		ret = hal_can_send(handle, &frame);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	hal_can_deinit(handle);
}

/* 测试用例: CAN过滤器边界值 */
static void _test_hal_can_filter_boundary(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = { .interface = "can0",
								.baudrate = 500000,
								.rx_timeout = 1000,
								.tx_timeout = 1000 };

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试接收所有帧 (mask=0x000) */
	ret = hal_can_set_filter(handle, 0x000, 0x000);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 测试仅接收精确ID (mask=0x7FF) */
	ret = hal_can_set_filter(handle, 0x123, 0x7FF);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_deinit(handle);
}

/* 测试用例: CAN零超时发送 */
static void _test_hal_can_zero_timeout_send(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = {
		.interface = "can0",
		.baudrate = 500000,
		.rx_timeout = 1000,
		.tx_timeout = 0 /* 零超时 */
	};

	int32_t ret = hal_can_init(&config, &handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_can_frame_t frame = { .can_id = 0x555,
							  .dlc = 4,
							  .data = { 0x12, 0x34, 0x56, 0x78 } };

	ret = hal_can_send(handle, &frame);
	/* 零超时可能成功或超时 */
	TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret == OSAL_ERR_TIMEOUT);

	hal_can_deinit(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// HAL CAN驱动测试
/* 初始化和清理 */
/* 发送和接收 */
/* 过滤器 */
/* 统计信息 */
//     //     //     //     /* 配置参数 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_hal_can_init_success",
	  .func = _test_hal_can_init_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_init_null_config",
	  .func = _test_hal_can_init_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_init_null_handle",
	  .func = _test_hal_can_init_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_init_invalid_interface",
	  .func = _test_hal_can_init_invalid_interface,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_deinit",
	  .func = _test_hal_can_deinit,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_deinit_null_handle",
	  .func = _test_hal_can_deinit_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_send_success",
	  .func = _test_hal_can_send_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_send_null_handle",
	  .func = _test_hal_can_send_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_send_null_frame",
	  .func = _test_hal_can_send_null_frame,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_recv_timeout",
	  .func = _test_hal_can_recv_timeout,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_recv_null_handle",
	  .func = _test_hal_can_recv_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_recv_null_frame",
	  .func = _test_hal_can_recv_null_frame,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_loopback",
	  .func = _test_hal_can_loopback,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_set_filter_success",
	  .func = _test_hal_can_set_filter_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_set_filter_null_handle",
	  .func = _test_hal_can_set_filter_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_different_baudrate",
	  .func = _test_hal_can_different_baudrate,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_can_send_only",
	  .func = _test_can_send_only,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_can_receive_only",
	  .func = _test_can_receive_only,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_max_data_length",
	  .func = _test_hal_can_max_data_length,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_min_data_length",
	  .func = _test_hal_can_min_data_length,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_extended_id",
	  .func = _test_hal_can_extended_id,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_standard_id_boundary",
	  .func = _test_hal_can_standard_id_boundary,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_various_baudrates",
	  .func = _test_hal_can_various_baudrates,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_burst_send",
	  .func = _test_hal_can_burst_send,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_filter_boundary",
	  .func = _test_hal_can_filter_boundary,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_can_zero_timeout_send",
	  .func = _test_hal_can_zero_timeout_send,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_can",
	.module_name = "hal_can",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "HAL hal_can tests" }
};

/* 测试套件注册函数 */
void register_hal_can_tests(void)
{
	libutest_register_suite(&test_suite);
}
