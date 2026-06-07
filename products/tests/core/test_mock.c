/**
 * @file test_mock.c
 * @brief Mock 框架实现
 */

#include "test_mock.h"

/* 全局 mock 注册表 */
test_mock_registry_t g_mock_registry = {0};

void test_mock_init(void)
{
    OSAL_memset(&g_mock_registry, 0, sizeof(test_mock_registry_t));
}

void test_mock_reset_all(void)
{
    uint32_t i;
    for (i = 0; i < g_mock_registry.mock_count; i++)
    {
        g_mock_registry.mocks[i].call_count = 0;
        g_mock_registry.mocks[i].return_value = 0;
        g_mock_registry.mocks[i].return_ptr = NULL;
        g_mock_registry.mocks[i].return_value_set = false;
        g_mock_registry.mocks[i].last_arg1 = NULL;
        g_mock_registry.mocks[i].last_arg2 = NULL;
        g_mock_registry.mocks[i].last_arg3 = NULL;
        g_mock_registry.mocks[i].last_arg4 = NULL;
    }
}

test_mock_state_t* test_mock_register(const char *func_name)
{
    test_mock_state_t *mock;

    if (g_mock_registry.mock_count >= TEST_MOCK_MAX_FUNCS)
    {
        return NULL;
    }

    /* 检查是否已注册 */
    mock = test_mock_get(func_name);
    if (mock)
    {
        return mock;
    }

    /* 注册新 mock */
    mock = &g_mock_registry.mocks[g_mock_registry.mock_count];
    mock->func_name = func_name;
    mock->call_count = 0;
    mock->return_value = 0;
    mock->return_ptr = NULL;
    mock->return_value_set = false;
    mock->last_arg1 = NULL;
    mock->last_arg2 = NULL;
    mock->last_arg3 = NULL;
    mock->last_arg4 = NULL;

    g_mock_registry.mock_count++;
    return mock;
}

test_mock_state_t* test_mock_get(const char *func_name)
{
    uint32_t i;

    for (i = 0; i < g_mock_registry.mock_count; i++)
    {
        if (OSAL_strcmp(g_mock_registry.mocks[i].func_name, func_name) == 0)
        {
            return &g_mock_registry.mocks[i];
        }
    }

    return NULL;
}
