#include "test_framework.h"
/**
 * @file test_pcl_api.c
 * @brief PCL API单元测试
 *
 * 测试覆盖：
 * - 配置库初始化和清理
 * - 配置注册和查询
 * - 硬件外设配置查询（MCU/BMC/FPGA/Switch）
 * - 配置验证
 */

#include "api/pcl_api.h"
#include "osal.h"

/* 测试用的配置数据 */
static pcl_mcu_cfg_t test_mcu = {
    .name = "test_mcu",
    .description = "Test MCU",
    .enabled = true,
    .interface = PCL_MCU_INTERFACE_SERIAL,
    .serial = {
        .device = "/dev/ttyS0",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 'N'
    },
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = false,
    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static pcl_bmc_cfg_t test_bmc = {
    .name = "test_bmc",
    .description = "Test BMC",
    .enabled = true,

    /* 网络配置 */
    .network = {
        .enabled = true,
        .ip_addr = "192.168.1.100",
        .port = 623,
        .username = "admin",
        .password = "admin",
        .timeout_ms = 5000
    },

    /* 串口配置 */
    .serial = {
        .enabled = true,
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .timeout_ms = 5000
    },

    .primary_channel = PCL_BMC_CHANNEL_NETWORK,
    .auto_switch = true,
    .retry_count = 3,
    .health_check_interval = 5000,
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pcl_fpga_cfg_t test_fpga = {
    .name = "test_fpga",
    .description = "Test FPGA",
    .enabled = true,
    .interface_type = PCL_HW_INTERFACE_SPI,
    .interface_cfg = {
        .spi = {
            .device = "/dev/spidev0.0",
            .speed_hz = 1000000,
            .mode = 0,
            .bits_per_word = 8
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3
};

static pcl_switch_cfg_t test_switch = {
    .name = "test_switch",
    .description = "Test Switch",
    .enabled = true,
    .interface_type = PCL_HW_INTERFACE_I2C,
    .interface_cfg = {
        .i2c = {
            .device = "/dev/i2c-0",
            .slave_addr = 0x50,
            .speed_hz = 100000
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3
};

static pcl_mcu_cfg_t *test_mcu_arr[] = { &test_mcu, NULL };
static pcl_bmc_cfg_t *test_bmc_arr[] = { &test_bmc, NULL };
static pcl_fpga_cfg_t *test_fpga_arr[] = { &test_fpga, NULL };
static pcl_switch_cfg_t *test_switch_arr[] = { &test_switch, NULL };

static const pcl_platform_config_t test_platform_v1 = {
    .platform_name = "test_platform",
    .chip_name = "test_chip",
    .project_name = "test_project",
    .product_name = "test_product_v1",
    .mcu_arr = test_mcu_arr,
    .bmc_arr = test_bmc_arr,
    .fpga_arr = test_fpga_arr,
    .switch_arr = test_switch_arr
};

static const pcl_platform_config_t test_platform_v2 = {
    .platform_name = "test_platform",
    .chip_name = "test_chip",
    .project_name = "test_project",
    .product_name = "test_product_v2",
    .mcu_arr = test_mcu_arr,
    .bmc_arr = test_bmc_arr,
    .fpga_arr = NULL,
    .switch_arr = NULL
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

    int32_t ret = PCL_Register(&test_platform_v1);
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
    int32_t ret = PCL_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

TEST_CASE(test_pcl_register_duplicate)
{
    PCL_Init();

    int32_t ret = PCL_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 重复注册相同配置应该失败 */
    ret = PCL_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_register_multiple_versions)
{
    PCL_Init();

    int32_t ret = PCL_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 注册不同版本应该成功 */
    ret = PCL_Register(&test_platform_v2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCL_Cleanup();
}

/* ========== 配置查询测试 ========== */

TEST_CASE(test_pcl_find_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_platform_config_t *found = PCL_Find("test_platform", "test_product_v1", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_product_v1", found->product_name);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_without_version)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);
    PCL_Register(&test_platform_v2);

    /* 不指定版本应该返回第一个匹配的 */
    const pcl_platform_config_t *found = PCL_Find("test_platform", "test_product_v1", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_product_v1", found->product_name);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_not_found)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_platform_config_t *found = PCL_Find("nonexistent", "product", NULL);
    TEST_ASSERT_NULL(found);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_find_null_parameters)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_platform_config_t *found = PCL_Find(NULL, "test_product_v1", NULL);
    TEST_ASSERT_NULL(found);

    found = PCL_Find("test_platform", NULL, NULL);
    TEST_ASSERT_NULL(found);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_list_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);
    PCL_Register(&test_platform_v2);

    const pcl_platform_config_t *configs[10];
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

    const pcl_platform_config_t *configs[10];
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
    PCL_Register(&test_platform_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(&test_platform_v1, "test_mcu");
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);
    TEST_ASSERT_EQUAL(PCL_MCU_INTERFACE_SERIAL, mcu->interface);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_mcu_not_found)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(&test_platform_v1, "nonexistent");
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_mcu_null_parameters)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_FindMCU(NULL, "test_mcu");
    TEST_ASSERT_NULL(mcu);

    mcu = PCL_HW_FindMCU(&test_platform_v1, NULL);
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_get_mcu_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_GetMCU(&test_platform_v1, 0);
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_get_mcu_invalid_id)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_mcu_cfg_t *mcu = PCL_HW_GetMCU(&test_platform_v1, 999);
    TEST_ASSERT_NULL(mcu);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_bmc_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_bmc_cfg_t *bmc = PCL_HW_FindBMC(&test_platform_v1, "test_bmc");
    TEST_ASSERT_NOT_NULL(bmc);
    TEST_ASSERT_STRING_EQUAL("test_bmc", bmc->name);
    TEST_ASSERT_EQUAL(PCL_BMC_CHANNEL_NETWORK, bmc->primary_channel);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_fpga_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_fpga_cfg_t *fpga = PCL_HW_FindFPGA(&test_platform_v1, "test_fpga");
    TEST_ASSERT_NOT_NULL(fpga);
    TEST_ASSERT_STRING_EQUAL("test_fpga", fpga->name);
    TEST_ASSERT_EQUAL(PCL_HW_INTERFACE_SPI, fpga->interface_type);

    PCL_Cleanup();
}

TEST_CASE(test_pcl_hw_find_switch_success)
{
    PCL_Init();
    PCL_Register(&test_platform_v1);

    const pcl_switch_cfg_t *sw = PCL_HW_FindSwitch(&test_platform_v1, "test_switch");
    TEST_ASSERT_NOT_NULL(sw);
    TEST_ASSERT_STRING_EQUAL("test_switch", sw->name);
    TEST_ASSERT_EQUAL(PCL_HW_INTERFACE_I2C, sw->interface_type);

    PCL_Cleanup();
}

/* ========== 配置验证测试 ========== */

TEST_CASE(test_pcl_validate_success)
{
    int32_t ret = PCL_Validate(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_pcl_validate_null_pointer)
{
    int32_t ret = PCL_Validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/* ========== 测试套件注册 ========== */

TEST_MODULE_BEGIN(test_pcl_init_suite, "PCL")
    TEST_CASE_REF(test_pcl_init_success)
    TEST_CASE_REF(test_pcl_cleanup)
TEST_MODULE_END(test_pcl_init_suite, "PCL")

TEST_MODULE_BEGIN(test_pcl_register_suite, "PCL")
    TEST_CASE_REF(test_pcl_register_success)
    TEST_CASE_REF(test_pcl_register_null_pointer)
    TEST_CASE_REF(test_pcl_register_not_initialized)
    TEST_CASE_REF(test_pcl_register_duplicate)
    TEST_CASE_REF(test_pcl_register_multiple_versions)
TEST_MODULE_END(test_pcl_register_suite, "PCL")

TEST_MODULE_BEGIN(test_pcl_find_suite, "PCL")
    TEST_CASE_REF(test_pcl_find_success)
    TEST_CASE_REF(test_pcl_find_without_version)
    TEST_CASE_REF(test_pcl_find_not_found)
    TEST_CASE_REF(test_pcl_find_null_parameters)
    TEST_CASE_REF(test_pcl_list_success)
    TEST_CASE_REF(test_pcl_list_null_parameters)
TEST_MODULE_END(test_pcl_find_suite, "PCL")

TEST_MODULE_BEGIN(test_pcl_hw_suite, "PCL")
    TEST_CASE_REF(test_pcl_hw_find_mcu_success)
    TEST_CASE_REF(test_pcl_hw_find_mcu_not_found)
    TEST_CASE_REF(test_pcl_hw_find_mcu_null_parameters)
    TEST_CASE_REF(test_pcl_hw_get_mcu_success)
    TEST_CASE_REF(test_pcl_hw_get_mcu_invalid_id)
    TEST_CASE_REF(test_pcl_hw_find_bmc_success)
    TEST_CASE_REF(test_pcl_hw_find_fpga_success)
    TEST_CASE_REF(test_pcl_hw_find_switch_success)
TEST_MODULE_END(test_pcl_hw_suite, "PCL")

TEST_MODULE_BEGIN(test_pcl_validate_suite, "PCL")
    TEST_CASE_REF(test_pcl_validate_success)
    TEST_CASE_REF(test_pcl_validate_null_pointer)
TEST_MODULE_END(test_pcl_validate_suite, "PCL")
