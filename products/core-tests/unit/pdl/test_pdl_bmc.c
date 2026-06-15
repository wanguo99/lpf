#include <test/test_framework.h>
/**
 * @file test_pdl_bmc.c
 * @brief PDL BMC通信服务单元测试
 */

#include "osal.h"
#include "pdl.h"
#include "pdl_bmc.h"

/*===========================================================================
 * 辅助函数：创建测试配置
 *===========================================================================*/

/* 创建网络主通道配置（串口备用） */
static void create_network_config(pdl_bmc_config_t *config)
{
    config->primary_channel = PDL_BMC_CHANNEL_NETWORK;
    config->primary_config.network.ip_addr = "192.168.1.100";
    config->primary_config.network.port = 623;
    config->primary_config.network.username = "admin";
    config->primary_config.network.password = "admin";
    config->primary_config.network.timeout_ms = 5000;

    config->backup_channel = PDL_BMC_CHANNEL_SERIAL;
    config->backup_config.serial.device = "/dev/ttyS2";
    config->backup_config.serial.baudrate = 115200;
    config->backup_config.serial.timeout_ms = 5000;

    config->auto_switch = false;
    config->retry_count = 3;
    config->health_check_interval = 10000;
}

/* 创建串口主通道配置（网络备用） */
static void create_serial_config(pdl_bmc_config_t *config)
{
    config->primary_channel = PDL_BMC_CHANNEL_SERIAL;
    config->primary_config.serial.device = "/dev/ttyS2";
    config->primary_config.serial.baudrate = 115200;
    config->primary_config.serial.timeout_ms = 5000;

    config->backup_channel = PDL_BMC_CHANNEL_NETWORK;
    config->backup_config.network.ip_addr = "192.168.1.100";
    config->backup_config.network.port = 623;
    config->backup_config.network.username = "admin";
    config->backup_config.network.password = "admin";
    config->backup_config.network.timeout_ms = 5000;

    config->auto_switch = false;
    config->retry_count = 3;
    config->health_check_interval = 10000;
}

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: BMC服务初始化 - 网络通道 */
static void test_pdl_bmc_init_network_success(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: BMC服务初始化 - 串口通道 */
static void test_pdl_bmc_init_serial_success(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_serial_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: BMC服务初始化 - 空配置 */
static void test_pdl_bmc_init_null_config(void)
{
    pdl_bmc_handle_t handle = NULL;

    int32_t ret = PDL_BMC_Init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务初始化 - 空句柄指针 */
static void test_pdl_bmc_init_null_handle(void)
{
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务清理 */
static void test_pdl_bmc_deinit(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_Deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 清理空句柄 */
static void test_pdl_bmc_deinit_null_handle(void)
{
    int32_t ret = PDL_BMC_Deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 电源控制测试
 *===========================================================================*/

/* 测试用例: 电源开启 - 成功 */
static void test_pdl_bmc_power_on_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerOn(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 电源开启 - 空句柄 */
static void test_pdl_bmc_power_on_null_handle(void)
{
    int32_t ret = PDL_BMC_PowerOn(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 电源关闭 - 成功 */
static void test_pdl_bmc_power_off_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerOff(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 电源复位 - 成功 */
static void test_pdl_bmc_power_reset_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerReset(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_msleep(100);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取电源状态 - 成功 */
static void test_pdl_bmc_get_power_state_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    pdl_bmc_power_state_t state;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetPowerState(handle, &state);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    TEST_ASSERT_TRUE(state >= PDL_BMC_POWER_OFF && state <= PDL_BMC_POWER_UNKNOWN);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取电源状态 - 空句柄 */
static void test_pdl_bmc_get_power_state_null_handle(void)
{
    pdl_bmc_power_state_t state;

    int32_t ret = PDL_BMC_GetPowerState(NULL, &state);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取电源状态 - 空指针 */
static void test_pdl_bmc_get_power_state_null_pointer(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetPowerState(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 传感器读取测试
 *===========================================================================*/

/* 测试用例: 读取传感器 - 成功 */
static void test_pdl_bmc_read_sensors_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    pdl_bmc_sensor_reading_t readings[16];
    uint32_t actual_count;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_memset(readings, 0, OSAL_sizeof(readings));
    ret = PDL_BMC_ReadSensors(handle, PDL_BMC_SENSOR_TEMP, readings, 16, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    TEST_ASSERT_TRUE(actual_count <= 16);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 读取传感器 - 空句柄 */
static void test_pdl_bmc_read_sensors_null_handle(void)
{
    pdl_bmc_sensor_reading_t readings[16];
    uint32_t actual_count;

    int32_t ret = PDL_BMC_ReadSensors(NULL, PDL_BMC_SENSOR_TEMP, readings, 16, &actual_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 读取传感器 - 空指针 */
static void test_pdl_bmc_read_sensors_null_pointer(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    uint32_t actual_count;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_ReadSensors(handle, PDL_BMC_SENSOR_TEMP, NULL, 16, &actual_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 命令执行测试
 *===========================================================================*/

/* 测试用例: 执行命令 - 成功 */
static void test_pdl_bmc_execute_command_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    char response[256];

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_ExecuteCommand(handle, "chassis status", response, OSAL_sizeof(response));
    TEST_ASSERT_TRUE(ret >= 0);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 执行命令 - 空句柄 */
static void test_pdl_bmc_execute_command_null_handle(void)
{
    char response[256];

    int32_t ret = PDL_BMC_ExecuteCommand(NULL, "chassis status", response, OSAL_sizeof(response));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 执行命令 - 空命令 */
static void test_pdl_bmc_execute_command_null_cmd(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    char response[256];

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_ExecuteCommand(handle, NULL, response, OSAL_sizeof(response));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 通道切换测试
 *===========================================================================*/

/* 测试用例: 切换通道 - 成功 */
static void test_pdl_bmc_switch_channel_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_SwitchChannel(handle, PDL_BMC_CHANNEL_SERIAL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_SwitchChannel(handle, PDL_BMC_CHANNEL_NETWORK);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 切换通道 - 空句柄 */
static void test_pdl_bmc_switch_channel_null_handle(void)
{
    int32_t ret = PDL_BMC_SwitchChannel(NULL, PDL_BMC_CHANNEL_NETWORK);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取当前通道 - 成功 */
static void test_pdl_bmc_get_channel_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    pdl_bmc_channel_t channel = PDL_BMC_GetChannel(handle);
    TEST_ASSERT_TRUE(channel == PDL_BMC_CHANNEL_NETWORK || channel == PDL_BMC_CHANNEL_SERIAL);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 连接状态测试
 *===========================================================================*/

/* 测试用例: 检查连接状态 - 成功 */
static void test_pdl_bmc_is_connected_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    (void)PDL_BMC_IsConnected(handle);
    /* 连接状态取决于实际硬件，只验证调用不崩溃 */

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 统计信息测试
 *===========================================================================*/

/* 测试用例: 获取统计信息 - 成功 */
static void test_pdl_bmc_get_stats_success(void)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);
    uint32_t cmd_count, success_count, fail_count, switch_count;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetStats(handle, &cmd_count, &success_count, &fail_count, &switch_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* uint32_t类型总是>=0，只需验证调用成功 */

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取统计信息 - 空句柄 */
static void test_pdl_bmc_get_stats_null_handle(void)
{
    uint32_t cmd_count, success_count, fail_count, switch_count;

    int32_t ret = PDL_BMC_GetStats(NULL, &cmd_count, &success_count, &fail_count, &switch_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取统计信息 - 空指针 */
static void test_pdl_bmc_get_stats_null_pointer(void)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config;
    create_network_config(&config);

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetStats(handle, NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// PDL BMC通信服务测试
    /* 初始化和清理 */
    /* 电源控制 */
    /* 传感器读取 */
    /* 命令执行 */
    /* 通道切换 */
    /* 连接状态 */
    /* 统计信息 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_pdl_bmc_init_network_success",
		.func = test_pdl_bmc_init_network_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_init_serial_success",
		.func = test_pdl_bmc_init_serial_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_init_null_config",
		.func = test_pdl_bmc_init_null_config,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_init_null_handle",
		.func = test_pdl_bmc_init_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_deinit",
		.func = test_pdl_bmc_deinit,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_deinit_null_handle",
		.func = test_pdl_bmc_deinit_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_power_on_success",
		.func = test_pdl_bmc_power_on_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_power_on_null_handle",
		.func = test_pdl_bmc_power_on_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_power_off_success",
		.func = test_pdl_bmc_power_off_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_power_reset_success",
		.func = test_pdl_bmc_power_reset_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_power_state_success",
		.func = test_pdl_bmc_get_power_state_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_power_state_null_handle",
		.func = test_pdl_bmc_get_power_state_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_power_state_null_pointer",
		.func = test_pdl_bmc_get_power_state_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_read_sensors_success",
		.func = test_pdl_bmc_read_sensors_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_read_sensors_null_handle",
		.func = test_pdl_bmc_read_sensors_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_read_sensors_null_pointer",
		.func = test_pdl_bmc_read_sensors_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_execute_command_success",
		.func = test_pdl_bmc_execute_command_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_execute_command_null_handle",
		.func = test_pdl_bmc_execute_command_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_execute_command_null_cmd",
		.func = test_pdl_bmc_execute_command_null_cmd,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_switch_channel_success",
		.func = test_pdl_bmc_switch_channel_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_switch_channel_null_handle",
		.func = test_pdl_bmc_switch_channel_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_channel_success",
		.func = test_pdl_bmc_get_channel_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_is_connected_success",
		.func = test_pdl_bmc_is_connected_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_stats_success",
		.func = test_pdl_bmc_get_stats_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_stats_null_handle",
		.func = test_pdl_bmc_get_stats_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_bmc_get_stats_null_pointer",
		.func = test_pdl_bmc_get_stats_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "pdl_bmc",
	.module_name = "pdl_bmc",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PDL pdl_bmc tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_pdl_bmc_tests(void)
{
	libutest_register_suite(&test_suite);
}
