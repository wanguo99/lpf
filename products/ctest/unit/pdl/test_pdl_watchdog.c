/************************************************************************
 * PDL层 - Watchdog服务测试
 ************************************************************************/

#include "test_core.h"
#include <test_framework/test_assert.h>
#include "test_registry.h"
#include "osal.h"
#include "pdl.h"
#include "pdl_watchdog.h"

/**
 * @brief 测试：初始化和反初始化
 */
static void test_pdl_watchdog_init_deinit(void)
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
static void test_pdl_watchdog_null_params(void)
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
static void test_pdl_watchdog_manual_kick(void)
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
static void test_pdl_watchdog_auto_mode(void)
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
static void test_pdl_watchdog_get_status(void)
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
static void test_pdl_watchdog_set_interval(void)
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
static void test_pdl_watchdog_enable_disable(void)
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

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_pdl_watchdog_init_deinit",
		.func = test_pdl_watchdog_init_deinit,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_null_params",
		.func = test_pdl_watchdog_null_params,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_manual_kick",
		.func = test_pdl_watchdog_manual_kick,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_auto_mode",
		.func = test_pdl_watchdog_auto_mode,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_get_status",
		.func = test_pdl_watchdog_get_status,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_set_interval",
		.func = test_pdl_watchdog_set_interval,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pdl_watchdog_enable_disable",
		.func = test_pdl_watchdog_enable_disable,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "pdl_watchdog",
	.module_name = "pdl_watchdog",
	.layer_name = "PDL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PDL pdl_watchdog tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_pdl_watchdog_tests(void)
{
	libutest_register_suite(&test_suite);
}
