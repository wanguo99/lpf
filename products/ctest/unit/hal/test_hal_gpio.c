/************************************************************************
 * HAL GPIO驱动单元测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "hal.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

/* 中断回调计数器 */
static volatile int32_t interrupt_count = 0;
static volatile hal_gpio_level_t last_level = HAL_GPIO_LEVEL_LOW;

static void test_gpio_isr_callback(uint32_t gpio_num, hal_gpio_level_t level, void *user_data)
{
    (void)gpio_num;
    interrupt_count++;
    last_level = level;
    if (user_data) {
        *(int32_t *)user_data = interrupt_count;
    }
}

static void test_gpio_init_deinit(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    /* 使用较大的GPIO号避免与系统GPIO冲突 */
    uint32_t test_gpio = 200;

    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    /* Linux平台可能因权限问题失败 */
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_deinit(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_gpio_set_get_direction(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 201;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    hal_gpio_direction_t dir;
    ret = HAL_GPIO_get_direction(test_gpio, &dir);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_DIR_OUTPUT, dir);

    ret = HAL_GPIO_set_direction(test_gpio, HAL_GPIO_DIR_INPUT);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_get_direction(test_gpio, &dir);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_DIR_INPUT, dir);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_set_get_level(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 202;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置高电平 */
    ret = HAL_GPIO_set_level(test_gpio, HAL_GPIO_LEVEL_HIGH);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    hal_gpio_level_t level;
    ret = HAL_GPIO_get_level(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_HIGH, level);

    /* 设置低电平 */
    ret = HAL_GPIO_set_level(test_gpio, HAL_GPIO_LEVEL_LOW);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_get_level(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_LOW, level);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_interrupt_setup(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_BOTH,
        .callback = test_gpio_isr_callback,
        .user_data = NULL
    };

    uint32_t test_gpio = 203;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 禁用中断 */
    ret = HAL_GPIO_disable_interrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 使能中断 */
    ret = HAL_GPIO_enable_interrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_invalid_params(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    /* 无效的GPIO号 */
    int32_t ret = HAL_GPIO_init(999, &config);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL配置 */
    ret = HAL_GPIO_init(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL方向指针 */
    ret = HAL_GPIO_get_direction(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL电平指针 */
    ret = HAL_GPIO_get_level(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

static void test_gpio_output_mode(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_HIGH,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 204;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证初始电平 */
    hal_gpio_level_t level;
    ret = HAL_GPIO_get_level(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_HIGH, level);

    /* 切换电平 */
    ret = HAL_GPIO_set_level(test_gpio, HAL_GPIO_LEVEL_LOW);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_get_level(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_LOW, level);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_input_mode(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 205;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 读取输入电平 */
    hal_gpio_level_t level;
    ret = HAL_GPIO_get_level(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_interrupt_enable_disable_edge_cases(void)
{
    uint32_t test_gpio = 206;

    /* 测试未初始化GPIO的中断操作 */
    int32_t ret = HAL_GPIO_enable_interrupt(test_gpio);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_disable_interrupt(test_gpio);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 初始化GPIO但不设置中断 */
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    ret = HAL_GPIO_init(test_gpio, &config);
#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 尝试使能未配置的中断 */
    ret = HAL_GPIO_enable_interrupt(test_gpio);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_interrupt_callback_verification(void)
{
    interrupt_count = 0;
    last_level = HAL_GPIO_LEVEL_LOW;
    int32_t user_counter = 0;

    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_BOTH,
        .callback = test_gpio_isr_callback,
        .user_data = &user_counter
    };

    uint32_t test_gpio = 207;
    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 使能中断 */
    ret = HAL_GPIO_enable_interrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证中断状态 - 初始应为0 */
    TEST_ASSERT_EQUAL(0, interrupt_count);
    TEST_ASSERT_EQUAL(0, user_counter);

    /* 禁用中断 */
    ret = HAL_GPIO_disable_interrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 再次禁用应该成功（幂等操作） */
    ret = HAL_GPIO_disable_interrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_set_interrupt_different_edges(void)
{
    uint32_t test_gpio = 208;

    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    int32_t ret = HAL_GPIO_init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试上升沿 */
    ret = HAL_GPIO_set_interrupt(test_gpio, HAL_GPIO_EDGE_RISING, test_gpio_isr_callback, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试下降沿 */
    ret = HAL_GPIO_set_interrupt(test_gpio, HAL_GPIO_EDGE_FALLING, test_gpio_isr_callback, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试双边沿 */
    ret = HAL_GPIO_set_interrupt(test_gpio, HAL_GPIO_EDGE_BOTH, test_gpio_isr_callback, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试禁用中断（callback为NULL） */
    ret = HAL_GPIO_set_interrupt(test_gpio, HAL_GPIO_EDGE_NONE, NULL, NULL);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_deinit(test_gpio);
}

static void test_gpio_boundary_values(void)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    /* 测试GPIO号边界值 */
    int32_t ret = HAL_GPIO_init(0, &config);
    if (ret != OSAL_ERR_PERMISSION && ret != OSAL_EINVAL) {
        HAL_GPIO_deinit(0);
    }

    /* 测试无效的大GPIO号 */
    ret = HAL_GPIO_init(999999, &config);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 测试无效方向值 */
    ret = HAL_GPIO_get_direction(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 测试无效电平值 */
    ret = HAL_GPIO_get_level(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_gpio_init_deinit",
		.func = test_gpio_init_deinit,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_set_get_direction",
		.func = test_gpio_set_get_direction,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_set_get_level",
		.func = test_gpio_set_get_level,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_interrupt_setup",
		.func = test_gpio_interrupt_setup,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_invalid_params",
		.func = test_gpio_invalid_params,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_output_mode",
		.func = test_gpio_output_mode,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_input_mode",
		.func = test_gpio_input_mode,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_interrupt_enable_disable_edge_cases",
		.func = test_gpio_interrupt_enable_disable_edge_cases,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_interrupt_callback_verification",
		.func = test_gpio_interrupt_callback_verification,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_set_interrupt_different_edges",
		.func = test_gpio_set_interrupt_different_edges,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_gpio_boundary_values",
		.func = test_gpio_boundary_values,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_gpio",
	.module_name = "hal_gpio",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "HAL hal_gpio tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_hal_gpio_tests(void)
{
	libutest_register_suite(&test_suite);
}
