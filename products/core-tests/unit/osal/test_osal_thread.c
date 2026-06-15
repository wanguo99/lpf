/************************************************************************
 * OSAL线程管理单元测试
 ************************************************************************/

#include "osal.h"
#include <test/test_framework.h>

#include <pthread.h>

/*===========================================================================
 * 测试辅助函数
 *===========================================================================*/

static void *simple_thread_func(void *arg)
{
    int32_t *result = (int32_t *)arg;
    *result = 42;
    return arg;
}

static void *sleep_thread_func(void *arg)
{
    OSAL_msleep(100);
    return arg;
}

/*===========================================================================
 * 线程创建和销毁测试
 *===========================================================================*/

static void test_thread_create_join_success(void)
{
    osal_thread_t thread;
    int32_t result = 0;
    void *retval = NULL;

    /* 创建线程 */
    int32_t ret = OSAL_pthread_create(&thread, NULL, simple_thread_func, &result);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待线程结束 */
    ret = OSAL_pthread_join(thread, &retval);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(42, result);
    TEST_ASSERT_EQUAL(&result, retval);
}

static void test_thread_create_null_pointer(void)
{
    /* 线程指针为NULL应该失败 */
    int32_t ret = OSAL_pthread_create(NULL, NULL, simple_thread_func, NULL);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

static void test_thread_detach_success(void)
{
    osal_thread_t thread;
    int32_t dummy = 0;

    /* 创建线程 */
    int32_t ret = OSAL_pthread_create(&thread, NULL, sleep_thread_func, &dummy);
    TEST_ASSERT_EQUAL(0, ret);

    /* 分离线程 */
    ret = OSAL_pthread_detach(thread);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待线程完成 */
    OSAL_msleep(200);
}

static void test_thread_self(void)
{
    /* 获取当前线程ID */
    osal_thread_t self = OSAL_pthread_self();

    /* 再次获取应该相同 */
    osal_thread_t self2 = OSAL_pthread_self();
    TEST_ASSERT_TRUE(pthread_equal(self, self2));
}

static void test_thread_equal(void)
{
    osal_thread_t self = OSAL_pthread_self();
    osal_thread_t thread;
    int32_t dummy = 0;

    /* 创建另一个线程 */
    int32_t ret = OSAL_pthread_create(&thread, NULL, sleep_thread_func, &dummy);
    TEST_ASSERT_EQUAL(0, ret);

    /* 两个线程ID应该不同 */
    TEST_ASSERT_FALSE(pthread_equal(self, thread));

    /* 同一个线程ID应该相等 */
    TEST_ASSERT_TRUE(pthread_equal(self, self));

    /* 清理 */
    OSAL_pthread_detach(thread);
    OSAL_msleep(200);
}

/*===========================================================================
 * 线程属性测试
 *===========================================================================*/

static void test_thread_attr_init_destroy(void)
{
    osal_threadattr_t attr;

    /* 初始化属性 */
    int32_t ret = OSAL_pthread_attr_init(&attr);
    TEST_ASSERT_EQUAL(0, ret);

    /* 销毁属性 */
    ret = OSAL_pthread_attr_destroy(&attr);
    TEST_ASSERT_EQUAL(0, ret);
}

static void test_thread_attr_detachstate(void)
{
    osal_threadattr_t attr;
    int32_t state;

    OSAL_pthread_attr_init(&attr);

    /* 设置为分离状态 */
    int32_t ret = OSAL_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    TEST_ASSERT_EQUAL(0, ret);

    /* 获取状态 */
    ret = OSAL_pthread_attr_getdetachstate(&attr, &state);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(PTHREAD_CREATE_DETACHED, state);

    OSAL_pthread_attr_destroy(&attr);
}

/*===========================================================================
 * 多线程并发测试
 *===========================================================================*/

static int32_t shared_counter = 0;

static void *increment_thread_func(void *arg)
{
    int32_t count = *(int32_t *)arg;
    for (int32_t i = 0; i < count; i++) {
        __sync_fetch_and_add(&shared_counter, 1);
    }
    return NULL;
}

static void test_thread_multithread_execution(void)
{
    const int32_t NUM_THREADS = 4;
    const int32_t INCREMENTS_PER_THREAD = 1000;
    osal_thread_t threads[NUM_THREADS];
    int32_t count = INCREMENTS_PER_THREAD;

    shared_counter = 0;

    /* 创建多个线程 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        int32_t ret = OSAL_pthread_create(&threads[i], NULL, increment_thread_func, &count);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程完成 */
    for (int32_t i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证结果 */
    TEST_ASSERT_EQUAL(NUM_THREADS * INCREMENTS_PER_THREAD, shared_counter);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_thread_create_join_success",
        .func = test_thread_create_join_success,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_create_null_pointer",
        .func = test_thread_create_null_pointer,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_detach_success",
        .func = test_thread_detach_success,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_self",
        .func = test_thread_self,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_equal",
        .func = test_thread_equal,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_attr_init_destroy",
        .func = test_thread_attr_init_destroy,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_attr_detachstate",
        .func = test_thread_attr_detachstate,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_thread_multithread_execution",
        .func = test_thread_multithread_execution,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "osal_thread",
    .module_name = "osal_thread",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 1000,
        .description = "OSAL thread management tests"
    }
};

__attribute__((constructor))
static void register_osal_thread_tests(void)
{
    libutest_register_suite(&test_suite);
}
