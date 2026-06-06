/************************************************************************
 * PDL层 - Watchdog服务测试
 ************************************************************************/

#include "test_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"
#include "pdl.h"
#include "pdl_watchdog.h"

/**
 * @brief 测试：初始化和反初始化
 */
TEST_CASE(test_pdl_watchdog_init_deinit)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_MANUAL,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    ret = PDL_WATCHDOG_Deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：NULL参数检查
 */
TEST_CASE(test_pdl_watchdog_null_params)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_MANUAL,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    /* NULL config */
    int32_t ret = PDL_WATCHDOG_Init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL handle */
    ret = PDL_WATCHDOG_Init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：手动模式喂狗
 */
TEST_CASE(test_pdl_watchdog_manual_kick)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_MANUAL,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 手动喂狗 */
    ret = PDL_WATCHDOG_Kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 多次喂狗 */
    uint32_t i;
    for (i = 0; i < 5; i++)
    {
        ret = PDL_WATCHDOG_Kick(handle);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        OSAL_msleep(100);
    }

    /* 检查状态 */
    pdl_watchdog_status_t status;
    ret = PDL_WATCHDOG_GetStatus(handle, &status);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(6, status.kick_count);
    TEST_ASSERT_EQUAL(PDL_WATCHDOG_MODE_MANUAL, status.mode);

    PDL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：自动模式启动和停止
 */
TEST_CASE(test_pdl_watchdog_auto_mode)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_AUTO,
        .kick_interval_ms = 500,  /* 500ms间隔用于测试 */
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 启动自动喂狗 */
    ret = PDL_WATCHDOG_Start(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 等待一段时间让自动喂狗线程工作 */
    OSAL_msleep(1500);

    /* 检查状态 */
    pdl_watchdog_status_t status;
    ret = PDL_WATCHDOG_GetStatus(handle, &status);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(true, status.running);
    TEST_ASSERT(status.kick_count >= 2);  /* 至少喂了2次 */

    /* 停止自动喂狗 */
    ret = PDL_WATCHDOG_Stop(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 检查状态 */
    ret = PDL_WATCHDOG_GetStatus(handle, &status);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(false, status.running);

    PDL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：获取状态
 */
TEST_CASE(test_pdl_watchdog_get_status)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 60,
        .mode = PDL_WATCHDOG_MODE_MANUAL,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 获取状态 */
    pdl_watchdog_status_t status;
    ret = PDL_WATCHDOG_GetStatus(handle, &status);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(false, status.running);
    TEST_ASSERT_EQUAL(PDL_WATCHDOG_MODE_MANUAL, status.mode);
    TEST_ASSERT_EQUAL(5000, status.kick_interval_ms);
    TEST_ASSERT_EQUAL(0, status.kick_count);

    PDL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：设置喂狗间隔
 */
TEST_CASE(test_pdl_watchdog_set_interval)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_AUTO,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 设置新的间隔 */
    ret = PDL_WATCHDOG_SetInterval(handle, 3000);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证 */
    pdl_watchdog_status_t status;
    ret = PDL_WATCHDOG_GetStatus(handle, &status);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(3000, status.kick_interval_ms);

    PDL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：启用和禁用
 */
TEST_CASE(test_pdl_watchdog_enable_disable)
{
    pdl_watchdog_handle_t handle = NULL;
    pdl_watchdog_config_t config = {
        .name = "test_watchdog",
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .mode = PDL_WATCHDOG_MODE_MANUAL,
        .kick_interval_ms = 5000,
        .enable_on_init = false
    };

    int32_t ret = PDL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 启用 */
    ret = PDL_WATCHDOG_Enable(handle);
    if (ret == OSAL_SUCCESS)
    {
        pdl_watchdog_status_t status;
        ret = PDL_WATCHDOG_GetStatus(handle, &status);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(true, status.enabled);
    }

    PDL_WATCHDOG_Deinit(handle);
}

TEST_SUITE_BEGIN(test_pdl_watchdog, "pdl_watchdog", "PDL")
    TEST_CASE_REF(test_pdl_watchdog_init_deinit)
    TEST_CASE_REF(test_pdl_watchdog_null_params)
    TEST_CASE_REF(test_pdl_watchdog_manual_kick)
    TEST_CASE_REF(test_pdl_watchdog_auto_mode)
    TEST_CASE_REF(test_pdl_watchdog_get_status)
    TEST_CASE_REF(test_pdl_watchdog_set_interval)
    TEST_CASE_REF(test_pdl_watchdog_enable_disable)
TEST_SUITE_END(test_pdl_watchdog, "test_pdl_watchdog", "PDL")
