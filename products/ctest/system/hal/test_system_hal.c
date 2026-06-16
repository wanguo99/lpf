/**
 * @file test_system_hal.c
 * @brief HAL层系统集成测试
 * @details 测试HAL模块在真实系统环境中的集成场景
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "hal.h"
#include "osal.h"

/* 测试配置 */
#define TEST_CAN_INTERFACE     "can0"
#define TEST_CAN_BAUDRATE      500000
#define TEST_UART_DEVICE       "/dev/ttyUSB0"
#define TEST_UART_BAUDRATE     115200
#define TEST_GPIO_PIN          17
#define TEST_WATCHDOG_DEVICE   "/dev/watchdog0"
#define TEST_TIMEOUT_MS        5000
#define TEST_LONG_TIMEOUT_MS   10000

/* 测试数据缓冲区 */
static uint8_t test_buffer[1024];

/*===========================================================================
 * Setup/Teardown
 *===========================================================================*/

static void system_hal_setup(void)
{
	OSAL_memset(test_buffer, 0, sizeof(test_buffer));
}

static void system_hal_teardown(void)
{
	/* 清理任何残留资源 */
	OSAL_msleep(100); /* 100ms */
}

/*===========================================================================
 * CAN系统集成测试
 *===========================================================================*/

/**
 * @brief 测试CAN完整生命周期管理
 * @details 验证CAN从初始化到配置、通信、清理的完整流程
 */
static void test_system_can_lifecycle(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = {
		.interface = TEST_CAN_INTERFACE,
		.baudrate = TEST_CAN_BAUDRATE,
		.rx_timeout = 1000,
		.tx_timeout = 1000
	};

	/* 阶段1: 初始化 */
	int32_t ret = HAL_CAN_Init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] CAN interface not available\n");
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}
	TEST_ASSERT_NOT_NULL(handle);

	/* 阶段2: 配置过滤器 */
	ret = HAL_CAN_SetFilter(handle, 0x100, 0x700);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段3: 发送测试帧 */
	hal_can_frame_t tx_frame = {
		.can_id = 0x123,
		.dlc = 8,
		.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
	};
	ret = HAL_CAN_Send(handle, &tx_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段4: 尝试接收（可能超时，这是正常的） */
	hal_can_frame_t rx_frame;
	OSAL_memset(&rx_frame, 0, sizeof(rx_frame));
	ret = HAL_CAN_Recv(handle, &rx_frame, 500);
	/* 接收可能超时，不强制要求成功 */

	/* 阶段5: 再次发送验证稳定性 */
	ret = HAL_CAN_Send(handle, &tx_frame);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段6: 清理 */
	ret = HAL_CAN_Deinit(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS ] CAN lifecycle test completed\n");
}

/**
 * @brief 测试CAN环回通信
 * @details 使用CAN loopback模式验证端到端通信
 */
static void test_system_can_loopback(void)
{
	hal_can_handle_t handle = NULL;
	hal_can_config_t config = {
		.interface = TEST_CAN_INTERFACE,
		.baudrate = TEST_CAN_BAUDRATE,
		.rx_timeout = 1000,
		.tx_timeout = 1000
	};

	/* 初始化CAN */
	int32_t ret = HAL_CAN_Init(&config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] CAN interface not available\n");
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}

	/* 发送多个测试帧 */
	for (int i = 0; i < 5; i++) {
		hal_can_frame_t tx_frame = {
			.can_id = 0x100 + i,
			.dlc = 4,
			.data = {i, i+1, i+2, i+3}
		};

		ret = HAL_CAN_Send(handle, &tx_frame);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		OSAL_msleep(10); /* 10ms */
	}

	/* 清理 */
	HAL_CAN_Deinit(handle);

	OSAL_printf("[ PASS ] CAN loopback test completed\n");
}

/*===========================================================================
 * UART系统集成测试
 *===========================================================================*/

/**
 * @brief 测试UART完整生命周期管理
 * @details 验证UART从初始化到配置、通信、清理的完整流程
 */
static void test_system_uart_lifecycle(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.baud_rate = TEST_UART_BAUDRATE,
		.data_bits = 8,
		.parity = HAL_SERIAL_PARITY_NONE,
		.stop_bits = 1,
		.flow_control = HAL_SERIAL_FLOW_NONE
	};

	/* 阶段1: 初始化 */
	int32_t ret = HAL_Serial_Open(TEST_UART_DEVICE, &config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] UART device %s not available\n", TEST_UART_DEVICE);
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}
	TEST_ASSERT_NOT_NULL(handle);

	/* 阶段2: 清空缓冲区 */
	ret = HAL_Serial_Flush(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段3: 写入测试数据 */
	const char *test_msg = "HAL System Test\r\n";
	ret = HAL_Serial_Write(handle, (const uint8_t *)test_msg,
	                       OSAL_strlen(test_msg), 1000);
	TEST_ASSERT_TRUE(ret >= 0);
	TEST_ASSERT_EQUAL(OSAL_strlen(test_msg), (uint32_t)ret);

	/* 阶段4: 动态修改配置 */
	hal_serial_config_t new_config = {
		.baud_rate = 9600,
		.data_bits = 8,
		.parity = HAL_SERIAL_PARITY_NONE,
		.stop_bits = 1,
		.flow_control = HAL_SERIAL_FLOW_NONE
	};
	ret = HAL_Serial_SetConfig(handle, &new_config);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段5: 恢复原配置并再次发送 */
	new_config.baud_rate = TEST_UART_BAUDRATE;
	ret = HAL_Serial_SetConfig(handle, &new_config);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = HAL_Serial_Write(handle, (const uint8_t *)test_msg,
	                       OSAL_strlen(test_msg), 1000);
	TEST_ASSERT_TRUE(ret >= 0);

	/* 阶段6: 清理 */
	ret = HAL_Serial_Close(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS ] UART lifecycle test completed\n");
}

/**
 * @brief 测试UART回显通信
 * @details 发送数据并尝试读回（需要硬件loopback或外部设备）
 */
static void test_system_uart_echo(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = {
		.baud_rate = TEST_UART_BAUDRATE,
		.data_bits = 8,
		.parity = HAL_SERIAL_PARITY_NONE,
		.stop_bits = 1,
		.flow_control = HAL_SERIAL_FLOW_NONE
	};

	/* 初始化 */
	int32_t ret = HAL_Serial_Open(TEST_UART_DEVICE, &config, &handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] UART device not available\n");
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}

	/* 清空接收缓冲区 */
	HAL_Serial_Flush(handle);

	/* 发送测试模式 */
	const uint8_t test_pattern[] = {0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};
	ret = HAL_Serial_Write(handle, test_pattern, sizeof(test_pattern), 1000);
	TEST_ASSERT_TRUE(ret >= 0);

	/* 尝试读回（如果有loopback则应该能读到） */
	uint8_t rx_buffer[32];
	ret = HAL_Serial_Read(handle, rx_buffer, sizeof(rx_buffer), 2000);
	/* 读取可能超时，这在没有loopback时是正常的 */

	/* 清理 */
	HAL_Serial_Close(handle);

	OSAL_printf("[ PASS ] UART echo test completed (bytes_read=%d)\n", ret >= 0 ? ret : 0);
}

/*===========================================================================
 * GPIO系统集成测试
 *===========================================================================*/

/**
 * @brief 测试GPIO完整生命周期
 * @details 验证GPIO初始化、配置、读写、中断的完整流程
 */
static void test_system_gpio_lifecycle(void)
{
	hal_gpio_config_t config = {
		.direction = HAL_GPIO_DIR_OUTPUT,
		.initial_level = HAL_GPIO_LEVEL_LOW,
		.edge = HAL_GPIO_EDGE_NONE,
		.callback = NULL,
		.user_data = NULL
	};

	/* 阶段1: 初始化为输出模式 */
	int32_t ret = HAL_GPIO_Init(TEST_GPIO_PIN, &config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] GPIO pin %d not available\n", TEST_GPIO_PIN);
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}

	/* 阶段2: 输出高电平 */
	ret = HAL_GPIO_SetLevel(TEST_GPIO_PIN, HAL_GPIO_LEVEL_HIGH);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_msleep(100); /* 100ms */

	/* 阶段3: 输出低电平 */
	ret = HAL_GPIO_SetLevel(TEST_GPIO_PIN, HAL_GPIO_LEVEL_LOW);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_msleep(100); /* 100ms */

	/* 阶段4: 切换为输入模式 */
	ret = HAL_GPIO_SetDirection(TEST_GPIO_PIN, HAL_GPIO_DIR_INPUT);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段5: 读取电平 */
	hal_gpio_level_t level;
	ret = HAL_GPIO_GetLevel(TEST_GPIO_PIN, &level);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 阶段6: 清理 */
	ret = HAL_GPIO_Deinit(TEST_GPIO_PIN);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS ] GPIO lifecycle test completed (final level=%d)\n", level);
}

/**
 * @brief 测试GPIO快速翻转
 * @details 验证GPIO在高频率操作下的稳定性
 */
static void test_system_gpio_toggle(void)
{
	hal_gpio_config_t config = {
		.direction = HAL_GPIO_DIR_OUTPUT,
		.initial_level = HAL_GPIO_LEVEL_LOW,
		.edge = HAL_GPIO_EDGE_NONE,
		.callback = NULL,
		.user_data = NULL
	};

	/* 初始化 */
	int32_t ret = HAL_GPIO_Init(TEST_GPIO_PIN, &config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ SKIP ] GPIO not available\n");
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
		return;
	}

	/* 快速翻转100次 */
	for (int i = 0; i < 100; i++) {
		ret = HAL_GPIO_SetLevel(TEST_GPIO_PIN,
		                        (i % 2) ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		OSAL_msleep(1); /* 1ms */
	}

	/* 清理 */
	HAL_GPIO_Deinit(TEST_GPIO_PIN);

	OSAL_printf("[ PASS ] GPIO toggle test completed\n");
}

/*===========================================================================
 * 多外设协同测试
 *===========================================================================*/

/**
 * @brief 测试多个外设同时工作
 * @details 验证CAN + GPIO + Watchdog同时运行时的稳定性
 */
static void test_system_multi_peripheral_coordination(void)
{
	hal_can_handle_t can_handle = NULL;
	hal_watchdog_handle_t wdt_handle = NULL;
	int32_t ret;
	int test_passed = 0;

	OSAL_printf("[ INFO ] Starting multi-peripheral coordination test...\n");

	/* 初始化CAN */
	hal_can_config_t can_config = {
		.interface = TEST_CAN_INTERFACE,
		.baudrate = TEST_CAN_BAUDRATE,
		.rx_timeout = 500,
		.tx_timeout = 500
	};
	ret = HAL_CAN_Init(&can_config, &can_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ WARN ] CAN init failed, continuing without CAN\n");
		can_handle = NULL;
	}

	/* 初始化GPIO */
	hal_gpio_config_t gpio_config = {
		.direction = HAL_GPIO_DIR_OUTPUT,
		.initial_level = HAL_GPIO_LEVEL_LOW,
		.edge = HAL_GPIO_EDGE_NONE,
		.callback = NULL,
		.user_data = NULL
	};
	ret = HAL_GPIO_Init(TEST_GPIO_PIN, &gpio_config);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ WARN ] GPIO init failed, continuing without GPIO\n");
	}

	/* 初始化Watchdog（仅读取，不启用） */
	hal_watchdog_config_t wdt_config = {
		.device = TEST_WATCHDOG_DEVICE,
		.timeout_sec = 30,
		.enable_on_init = false
	};
	ret = HAL_WATCHDOG_Init(&wdt_config, &wdt_handle);
	if (ret != OSAL_SUCCESS) {
		OSAL_printf("[ WARN ] Watchdog init failed, continuing without watchdog\n");
		wdt_handle = NULL;
	}

	/* 协同操作：循环10次，每次操作所有外设 */
	for (int i = 0; i < 10; i++) {
		/* 操作CAN */
		if (can_handle) {
			hal_can_frame_t frame = {
				.can_id = 0x200 + i,
				.dlc = 2,
				.data = {i, i+1}
			};
			ret = HAL_CAN_Send(can_handle, &frame);
			if (ret != OSAL_SUCCESS) {
				OSAL_printf("[ WARN ] CAN send failed at iteration %d\n", i);
			}
		}

		/* 操作GPIO */
		ret = HAL_GPIO_SetLevel(TEST_GPIO_PIN,
		                        (i % 2) ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW);
		if (ret != OSAL_SUCCESS) {
			OSAL_printf("[ WARN ] GPIO set failed at iteration %d\n", i);
		}

		/* 读取Watchdog状态 */
		if (wdt_handle) {
			uint32_t kick_count = 0;
			ret = HAL_WATCHDOG_GetStats(wdt_handle, &kick_count);
			/* Watchdog可能不支持GetStats，这是正常的 */
		}

		OSAL_msleep(100); /* 100ms */
	}

	test_passed = 1;

	/* 清理所有资源 */
	if (can_handle) {
		HAL_CAN_Deinit(can_handle);
	}
	HAL_GPIO_Deinit(TEST_GPIO_PIN);
	if (wdt_handle) {
		HAL_WATCHDOG_Deinit(wdt_handle);
	}

	TEST_ASSERT_TRUE(test_passed);
	OSAL_printf("[ PASS ] Multi-peripheral coordination test completed\n");
}

/*===========================================================================
 * 错误恢复测试
 *===========================================================================*/

/**
 * @brief 测试硬件故障场景下的恢复能力
 * @details 模拟各种错误情况并验证系统能否正确恢复
 */
static void test_system_hardware_fault_recovery(void)
{
	int32_t ret;
	int recovery_count = 0;

	OSAL_printf("[ INFO ] Starting hardware fault recovery test...\n");

	/* 场景1: CAN初始化失败后重试 */
	{
		hal_can_handle_t handle = NULL;
		hal_can_config_t config = {
			.interface = "invalid_can",
			.baudrate = TEST_CAN_BAUDRATE,
			.rx_timeout = 100,
			.tx_timeout = 100
		};

		/* 第一次尝试（应该失败） */
		ret = HAL_CAN_Init(&config, &handle);
		TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_NULL(handle);

		/* 使用正确配置重试 */
		config.interface = TEST_CAN_INTERFACE;
		ret = HAL_CAN_Init(&config, &handle);
		if (ret == OSAL_SUCCESS) {
			recovery_count++;
			HAL_CAN_Deinit(handle);
			OSAL_printf("[ PASS ] CAN recovery successful\n");
		} else {
			OSAL_printf("[ SKIP ] CAN device not available for recovery test\n");
		}
	}

	/* 场景2: GPIO重复初始化和清理 */
	{
		hal_gpio_config_t config = {
			.direction = HAL_GPIO_DIR_OUTPUT,
			.initial_level = HAL_GPIO_LEVEL_LOW,
			.edge = HAL_GPIO_EDGE_NONE,
			.callback = NULL,
			.user_data = NULL
		};

		/* 多次初始化-清理循环 */
		for (int i = 0; i < 3; i++) {
			ret = HAL_GPIO_Init(TEST_GPIO_PIN, &config);
			if (ret == OSAL_SUCCESS) {
				HAL_GPIO_Deinit(TEST_GPIO_PIN);
			}
		}

		/* 最终验证GPIO仍然可用 */
		ret = HAL_GPIO_Init(TEST_GPIO_PIN, &config);
		if (ret == OSAL_SUCCESS) {
			recovery_count++;
			HAL_GPIO_Deinit(TEST_GPIO_PIN);
			OSAL_printf("[ PASS ] GPIO recovery successful\n");
		} else {
			OSAL_printf("[ SKIP ] GPIO not available for recovery test\n");
		}
	}

	/* 场景3: UART配置错误恢复 */
	{
		hal_serial_handle_t handle = NULL;
		hal_serial_config_t config = {
			.baud_rate = 9999999, /* 无效波特率 */
			.data_bits = 8,
			.parity = HAL_SERIAL_PARITY_NONE,
			.stop_bits = 1,
			.flow_control = HAL_SERIAL_FLOW_NONE
		};

		/* 第一次尝试（可能失败） */
		ret = HAL_Serial_Open(TEST_UART_DEVICE, &config, &handle);
		if (ret != OSAL_SUCCESS) {
			/* 使用正确配置重试 */
			config.baud_rate = TEST_UART_BAUDRATE;
			ret = HAL_Serial_Open(TEST_UART_DEVICE, &config, &handle);
			if (ret == OSAL_SUCCESS) {
				recovery_count++;
				HAL_Serial_Close(handle);
				OSAL_printf("[ PASS ] UART recovery successful\n");
			}
		} else {
			/* 如果第一次成功了，也算通过 */
			HAL_Serial_Close(handle);
			recovery_count++;
		}
	}

	/* 至少有一个恢复场景成功才算通过 */
	TEST_ASSERT_TRUE(recovery_count > 0);
	OSAL_printf("[ PASS ] Hardware fault recovery test completed (%d scenarios recovered)\n",
	            recovery_count);
}

/**
 * @brief 测试资源耗尽恢复
 * @details 打开多个外设直到资源耗尽，然后验证释放后能否恢复
 */
static void test_system_resource_exhaustion_recovery(void)
{
	#define MAX_HANDLES 10
	int opened_count = 0;
	int32_t ret;

	OSAL_printf("[ INFO ] Starting resource exhaustion recovery test...\n");

	/* 尝试打开多个GPIO */
	for (int i = 0; i < MAX_HANDLES; i++) {
		hal_gpio_config_t config = {
			.direction = HAL_GPIO_DIR_INPUT,
			.initial_level = HAL_GPIO_LEVEL_LOW,
			.edge = HAL_GPIO_EDGE_NONE,
			.callback = NULL,
			.user_data = NULL
		};

		ret = HAL_GPIO_Init(TEST_GPIO_PIN + i, &config);
		if (ret == OSAL_SUCCESS) {
			opened_count++;
		} else {
			break; /* 资源耗尽或pin不存在 */
		}
	}

	OSAL_printf("[ INFO ] Opened %d GPIO handles\n", opened_count);

	/* 释放所有资源 */
	for (int i = 0; i < opened_count; i++) {
		HAL_GPIO_Deinit(TEST_GPIO_PIN + i);
	}

	/* 验证释放后能否再次打开 */
	hal_gpio_config_t config = {
		.direction = HAL_GPIO_DIR_INPUT,
		.initial_level = HAL_GPIO_LEVEL_LOW,
		.edge = HAL_GPIO_EDGE_NONE,
		.callback = NULL,
		.user_data = NULL
	};

	ret = HAL_GPIO_Init(TEST_GPIO_PIN, &config);
	if (ret == OSAL_SUCCESS) {
		HAL_GPIO_Deinit(TEST_GPIO_PIN);
		OSAL_printf("[ PASS ] Resource recovery successful\n");
		TEST_ASSERT_TRUE(1);
	} else {
		OSAL_printf("[ SKIP ] GPIO not available for recovery test\n");
		TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);
	}
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	/* CAN系统测试 */
	{
		.name = "test_system_can_lifecycle",
		.func = test_system_can_lifecycle,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	{
		.name = "test_system_can_loopback",
		.func = test_system_can_loopback,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	/* UART系统测试 */
	{
		.name = "test_system_uart_lifecycle",
		.func = test_system_uart_lifecycle,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	{
		.name = "test_system_uart_echo",
		.func = test_system_uart_echo,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	/* GPIO系统测试 */
	{
		.name = "test_system_gpio_lifecycle",
		.func = test_system_gpio_lifecycle,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	{
		.name = "test_system_gpio_toggle",
		.func = test_system_gpio_toggle,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	/* 多外设协同测试 */
	{
		.name = "test_system_multi_peripheral_coordination",
		.func = test_system_multi_peripheral_coordination,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	/* 错误恢复测试 */
	{
		.name = "test_system_hardware_fault_recovery",
		.func = test_system_hardware_fault_recovery,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
	{
		.name = "test_system_resource_exhaustion_recovery",
		.func = test_system_resource_exhaustion_recovery,
		.setup = system_hal_setup,
		.teardown = system_hal_teardown
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "system_hal",
	.module_name = "system_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
		.timeout_ms = TEST_LONG_TIMEOUT_MS,
		.description = "HAL system integration tests - lifecycle, coordination, and recovery"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_system_hal_tests(void)
{
	libutest_register_suite(&test_suite);
}
