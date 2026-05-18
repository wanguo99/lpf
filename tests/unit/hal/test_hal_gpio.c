/************************************************************************
 * HAL GPIO驱动单元测试
 ************************************************************************/

#include "test_framework.h"
#include "hal_gpio.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

/* 中断回调计数器 */
static volatile int interrupt_count = 0;
static volatile hal_gpio_level_t last_level = HAL_GPIO_LEVEL_LOW;

static void test_gpio_isr_callback(uint32_t gpio_num, hal_gpio_level_t level, void *user_data)
{
    (void)gpio_num;
    interrupt_count++;
    last_level = level;
    if (user_data) {
        *(int *)user_data = interrupt_count;
    }
}

TEST_CASE(test_gpio_init_deinit)
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

    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    /* Linux平台可能因权限问题失败 */
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_Deinit(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_CASE(test_gpio_set_get_direction)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 201;
    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    hal_gpio_direction_t dir;
    ret = HAL_GPIO_GetDirection(test_gpio, &dir);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_DIR_OUTPUT, dir);

    ret = HAL_GPIO_SetDirection(test_gpio, HAL_GPIO_DIR_INPUT);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_GetDirection(test_gpio, &dir);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_DIR_INPUT, dir);

    HAL_GPIO_Deinit(test_gpio);
}

TEST_CASE(test_gpio_set_get_level)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 202;
    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置高电平 */
    ret = HAL_GPIO_SetLevel(test_gpio, HAL_GPIO_LEVEL_HIGH);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    hal_gpio_level_t level;
    ret = HAL_GPIO_GetLevel(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_HIGH, level);

    /* 设置低电平 */
    ret = HAL_GPIO_SetLevel(test_gpio, HAL_GPIO_LEVEL_LOW);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_GetLevel(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_LOW, level);

    HAL_GPIO_Deinit(test_gpio);
}

TEST_CASE(test_gpio_interrupt_setup)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_BOTH,
        .callback = test_gpio_isr_callback,
        .user_data = NULL
    };

    uint32_t test_gpio = 203;
    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 禁用中断 */
    ret = HAL_GPIO_DisableInterrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 使能中断 */
    ret = HAL_GPIO_EnableInterrupt(test_gpio);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_Deinit(test_gpio);
}

TEST_CASE(test_gpio_invalid_params)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    /* 无效的GPIO号 */
    int32_t ret = HAL_GPIO_Init(999, &config);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL配置 */
    ret = HAL_GPIO_Init(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL方向指针 */
    ret = HAL_GPIO_GetDirection(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* NULL电平指针 */
    ret = HAL_GPIO_GetLevel(100, NULL);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

TEST_CASE(test_gpio_output_mode)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_OUTPUT,
        .initial_level = HAL_GPIO_LEVEL_HIGH,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 204;
    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证初始电平 */
    hal_gpio_level_t level;
    ret = HAL_GPIO_GetLevel(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_HIGH, level);

    /* 切换电平 */
    ret = HAL_GPIO_SetLevel(test_gpio, HAL_GPIO_LEVEL_LOW);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_GPIO_GetLevel(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(HAL_GPIO_LEVEL_LOW, level);

    HAL_GPIO_Deinit(test_gpio);
}

TEST_CASE(test_gpio_input_mode)
{
    hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIR_INPUT,
        .initial_level = HAL_GPIO_LEVEL_LOW,
        .edge = HAL_GPIO_EDGE_NONE,
        .callback = NULL,
        .user_data = NULL
    };

    uint32_t test_gpio = 205;
    int32_t ret = HAL_GPIO_Init(test_gpio, &config);

#ifdef __linux__
    if (ret == OSAL_ERR_PERMISSION) {
        TEST_MESSAGE("SKIPPED: Need permission to access GPIO");
        return;
    }
#endif

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 读取输入电平 */
    hal_gpio_level_t level;
    ret = HAL_GPIO_GetLevel(test_gpio, &level);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_GPIO_Deinit(test_gpio);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_hal_gpio, "HAL")
    TEST_CASE_REGISTER(test_gpio_init_deinit, "GPIO init and deinit")
    TEST_CASE_REGISTER(test_gpio_set_get_direction, "GPIO direction control")
    TEST_CASE_REGISTER(test_gpio_set_get_level, "GPIO level control")
    TEST_CASE_REGISTER(test_gpio_interrupt_setup, "GPIO interrupt setup")
    TEST_CASE_REGISTER(test_gpio_invalid_params, "GPIO invalid parameters")
    TEST_CASE_REGISTER(test_gpio_output_mode, "GPIO output mode")
    TEST_CASE_REGISTER(test_gpio_input_mode, "GPIO input mode")
TEST_MODULE_END(test_hal_gpio, "HAL")
