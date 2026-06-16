/************************************************************************
 * HAL层 - Watchdog驱动测试
 ************************************************************************/

#include "test_core.h"
#include <test_framework/test_assert.h>
#include "test_registry.h"
#include "hal.h"
#include "osal.h"

/**
 * @brief 测试：初始化和反初始化
 */
static void test_hal_watchdog_init_deinit(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);

    /* 如果设备不存在，跳过测试 */
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    ret = HAL_WATCHDOG_deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：NULL参数检查
 */
static void test_hal_watchdog_null_params(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    /* NULL config */
    int32_t ret = HAL_WATCHDOG_init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL handle */
    ret = HAL_WATCHDOG_init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL device */
    config.device = NULL;
    ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：喂狗功能
 */
static void test_hal_watchdog_kick(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 喂狗 */
    ret = HAL_WATCHDOG_kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 多次喂狗 */
    uint32_t i;
    for (i = 0; i < 5; i++)
    {
        ret = HAL_WATCHDOG_kick(handle);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        OSAL_msleep(100);
    }

    /* 检查统计信息 */
    uint32_t kick_count = 0;
    ret = HAL_WATCHDOG_get_stats(handle, &kick_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(6, kick_count);

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：超时时间设置和获取
 */
static void test_hal_watchdog_timeout(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 0,  /* 使用默认值 */
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 获取当前超时时间 */
    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_get_timeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    LOG_INFO("TEST", "Current timeout: %u seconds", timeout);

    /* 尝试设置新的超时时间（某些硬件可能不支持） */
    ret = HAL_WATCHDOG_set_timeout(handle, 60);
    if (ret == OSAL_ERR_NOT_SUPPORTED) {
        LOG_INFO("TEST", "Hardware does not support setting timeout dynamically");
        HAL_WATCHDOG_deinit(handle);
        return;
    }
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证设置 */
    ret = HAL_WATCHDOG_get_timeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(60, timeout);

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：获取剩余时间
 */
static void test_hal_watchdog_timeleft(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 喂狗 */
    ret = HAL_WATCHDOG_kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取剩余时间（某些硬件可能不支持） */
    uint32_t timeleft = 0;
    ret = HAL_WATCHDOG_get_timeleft(handle, &timeleft);
    if (ret == OSAL_SUCCESS)
    {
        LOG_INFO("TEST", "Time left: %u seconds", timeleft);
        TEST_ASSERT(timeleft > 0 && timeleft <= 30);
    }
    else if (ret == OSAL_ERR_NOT_SUPPORTED)
    {
        LOG_WARN("TEST", "GetTimeleft not supported by hardware");
    }

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：启用和禁用看门狗
 */
static void test_hal_watchdog_enable_disable(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 启用看门狗 */
    ret = HAL_WATCHDOG_enable(handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_WARN("TEST", "Enable not supported, skipping");
        HAL_WATCHDOG_deinit(handle);
        return;
    }

    /* 喂狗 */
    ret = HAL_WATCHDOG_kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 尝试禁用（某些硬件可能不支持） */
    ret = HAL_WATCHDOG_disable(handle);
    if (ret == OSAL_ERR_NOT_SUPPORTED)
    {
        LOG_WARN("TEST", "Disable not supported by hardware");
    }

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：无效句柄
 */
static void test_hal_watchdog_invalid_handle(void)
{
    int32_t ret = HAL_WATCHDOG_kick(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_WATCHDOG_deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_get_timeout(NULL, &timeout);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：GetStats详细验证
 */
static void test_hal_watchdog_getstats_verification(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);

    /* 初始统计应为0 */
    uint32_t kick_count = 999;
    ret = HAL_WATCHDOG_get_stats(handle, &kick_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, kick_count);

    /* 喂狗后统计应递增 */
    for (uint32_t i = 1; i <= 10; i++) {
        ret = HAL_WATCHDOG_kick(handle);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        ret = HAL_WATCHDOG_get_stats(handle, &kick_count);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(i, kick_count);
    }

    /* NULL参数测试 */
    ret = HAL_WATCHDOG_get_stats(NULL, &kick_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_WATCHDOG_get_stats(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：边界值 - 最小超时时间
 */
static void test_hal_watchdog_min_timeout(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 1,  /* 最小超时 */
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);

    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_get_timeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT(timeout >= 1);

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：边界值 - 最大超时时间
 */
static void test_hal_watchdog_max_timeout(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 300,  /* 较大超时 */
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    if (ret != OSAL_SUCCESS) {
        TEST_MESSAGE("SKIPPED: Max timeout not supported");
        return;
    }

    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_get_timeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_WATCHDOG_deinit(handle);
}

/**
 * @brief 测试：重复初始化
 */
static void test_hal_watchdog_double_init(void)
{
    hal_watchdog_handle_t handle1 = NULL;
    hal_watchdog_handle_t handle2 = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle1);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);

    /* 尝试再次初始化同一设备 */
    ret = HAL_WATCHDOG_init(&config, &handle2);
    /* 某些实现可能允许，某些可能拒绝 */
    if (ret == OSAL_SUCCESS) {
        HAL_WATCHDOG_deinit(handle2);
    }

    HAL_WATCHDOG_deinit(handle1);
}

/**
 * @brief 测试：重复反初始化
 */
static void test_hal_watchdog_double_deinit(void)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS);

    ret = HAL_WATCHDOG_deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 再次反初始化应失败 */
    ret = HAL_WATCHDOG_deinit(handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_hal_watchdog_init_deinit",
		.func = test_hal_watchdog_init_deinit,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_null_params",
		.func = test_hal_watchdog_null_params,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_kick",
		.func = test_hal_watchdog_kick,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_timeout",
		.func = test_hal_watchdog_timeout,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_timeleft",
		.func = test_hal_watchdog_timeleft,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_enable_disable",
		.func = test_hal_watchdog_enable_disable,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_invalid_handle",
		.func = test_hal_watchdog_invalid_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_getstats_verification",
		.func = test_hal_watchdog_getstats_verification,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_min_timeout",
		.func = test_hal_watchdog_min_timeout,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_max_timeout",
		.func = test_hal_watchdog_max_timeout,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_double_init",
		.func = test_hal_watchdog_double_init,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_watchdog_double_deinit",
		.func = test_hal_watchdog_double_deinit,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_watchdog",
	.module_name = "hal_watchdog",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "HAL hal_watchdog tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_hal_watchdog_tests(void)
{
	libutest_register_suite(&test_suite);
}
