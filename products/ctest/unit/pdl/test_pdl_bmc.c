/**
 * @file test_pdl_bmc.c
 * @brief PDL BMC通信服务单元测试
 *
 * 注意：这些测试需要在 PCONFIG 中预先配置 BMC 设备
 */

#include <test_framework/test_framework.h>
#include "osal.h"
#include "pdl.h"
#include "pconfig.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: BMC服务初始化 - 有效索引 */
static void test_pdl_bmc_init_valid_index(void)
{
    pdl_bmc_handle_t handle = NULL;

    /* 使用索引 0 初始化（假设 PCONFIG 中已配置） */
    int32_t ret = PDL_BMC_init(0, &handle);

    /* 如果配置存在且启用，应该成功 */
    if (ret == OSAL_SUCCESS) {
        TEST_ASSERT_NOT_NULL(handle);
        PDL_BMC_deinit(handle);
    } else {
        /* 配置不存在或未启用，跳过测试 */
        TEST_MESSAGE("SKIPPED: BMC index 0 not configured in PCONFIG");
    }
}

/* 测试用例: BMC服务初始化 - 空句柄指针 */
static void test_pdl_bmc_init_null_handle(void)
{
    int32_t ret = PDL_BMC_init(0, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务初始化 - 无效索引 */
static void test_pdl_bmc_init_invalid_index(void)
{
    pdl_bmc_handle_t handle = NULL;

    /* 使用一个很大的无效索引 */
    int32_t ret = PDL_BMC_init(999, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: BMC服务清理 */
static void test_pdl_bmc_deinit(void)
{
    pdl_bmc_handle_t handle = NULL;

    int32_t ret = PDL_BMC_init(0, &handle);
    if (ret == OSAL_SUCCESS) {
        ret = PDL_BMC_deinit(handle);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    } else {
        TEST_MESSAGE("SKIPPED: BMC index 0 not configured");
    }
}

/* 测试用例: 清理空句柄 */
static void test_pdl_bmc_deinit_null_handle(void)
{
    int32_t ret = PDL_BMC_deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 电源控制测试
 *===========================================================================*/

/* 测试用例: 电源操作 - 空句柄 */
static void test_pdl_bmc_power_on_null_handle(void)
{
    int32_t ret = PDL_BMC_power_on(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pdl_bmc_power_off_null_handle(void)
{
    int32_t ret = PDL_BMC_power_off(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pdl_bmc_power_reset_null_handle(void)
{
    int32_t ret = PDL_BMC_power_reset(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 测试套件入口
 *===========================================================================*/

void test_pdl_bmc(void)
{
    test_pdl_bmc_init_valid_index();
    test_pdl_bmc_init_null_handle();
    test_pdl_bmc_init_invalid_index();
    test_pdl_bmc_deinit();
    test_pdl_bmc_deinit_null_handle();

    test_pdl_bmc_power_on_null_handle();
    test_pdl_bmc_power_off_null_handle();
    test_pdl_bmc_power_reset_null_handle();
}
