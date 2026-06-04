#include "test_framework.h"
/**
 * @file test_pdl_bmc.c
 * @brief PDL BMC通信服务单元测试
 */

#include "pdl/pdl_bmc_api.h"
#include "osal/osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: BMC服务初始化 - 网络通道 */
TEST_CASE(test_pdl_bmc_init_network_success)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .serial = {
            .enabled = false
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: BMC服务初始化 - 串口通道 */
TEST_CASE(test_pdl_bmc_init_serial_success)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false
        },
        .serial = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .device = "/dev/ttyS2",
            .baudrate = 115200,
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_SERIAL,
        .auto_switch = false,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: BMC服务初始化 - 空配置 */
TEST_CASE(test_pdl_bmc_init_null_config)
{
    pdl_bmc_handle_t handle = NULL;

    int32_t ret = PDL_BMC_Init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务初始化 - 空句柄指针 */
TEST_CASE(test_pdl_bmc_init_null_handle)
{
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务清理 */
TEST_CASE(test_pdl_bmc_deinit)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_Deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 清理空句柄 */
TEST_CASE(test_pdl_bmc_deinit_null_handle)
{
    int32_t ret = PDL_BMC_Deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 电源控制测试
 *===========================================================================*/

/* 测试用例: 电源开启 - 成功 */
TEST_CASE(test_pdl_bmc_power_on_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerOn(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 电源开启 - 空句柄 */
TEST_CASE(test_pdl_bmc_power_on_null_handle)
{
    int32_t ret = PDL_BMC_PowerOn(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 电源关闭 - 成功 */
TEST_CASE(test_pdl_bmc_power_off_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerOff(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 电源复位 - 成功 */
TEST_CASE(test_pdl_bmc_power_reset_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_PowerReset(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_msleep(100);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取电源状态 - 成功 */
TEST_CASE(test_pdl_bmc_get_power_state_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
    pdl_bmc_power_state_t state;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetPowerState(handle, &state);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    TEST_ASSERT_TRUE(state >= PDL_BMC_POWER_OFF && state <= PDL_BMC_POWER_UNKNOWN);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取电源状态 - 空句柄 */
TEST_CASE(test_pdl_bmc_get_power_state_null_handle)
{
    pdl_bmc_power_state_t state;

    int32_t ret = PDL_BMC_GetPowerState(NULL, &state);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取电源状态 - 空指针 */
TEST_CASE(test_pdl_bmc_get_power_state_null_pointer)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

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
TEST_CASE(test_pdl_bmc_read_sensors_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
    pdl_bmc_sensor_reading_t readings[16];
    uint32_t actual_count;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_Memset(readings, 0, sizeof(readings));
    ret = PDL_BMC_ReadSensors(handle, PDL_BMC_SENSOR_TEMP, readings, 16, &actual_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    TEST_ASSERT_TRUE(actual_count <= 16);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 读取传感器 - 空句柄 */
TEST_CASE(test_pdl_bmc_read_sensors_null_handle)
{
    pdl_bmc_sensor_reading_t readings[16];
    uint32_t actual_count;

    int32_t ret = PDL_BMC_ReadSensors(NULL, PDL_BMC_SENSOR_TEMP, readings, 16, &actual_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 读取传感器 - 空指针 */
TEST_CASE(test_pdl_bmc_read_sensors_null_pointer)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
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
TEST_CASE(test_pdl_bmc_execute_command_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
    char response[256];

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_ExecuteCommand(handle, "chassis status", response, sizeof(response));
    TEST_ASSERT_TRUE(ret >= 0);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 执行命令 - 空句柄 */
TEST_CASE(test_pdl_bmc_execute_command_null_handle)
{
    char response[256];

    int32_t ret = PDL_BMC_ExecuteCommand(NULL, "chassis status", response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 执行命令 - 空命令 */
TEST_CASE(test_pdl_bmc_execute_command_null_cmd)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
    char response[256];

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_ExecuteCommand(handle, NULL, response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 通道切换测试
 *===========================================================================*/

/* 测试用例: 切换通道 - 成功 */
TEST_CASE(test_pdl_bmc_switch_channel_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .serial = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .device = "/dev/ttyS2",
            .baudrate = 115200,
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_SwitchChannel(handle, PDL_BMC_CHANNEL_SERIAL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_SwitchChannel(handle, PDL_BMC_CHANNEL_NETWORK);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 切换通道 - 空句柄 */
TEST_CASE(test_pdl_bmc_switch_channel_null_handle)
{
    int32_t ret = PDL_BMC_SwitchChannel(NULL, PDL_BMC_CHANNEL_NETWORK);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取当前通道 - 成功 */
TEST_CASE(test_pdl_bmc_get_channel_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

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
TEST_CASE(test_pdl_bmc_is_connected_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

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
TEST_CASE(test_pdl_bmc_get_stats_success)
{
    TEST_ASSERT_FALSE(true); // Requires real BMC hardware
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };
    uint32_t cmd_count, success_count, fail_count, switch_count;

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetStats(handle, &cmd_count, &success_count, &fail_count, &switch_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* uint32_t类型总是>=0，只需验证调用成功 */

    PDL_BMC_Deinit(handle);
}

/* 测试用例: 获取统计信息 - 空句柄 */
TEST_CASE(test_pdl_bmc_get_stats_null_handle)
{
    uint32_t cmd_count, success_count, fail_count, switch_count;

    int32_t ret = PDL_BMC_GetStats(NULL, &cmd_count, &success_count, &fail_count, &switch_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取统计信息 - 空指针 */
TEST_CASE(test_pdl_bmc_get_stats_null_pointer)
{
    pdl_bmc_handle_t handle = NULL;
    pdl_bmc_config_t config = {
        .network = {
            .enabled = false,  /* 禁用网络避免测试卡死 */
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = "admin",
            .timeout_ms = 5000
        },
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    };

    int32_t ret = PDL_BMC_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PDL_BMC_GetStats(handle, NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    PDL_BMC_Deinit(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_pdl_bmc, "PDL")
    // PDL BMC通信服务测试
    /* 初始化和清理 */
    TEST_CASE_REF(test_pdl_bmc_init_network_success)
    TEST_CASE_REF(test_pdl_bmc_init_serial_success)
    TEST_CASE_REF(test_pdl_bmc_init_null_config)
    TEST_CASE_REF(test_pdl_bmc_init_null_handle)
    TEST_CASE_REF(test_pdl_bmc_deinit)
    TEST_CASE_REF(test_pdl_bmc_deinit_null_handle)

    /* 电源控制 */
    TEST_CASE_REF(test_pdl_bmc_power_on_success)
    TEST_CASE_REF(test_pdl_bmc_power_on_null_handle)
    TEST_CASE_REF(test_pdl_bmc_power_off_success)
    TEST_CASE_REF(test_pdl_bmc_power_reset_success)
    TEST_CASE_REF(test_pdl_bmc_get_power_state_success)
    TEST_CASE_REF(test_pdl_bmc_get_power_state_null_handle)
    TEST_CASE_REF(test_pdl_bmc_get_power_state_null_pointer)

    /* 传感器读取 */
    TEST_CASE_REF(test_pdl_bmc_read_sensors_success)
    TEST_CASE_REF(test_pdl_bmc_read_sensors_null_handle)
    TEST_CASE_REF(test_pdl_bmc_read_sensors_null_pointer)

    /* 命令执行 */
    TEST_CASE_REF(test_pdl_bmc_execute_command_success)
    TEST_CASE_REF(test_pdl_bmc_execute_command_null_handle)
    TEST_CASE_REF(test_pdl_bmc_execute_command_null_cmd)

    /* 通道切换 */
    TEST_CASE_REF(test_pdl_bmc_switch_channel_success)
    TEST_CASE_REF(test_pdl_bmc_switch_channel_null_handle)
    TEST_CASE_REF(test_pdl_bmc_get_channel_success)

    /* 连接状态 */
    TEST_CASE_REF(test_pdl_bmc_is_connected_success)

    /* 统计信息 */
    TEST_CASE_REF(test_pdl_bmc_get_stats_success)
    TEST_CASE_REF(test_pdl_bmc_get_stats_null_handle)
    TEST_CASE_REF(test_pdl_bmc_get_stats_null_pointer)
TEST_MODULE_END(test_pdl_bmc, "PDL")
