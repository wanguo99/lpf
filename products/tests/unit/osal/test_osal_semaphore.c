#include "test_framework.h"
/**
 * @file test_osal_semaphore.c
 * @brief OSAL信号量单元测试 - 使用新的 POSIX 薄封装 API
 *
 * 使用新的libtest框架，测试自动注册
 */

#include "osal.h"
#include <errno.h>

static int32_t shared_counter = 0;

/* 测试用例1: 信号量初始化成功 */
static void test_semaphore_init_success(void)
{
    sem_t sem;

    int32_t ret = OSAL_sem_init(&sem, 0, 1);

    TEST_ASSERT_EQUAL(0, ret);

    OSAL_sem_destroy(&sem);
}

/* 测试用例2: 信号量初始化失败 - 空指针 */
static void test_semaphore_init_nullpointer(void)
{
    int32_t ret = OSAL_sem_init(NULL, 0, 1);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例3: 信号量等待和释放 */
static void test_semaphore_wait_post_success(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 1);

    int32_t ret = OSAL_sem_wait(&sem);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_sem_post(&sem);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_sem_destroy(&sem);
}

/* 测试用例4: 信号量等待失败 - 空指针 */
static void test_semaphore_wait_nullpointer(void)
{
    int32_t ret = OSAL_sem_wait(NULL);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例5: 信号量释放失败 - 空指针 */
static void test_semaphore_post_nullpointer(void)
{
    int32_t ret = OSAL_sem_post(NULL);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例6: 信号量超时等待 - 超时 */
static void test_semaphore_timedwait_timeout(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 0);

    int32_t ret = OSAL_sem_timedwait(&sem, 100);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(ETIMEDOUT, errno);

    OSAL_sem_destroy(&sem);
}

/* 测试用例7: 信号量超时等待 - 成功 */
static void test_semaphore_timedwait_success(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 1);

    int32_t ret = OSAL_sem_timedwait(&sem, 100);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_sem_destroy(&sem);
}

/* 测试用例8: 信号量非阻塞等待 - 失败（信号量为0）*/
static void test_semaphore_trywait_fail(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 0);

    int32_t ret = OSAL_sem_trywait(&sem);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);

    OSAL_sem_destroy(&sem);
}

/* 测试用例9: 信号量非阻塞等待 - 成功 */
static void test_semaphore_trywait_success(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 1);

    int32_t ret = OSAL_sem_trywait(&sem);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_sem_destroy(&sem);
}

/* 测试用例10: 信号量获取值 */
static void test_semaphore_getvalue(void)
{
    sem_t sem;
    int32_t value;

    OSAL_sem_init(&sem, 0, 5);

    int32_t ret = OSAL_sem_getvalue(&sem, &value);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(5, value);

    OSAL_sem_destroy(&sem);
}

/* 测试用例11: 信号量销毁 */
static void test_semaphore_destroy_success(void)
{
    sem_t sem;
    OSAL_sem_init(&sem, 0, 1);

    int32_t ret = OSAL_sem_destroy(&sem);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例12: 信号量销毁失败 - 空指针 */
static void test_semaphore_destroy_nullpointer(void)
{
    int32_t ret = OSAL_sem_destroy(NULL);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 生产者线程 */
static void* producer_thread(void *arg)
{
    sem_t *sem = (sem_t *)arg;

    int32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_msleep(10);
        shared_counter++;
        OSAL_sem_post(sem);
    }

    return NULL;
}

/* 消费者线程 */
static void* consumer_thread(void *arg)
{
    sem_t *sem = (sem_t *)arg;

    int32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_sem_wait(sem);
        shared_counter--;
    }

    return NULL;
}

/* 测试用例13: 信号量生产者-消费者模式 */
static void test_semaphore_producer_consumer(void)
{
    shared_counter = 0;
    sem_t sem;
    OSAL_sem_init(&sem, 0, 0);

    osal_thread_t producer, consumer;

    /* 创建生产者和消费者线程 */
    OSAL_ThreadCreate(&producer, producer_thread, &sem);
    OSAL_ThreadCreate(&consumer, consumer_thread, &sem);

    /* 等待线程完成 */
    OSAL_ThreadJoin(producer);
    OSAL_ThreadJoin(consumer);

    /* 验证计数器归零 */
    TEST_ASSERT_EQUAL(0, shared_counter);

    OSAL_sem_destroy(&sem);
}

/* 注册测试套件 - 自动注册 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_semaphore_init_success",
		.func = test_semaphore_init_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_init_nullpointer",
		.func = test_semaphore_init_nullpointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_wait_post_success",
		.func = test_semaphore_wait_post_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_wait_nullpointer",
		.func = test_semaphore_wait_nullpointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_post_nullpointer",
		.func = test_semaphore_post_nullpointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_timedwait_timeout",
		.func = test_semaphore_timedwait_timeout,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_timedwait_success",
		.func = test_semaphore_timedwait_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_trywait_fail",
		.func = test_semaphore_trywait_fail,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_trywait_success",
		.func = test_semaphore_trywait_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_getvalue",
		.func = test_semaphore_getvalue,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_destroy_success",
		.func = test_semaphore_destroy_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_destroy_nullpointer",
		.func = test_semaphore_destroy_nullpointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_semaphore_producer_consumer",
		.func = test_semaphore_producer_consumer,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_semaphore",
	.module_name = "osal_semaphore",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "OSAL semaphore tests (POSIX thin wrapper)"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_osal_semaphore_tests(void)
{
	libutest_register_suite(&test_suite);
}
