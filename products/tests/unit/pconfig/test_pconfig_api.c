/**
 * @file test_pconfig_api.c
 * @brief PCONFIG API单元测试
 */

#include "test_framework.h"
#include "pconfig/pconfig.h"

/* 测试用的MCU配置 */
static pconfig_mcu_entry_t test_mcu_entries[] = {
    {
        .name = "power_mcu",
        .description = "Power management MCU",
        .enabled = true,
        .config = {
            .name = "power_mcu",
            .interface = PDL_MCU_INTERFACE_CAN,
            .hw.can = {
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
            .hw.can = {
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
            .primary_channel = PDL_BMC_CHANNEL_SERIAL,
            .primary_config.serial = {
                .device = "/dev/ttyS1",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = 0,
                .timeout_ms = 2000
            },
            .backup_channel = PDL_BMC_CHANNEL_NETWORK,
            .backup_config.network = {
                .ip_addr = "192.168.1.100",
                .port = 623,
                .username = "admin",
                .password = "admin",
                .timeout_ms = 5000
            },
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

    .mcu_count = 2,
    .mcu_arr = test_mcu_ptrs,

    .bmc_count = 1,
    .bmc_arr = test_bmc_ptrs,

    .fpga_count = 0,
    .fpga_arr = NULL,

    .switch_count = 0,
    .switch_arr = NULL
};

/*===========================================================================
 * 初始化和注册测试
 *===========================================================================*/

static void test_pconfig_init_success(void)
{
    int32_t ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_register_success(void)
{
    PCONFIG_Init();
    int32_t ret = PCONFIG_Register(&test_platform_config);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_register_null(void)
{
    PCONFIG_Init();
    int32_t ret = PCONFIG_Register(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 板级配置查询测试
 *===========================================================================*/

static void test_pconfig_get_board(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->platform_name, "ti"));
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->product_name, "test_board"));
}

static void test_pconfig_find_by_name(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("ti", "test_board", NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->chip_name, "am625"));
}

static void test_pconfig_find_not_found(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("unknown", "unknown", NULL);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

/*===========================================================================
 * MCU配置查询测试
 *===========================================================================*/

static void test_pconfig_hw_find_mcu_by_name(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, "power_mcu");

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(mcu->name, "power_mcu"));
    TEST_ASSERT_EQUAL(0x100, mcu->config.hw.can.tx_id);
    TEST_ASSERT_TRUE(mcu->enabled);
}

static void test_pconfig_hw_find_mcu_not_found(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, "unknown_mcu");

    TEST_ASSERT_EQUAL(NULL, mcu);
}

static void test_pconfig_hw_get_mcu_by_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(mcu->name, "power_mcu"));
}

static void test_pconfig_hw_get_mcu_invalid_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 999);

    TEST_ASSERT_EQUAL(NULL, mcu);
}

static void test_pconfig_hw_find_mcu_null_platform(void)
{
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(NULL, "power_mcu");
    TEST_ASSERT_EQUAL(NULL, mcu);
}

static void test_pconfig_hw_find_mcu_null_name(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_FindMCU(platform, NULL);
    TEST_ASSERT_EQUAL(NULL, mcu);
}

/*===========================================================================
 * BMC配置查询测试
 *===========================================================================*/

static void test_pconfig_hw_find_bmc_by_name(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_FindBMC(platform, "satellite_bmc");

    TEST_ASSERT_NOT_EQUAL(NULL, bmc);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(bmc->name, "satellite_bmc"));
    TEST_ASSERT_EQUAL(PDL_BMC_CHANNEL_SERIAL, bmc->config.primary_channel);
    TEST_ASSERT_EQUAL(115200, bmc->config.primary_config.serial.baudrate);
    TEST_ASSERT_TRUE(bmc->enabled);
}

static void test_pconfig_hw_find_bmc_not_found(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_FindBMC(platform, "unknown_bmc");

    TEST_ASSERT_EQUAL(NULL, bmc);
}

static void test_pconfig_hw_get_bmc_by_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, bmc);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(bmc->name, "satellite_bmc"));
}

static void test_pconfig_hw_get_bmc_invalid_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(platform, 999);

    TEST_ASSERT_EQUAL(NULL, bmc);
}

/*===========================================================================
 * 配置验证测试
 *===========================================================================*/

static void test_pconfig_validate_success(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    int32_t ret = PCONFIG_Validate(platform);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_validate_null(void)
{
    int32_t ret = PCONFIG_Validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 配置列表测试
 *===========================================================================*/

static void test_pconfig_list(void)
{
    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCONFIG_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(count >= 1);  /* 至少有我们注册的一个 */
}

static void test_pconfig_list_null_pointer(void)
{
    uint32_t count = 10;
    int32_t ret = PCONFIG_List(NULL, &count);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/*===========================================================================
 * 打印配置测试（仅验证不崩溃）
 *===========================================================================*/

static void test_pconfig_print(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    /* 仅验证函数不崩溃 */
    PCONFIG_Print(platform);
    TEST_ASSERT_TRUE(true);
}

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_pconfig_init_success",
		.func = test_pconfig_init_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_register_success",
		.func = test_pconfig_register_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_register_null",
		.func = test_pconfig_register_null,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_get_board",
		.func = test_pconfig_get_board,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_find_by_name",
		.func = test_pconfig_find_by_name,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_find_not_found",
		.func = test_pconfig_find_not_found,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_mcu_by_name",
		.func = test_pconfig_hw_find_mcu_by_name,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_mcu_not_found",
		.func = test_pconfig_hw_find_mcu_not_found,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_get_mcu_by_id",
		.func = test_pconfig_hw_get_mcu_by_id,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_get_mcu_invalid_id",
		.func = test_pconfig_hw_get_mcu_invalid_id,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_mcu_null_platform",
		.func = test_pconfig_hw_find_mcu_null_platform,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_mcu_null_name",
		.func = test_pconfig_hw_find_mcu_null_name,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_bmc_by_name",
		.func = test_pconfig_hw_find_bmc_by_name,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_find_bmc_not_found",
		.func = test_pconfig_hw_find_bmc_not_found,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_get_bmc_by_id",
		.func = test_pconfig_hw_get_bmc_by_id,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_hw_get_bmc_invalid_id",
		.func = test_pconfig_hw_get_bmc_invalid_id,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_validate_success",
		.func = test_pconfig_validate_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_validate_null",
		.func = test_pconfig_validate_null,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_list",
		.func = test_pconfig_list,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_list_null_pointer",
		.func = test_pconfig_list_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pconfig_print",
		.func = test_pconfig_print,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "pconfig_api",
	.module_name = "pconfig_api",
	.layer_name = "PCONFIG",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PCONFIG pconfig_api tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_pconfig_api_tests(void)
{
	libutest_register_suite(&test_suite);
}
