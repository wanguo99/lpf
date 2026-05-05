/**
 * @file test_pcl_api.c
 * @brief PCL API单元测试
 *
 * 测试覆盖：
 * - 配置库初始化和清理
 * - 配置注册和查询
 * - 硬件外设配置查询
 * - APP配置查询
 * - 配置验证
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "pcl_api.h"
#include "osal.h"

/* 测试用的配置数据 */
static const pcl_mcu_cfg_t test_mcu = {
    .name = "test_mcu",
    .description = "Test MCU",
    .enabled = true,
    .interface_type = PCL_HW_INTERFACE_UART,
    .interface_cfg = {
        .uart = {
            .device = "/dev/ttyS0",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = 'N'
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_crc = true,
    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static const pcl_bmc_cfg_t test_bmc = {
    .name = "test_bmc",
    .description = "Test BMC",
    .enabled = true,
    .primary_channel = {
        .protocol = PCL_BMC_PROTOCOL_REDFISH,
        .cfg = {
            .redfish = {
                .base_url = "https://192.168.1.100",
                .username = "admin",
                .password = "admin",
                .use_https = true
            }
        }
    },
    .backup_channel = {
        .protocol = PCL_BMC_PROTOCOL_IPMI,
        .cfg = {
            .device = "/dev/ttyS1",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = 'N'
        }
    },
    .cmd_timeout_ms = 5000,
    .retry_count = 3,
    .failover_threshold = 3,
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static const pcl_satellite_cfg_t test_satellite = {
    .name = "test_satellite",
    .description = "Test satellite",
    .enabled = true,
    .interface_type = PCL_HW_INTERFACE_CAN,
    .interface_cfg = {
        .can = {
            .device = "can0",
            .bitrate = 500000,
            .tx_id = 0x100,
            .rx_id = 0x200
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_telemetry = true,
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pcl_mcu_cfg_t *test_mcus[] = { (pcl_mcu_cfg_t *)&test_mcu };
static pcl_bmc_cfg_t *test_bmcs[] = { (pcl_bmc_cfg_t *)&test_bmc };
static pcl_satellite_cfg_t *test_satellites[] = { (pcl_satellite_cfg_t *)&test_satellite };

static const pcl_board_config_t test_board_v1 = {
    .platform = "test_platform",
    .product = "test_product",
    .version = "v1.0",
    .description = "Test board v1.0",
    .mcus = test_mcus,
    .mcu_count = 1,
    .bmcs = test_bmcs,
    .bmc_count = 1,
    .satellites = test_satellites,
    .satellite_count = 1,
    .sensors = NULL,
    .sensor_count = 0,
    .storages = NULL,
    .storage_count = 0,
    .power_domains = NULL,
    .power_domain_count = 0,
    .apps = NULL,
    .app_count = 0
};

static const pcl_board_config_t test_board_v2 = {
    .platform = "test_platform",
    .product = "test_product",
    .version = "v2.0",
    .description = "Test board v2.0",
    .mcus = test_mcus,
    .mcu_count = 1,
    .bmcs = test_bmcs,
    .bmc_count = 1,
    .satellites = test_satellites,
    .satellite_count = 1,
    .sensors = NULL,
    .sensor_count = 0,
    .storages = NULL,
    .storage_count = 0,
    .power_domains = NULL,
    .power_domain_count = 0,
    .apps = NULL,
    .app_count = 0
};

/* ========== 初始化和清理测试 ========== */

TEST_CASE(test_pcl_init_success)
{
    int32_t ret = PCL_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 重复初始化应该成功 */
    ret = PCL_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_cleanup)
{
    PCL_Init();
    PCL_Cleanup();

    /* 重复清理应该安全 */
    PCL_Cleanup();
}

/* ========== 配置注册测试 ========== */

TEST_CASE(test_pcl_register_success)
{
    PCL_Init();

    int32_t ret = PCL_Register(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_register_null_pointer)
{
    PCL_Init();

    int32_t ret = PCL_Register(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_register_not_initialized)
{
    /* 未初始化时注册应该失败 */
    int32_t ret = PCL_Register(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

TEST_CASE(test_pcl_register_duplicate)
{
    PCL_Init();

    int32_t ret = PCL_Register(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 重复注册相同配置应该失败 */
    ret = PCL_Register(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_register_multiple_versions)
{
    PCL_Init();

    int32_t ret = PCL_Register(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 注册不同版本应该成功 */
    ret = PCL_Register(&test_board_v2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCL_Cleanup();
}

/* ========== 配置查询测试 ========== */

TEST_CASE(test_pcl_find_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_board_config_t *found = PCL_Find("test_platform", "test_product", "v1.0");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform);
    TEST_ASSERT_STRING_EQUAL("test_product", found->product);
    TEST_ASSERT_STRING_EQUAL("v1.0", found->version);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_without_version)
{
    PCL_Init();
    PCL_Register(&test_board_v1);
    PCL_Register(&test_board_v2);

    /* 不指定版本应该返回第一个匹配的 */
    const pcl_board_config_t *found = PCL_Find("test_platform", "test_product", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform);
    TEST_ASSERT_STRING_EQUAL("test_product", found->product);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_not_found)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_board_config_t *found = PCL_Find("nonexistent", "product", NULL);
    TEST_ASSERT_NULL(found);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_null_parameters)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_board_config_t *found = PCL_Find(NULL, "test_product", NULL);
    TEST_ASSERT_NULL(found);

    found = PCL_Find("test_platform", NULL, NULL);
    TEST_ASSERT_NULL(found);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_list_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);
    PCL_Register(&test_board_v2);

    const pcl_board_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCL_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_NOT_NULL(configs[0]);
    TEST_ASSERT_NOT_NULL(configs[1]);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_list_null_parameters)
{
    PCL_Init();

    const pcl_board_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCL_List(NULL, &count);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    ret = PCL_List(configs, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCL_Cleanup();
}

/* ========== 硬件外设查询测试 ========== */

TEST_CASE(test_pcl_hw_find_mcu_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(&test_board_v1, "test_mcu");
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);
    TEST_ASSERT_EQUAL(PCL_HW_INTERFACE_UART, mcu->interface_type);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_mcu_not_found)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(&test_board_v1, "nonexistent");
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_mcu_null_parameters)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(NULL, "test_mcu");
    TEST_ASSERT_NULL(mcu);

    mcu = PCL_HW_FindMCU(&test_board_v1, NULL);
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_get_mcu_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_GetMCU(&test_board_v1, 0);
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_get_mcu_invalid_id)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_GetMCU(&test_board_v1, 999);
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_bmc_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_bmc_cfg_t *bmc = PCL_HW_FindBMC(&test_board_v1, "test_bmc");
    TEST_ASSERT_NOT_NULL(bmc);
    TEST_ASSERT_STRING_EQUAL("test_bmc", bmc->name);
    TEST_ASSERT_EQUAL(PCL_BMC_PROTOCOL_REDFISH, bmc->primary_channel.protocol);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_satellite_success)
{
    PCL_Init();
    PCL_Register(&test_board_v1);

    const pcl_satellite_cfg_t *sat = PCL_HW_FindSatellite(&test_board_v1, "test_satellite");
    TEST_ASSERT_NOT_NULL(sat);
    TEST_ASSERT_STRING_EQUAL("test_satellite", sat->name);
    TEST_ASSERT_EQUAL(PCL_HW_INTERFACE_CAN, sat->interface_type);

    PCL_Cleanup();
}

/* ========== 配置验证测试 ========== */

TEST_CASE(test_pcl_validate_success)
{
    int32_t ret = PCL_Validate(&test_board_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_pcl_validate_null_pointer)
{
    int32_t ret = PCL_Validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/* ========== 测试套件注册 ========== */

TEST_SUITE_BEGIN(test_pcl_init_suite, "pcl_api", "PCL")
    TEST_CASE_REF(test_pcl_init_success)
    TEST_CASE_REF(test_pcl_cleanup)
TEST_SUITE_END(test_pcl_init_suite, "test_pcl_api", "PCL")

TEST_SUITE_BEGIN(test_pcl_register_suite, "pcl_api", "PCL")
    TEST_CASE_REF(test_pcl_register_success)
    TEST_CASE_REF(test_pcl_register_null_pointer)
    TEST_CASE_REF(test_pcl_register_not_initialized)
    TEST_CASE_REF(test_pcl_register_duplicate)
    TEST_CASE_REF(test_pcl_register_multiple_versions)
TEST_SUITE_END(test_pcl_register_suite, "test_pcl_api", "PCL")

TEST_SUITE_BEGIN(test_pcl_query_suite, "pcl_api", "PCL")
    TEST_CASE_REF(test_pcl_find_success)
    TEST_CASE_REF(test_pcl_find_without_version)
    TEST_CASE_REF(test_pcl_find_not_found)
    TEST_CASE_REF(test_pcl_find_null_parameters)
    TEST_CASE_REF(test_pcl_list_success)
    TEST_CASE_REF(test_pcl_list_null_parameters)
TEST_SUITE_END(test_pcl_query_suite, "test_pcl_api", "PCL")

TEST_SUITE_BEGIN(test_pcl_hw_suite, "pcl_api", "PCL")
    TEST_CASE_REF(test_pcl_hw_find_mcu_success)
    TEST_CASE_REF(test_pcl_hw_find_mcu_not_found)
    TEST_CASE_REF(test_pcl_hw_find_mcu_null_parameters)
    TEST_CASE_REF(test_pcl_hw_get_mcu_success)
    TEST_CASE_REF(test_pcl_hw_get_mcu_invalid_id)
    TEST_CASE_REF(test_pcl_hw_find_bmc_success)
    TEST_CASE_REF(test_pcl_hw_find_satellite_success)
TEST_SUITE_END(test_pcl_hw_suite, "test_pcl_api", "PCL")

TEST_SUITE_BEGIN(test_pcl_validate_suite, "pcl_api", "PCL")
    TEST_CASE_REF(test_pcl_validate_success)
    TEST_CASE_REF(test_pcl_validate_null_pointer)
TEST_SUITE_END(test_pcl_validate_suite, "test_pcl_api", "PCL")
