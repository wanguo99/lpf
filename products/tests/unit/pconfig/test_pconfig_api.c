/**
 * @file test_pconfig_api.c
 * @brief PCONFIG API单元测试
 */

#include "test_framework.h"
#include "pconfig.h"
#include <string.h>

/* 测试用的MCU配置 */
static pconfig_mcu_entry_t test_mcu_entries[] = {
    {
        .name = "power_mcu",
        .description = "Power management MCU",
        .enabled = true,
        .config = {
            .name = "power_mcu",
            .interface = PDL_MCU_INTERFACE_CAN,
            .can = {
                .device = "can0",
                .bitrate = 500000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x100,
                .rx_id = 0x101
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    {
        .name = "payload_mcu",
        .description = "Payload control MCU",
        .enabled = true,
        .config = {
            .name = "payload_mcu",
            .interface = PDL_MCU_INTERFACE_CAN,
            .can = {
                .device = "can0",
                .bitrate = 500000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x200,
                .rx_id = 0x201
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};

/* 测试用的BMC配置 */
static pconfig_bmc_entry_t test_bmc_entries[] = {
    {
        .name = "satellite_bmc",
        .description = "Satellite platform BMC",
        .enabled = true,
        .config = {
            .serial = {
                .enabled = true,
                .device = "/dev/ttyS1",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = 0,
                .timeout_ms = 2000
            },
            .primary_channel = PDL_BMC_CHANNEL_SERIAL,
            .auto_switch = false,
            .retry_count = 3,
            .health_check_interval = 5000
        }
    }
};

/* 测试用的MCU配置指针数组 */
static pconfig_mcu_entry_t *test_mcu_ptrs[] = {
    &test_mcu_entries[0],
    &test_mcu_entries[1],
    NULL
};

/* 测试用的BMC配置指针数组 */
static pconfig_bmc_entry_t *test_bmc_ptrs[] = {
    &test_bmc_entries[0],
    NULL
};

/* 测试用的平台配置 */
static pconfig_platform_config_t test_platform_config = {
    .platform_name = "ti",
    .chip_name = "am625",
    .project_name = "test_project",
    .product_name = "test_board",

    .mcu_arr = test_mcu_ptrs,
    .bmc_arr = test_bmc_ptrs,
    .fpga_arr = NULL,
    .switch_arr = NULL
};

/* 测试夹具 */
static void pconfig_test_setup(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_config);
}

static void pconfig_test_teardown(void)
{
    PCONFIG_Cleanup();
}

/*===========================================================================
 * 初始化和注册测试
 *===========================================================================*/

TEST_CASE(test_pconfig_init_success)
{
    int32_t ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_pconfig_register_success)
{
    PCONFIG_Init();
    int32_t ret = PCONFIG_Register(&test_platform_config);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_pconfig_register_null)
{
    PCONFIG_Init();
    int32_t ret = PCONFIG_Register(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 板级配置查询测试
 *===========================================================================*/

TEST_CASE(test_pconfig_get_board)
{
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, strcmp(cfg->platform_name, "ti"));
    TEST_ASSERT_EQUAL(0, strcmp(cfg->product_name, "test_board"));
}

TEST_CASE(test_pconfig_find_by_name)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("ti", "test_board", NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, strcmp(cfg->chip_name, "am625"));
}

TEST_CASE(test_pconfig_find_not_found)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("unknown", "unknown", NULL);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

/*===========================================================================
 * MCU配置查询测试
 *===========================================================================*/

TEST_CASE(test_pconfig_hw_find_mcu_by_name)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, "power_mcu");

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, strcmp(mcu->name, "power_mcu"));
    TEST_ASSERT_EQUAL(0x100, mcu->config.can.tx_id);
    TEST_ASSERT_TRUE(mcu->enabled);
}

TEST_CASE(test_pconfig_hw_find_mcu_not_found)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, "unknown_mcu");

    TEST_ASSERT_EQUAL(NULL, mcu);
}

TEST_CASE(test_pconfig_hw_get_mcu_by_id)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, strcmp(mcu->name, "power_mcu"));
}

TEST_CASE(test_pconfig_hw_get_mcu_invalid_id)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 999);

    TEST_ASSERT_EQUAL(NULL, mcu);
}

TEST_CASE(test_pconfig_hw_find_mcu_null_platform)
{
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(NULL, "power_mcu");
    TEST_ASSERT_EQUAL(NULL, mcu);
}

TEST_CASE(test_pconfig_hw_find_mcu_null_name)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, NULL);
    TEST_ASSERT_EQUAL(NULL, mcu);
}

/*===========================================================================
 * BMC配置查询测试
 *===========================================================================*/

TEST_CASE(test_pconfig_hw_find_bmc_by_name)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_FindBMC(platform, "satellite_bmc");

    TEST_ASSERT_NOT_EQUAL(NULL, bmc);
    TEST_ASSERT_EQUAL(0, strcmp(bmc->name, "satellite_bmc"));
    TEST_ASSERT_EQUAL(115200, bmc->config.serial.baudrate);
    TEST_ASSERT_TRUE(bmc->enabled);
}

TEST_CASE(test_pconfig_hw_find_bmc_not_found)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_FindBMC(platform, "unknown_bmc");

    TEST_ASSERT_EQUAL(NULL, bmc);
}

TEST_CASE(test_pconfig_hw_get_bmc_by_id)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, bmc);
    TEST_ASSERT_EQUAL(0, strcmp(bmc->name, "satellite_bmc"));
}

TEST_CASE(test_pconfig_hw_get_bmc_invalid_id)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(platform, 999);

    TEST_ASSERT_EQUAL(NULL, bmc);
}

/*===========================================================================
 * 配置验证测试
 *===========================================================================*/

TEST_CASE(test_pconfig_validate_success)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    int32_t ret = PCONFIG_Validate(platform);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_pconfig_validate_null)
{
    int32_t ret = PCONFIG_Validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 配置列表测试
 *===========================================================================*/

TEST_CASE(test_pconfig_list)
{
    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCONFIG_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(count >= 1);  /* 至少有我们注册的一个 */
}

TEST_CASE(test_pconfig_list_null_pointer)
{
    uint32_t count = 10;
    int32_t ret = PCONFIG_List(NULL, &count);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 打印配置测试（仅验证不崩溃）
 *===========================================================================*/

TEST_CASE(test_pconfig_print)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    /* 仅验证函数不崩溃 */
    PCONFIG_Print(platform);
    TEST_ASSERT_TRUE(true);
}

/* 注册测试套件 */
static const test_case_t pconfig_api_cases[] = {
    TEST_CASE_REF(test_pconfig_init_success)
    TEST_CASE_REF(test_pconfig_register_success)
    TEST_CASE_REF(test_pconfig_register_null)
    TEST_CASE_REF(test_pconfig_get_board)
    TEST_CASE_REF(test_pconfig_find_by_name)
    TEST_CASE_REF(test_pconfig_find_not_found)
    TEST_CASE_REF(test_pconfig_hw_find_mcu_by_name)
    TEST_CASE_REF(test_pconfig_hw_find_mcu_not_found)
    TEST_CASE_REF(test_pconfig_hw_get_mcu_by_id)
    TEST_CASE_REF(test_pconfig_hw_get_mcu_invalid_id)
    TEST_CASE_REF(test_pconfig_hw_find_mcu_null_platform)
    TEST_CASE_REF(test_pconfig_hw_find_mcu_null_name)
    TEST_CASE_REF(test_pconfig_hw_find_bmc_by_name)
    TEST_CASE_REF(test_pconfig_hw_find_bmc_not_found)
    TEST_CASE_REF(test_pconfig_hw_get_bmc_by_id)
    TEST_CASE_REF(test_pconfig_hw_get_bmc_invalid_id)
    TEST_CASE_REF(test_pconfig_validate_success)
    TEST_CASE_REF(test_pconfig_validate_null)
    TEST_CASE_REF(test_pconfig_list)
    TEST_CASE_REF(test_pconfig_list_null_pointer)
    TEST_CASE_REF(test_pconfig_print)
};

static const test_suite_t pconfig_api_suite = {
    .suite_name = "pconfig_api",
    .module_name = "pconfig_api",
    .layer_name = "PCONFIG",
    .cases = pconfig_api_cases,
    .case_count = sizeof(pconfig_api_cases) / sizeof(test_case_t),
    .suite_setup = pconfig_test_setup,
    .suite_teardown = pconfig_test_teardown
};

__attribute__((constructor))
static void register_pconfig_api(void) {
    libutest_register_suite(&pconfig_api_suite);
}
