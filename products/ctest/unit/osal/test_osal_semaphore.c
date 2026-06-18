#include <test_framework/test_framework.h>
/**
 * @file test_osal_semaphore.c
 * @brief OSAL信号量单元测试 - 使用统一 OSAL API
 *
 * 使用新的libtest框架，测试自动注册
 */

#include "osal.h"
#include <errno.h>

static int32_t shared_counter = 0;

/* 测试用例1: 信号量初始化成功 */
static void _test_semaphore_init_success(void)
{
	osal_sem_t sem;

	int32_t ret = osal_sem_init(&sem, 0, 1);

	TEST_ASSERT_EQUAL(0, ret);

	osal_sem_destroy(&sem);
}

/* 测试用例2: 信号量初始化失败 - 空指针 */
static void _test_semaphore_init_nullpointer(void)
{
	int32_t ret = osal_sem_init(NULL, 0, 1);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例3: 信号量等待和释放 */
static void _test_semaphore_wait_post_success(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 1);

	int32_t ret = osal_sem_wait(&sem);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_sem_post(&sem);
	TEST_ASSERT_EQUAL(0, ret);

	osal_sem_destroy(&sem);
}

/* 测试用例4: 信号量等待失败 - 空指针 */
static void _test_semaphore_wait_nullpointer(void)
{
	int32_t ret = osal_sem_wait(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例5: 信号量释放失败 - 空指针 */
static void _test_semaphore_post_nullpointer(void)
{
	int32_t ret = osal_sem_post(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例6: 信号量超时等待 - 超时 */
static void _test_semaphore_timedwait_timeout(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 0);

	int32_t ret = osal_sem_timed_wait(&sem, 100);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(ETIMEDOUT, errno);

	osal_sem_destroy(&sem);
}

/* 测试用例7: 信号量超时等待 - 成功 */
static void _test_semaphore_timedwait_success(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 1);

	int32_t ret = osal_sem_timed_wait(&sem, 100);
	TEST_ASSERT_EQUAL(0, ret);

	osal_sem_destroy(&sem);
}

/* 测试用例8: 信号量非阻塞等待 - 失败（信号量为0）*/
static void _test_semaphore_trywait_fail(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 0);

	int32_t ret = osal_sem_try_wait(&sem);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);

	osal_sem_destroy(&sem);
}

/* 测试用例9: 信号量非阻塞等待 - 成功 */
static void _test_semaphore_trywait_success(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 1);

	int32_t ret = osal_sem_try_wait(&sem);
	TEST_ASSERT_EQUAL(0, ret);

	osal_sem_destroy(&sem);
}

/* 测试用例10: 信号量获取值 */
static void _test_semaphore_getvalue(void)
{
	osal_sem_t sem;
	int32_t value;

	osal_sem_init(&sem, 0, 5);

	int32_t ret = osal_sem_get_value(&sem, &value);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(5, value);

	osal_sem_destroy(&sem);
}

/* 测试用例11: 信号量销毁 */
static void _test_semaphore_destroy_success(void)
{
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 1);

	int32_t ret = osal_sem_destroy(&sem);
	TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例12: 信号量销毁失败 - 空指针 */
static void _test_semaphore_destroy_nullpointer(void)
{
	int32_t ret = osal_sem_destroy(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 生产者线程 */
static void *_producer_thread(void *arg)
{
	osal_sem_t *sem = (osal_sem_t *)arg;

	int32_t i;

	for (i = 0; i < 10; i++) {
		osal_msleep(10);
		shared_counter++;
		osal_sem_post(sem);
	}

	return NULL;
}

/* 消费者线程 */
static void *_consumer_thread(void *arg)
{
	osal_sem_t *sem = (osal_sem_t *)arg;

	int32_t i;

	for (i = 0; i < 10; i++) {
		osal_sem_wait(sem);
		shared_counter--;
	}

	return NULL;
}

/* 测试用例13: 信号量生产者-消费者模式 */
static void _test_semaphore_producer_consumer(void)
{
	shared_counter = 0;
	osal_sem_t sem;
	osal_sem_init(&sem, 0, 0);

	osal_thread_t producer, consumer;

	/* 创建生产者和消费者线程 */
	osal_thread_create(&producer, NULL, _producer_thread, &sem);
	osal_thread_create(&consumer, NULL, _consumer_thread, &sem);

	/* 等待线程完成 */
	osal_thread_join(producer, NULL);
	osal_thread_join(consumer, NULL);

	/* 验证计数器归零 */
	TEST_ASSERT_EQUAL(0, shared_counter);

	osal_sem_destroy(&sem);
}

/* 注册测试套件 - 自动注册 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_semaphore_init_success",
	  .func = _test_semaphore_init_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_init_nullpointer",
	  .func = _test_semaphore_init_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_wait_post_success",
	  .func = _test_semaphore_wait_post_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_wait_nullpointer",
	  .func = _test_semaphore_wait_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_post_nullpointer",
	  .func = _test_semaphore_post_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_timedwait_timeout",
	  .func = _test_semaphore_timedwait_timeout,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_timedwait_success",
	  .func = _test_semaphore_timedwait_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_trywait_fail",
	  .func = _test_semaphore_trywait_fail,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_trywait_success",
	  .func = _test_semaphore_trywait_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_getvalue",
	  .func = _test_semaphore_getvalue,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_destroy_success",
	  .func = _test_semaphore_destroy_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_destroy_nullpointer",
	  .func = _test_semaphore_destroy_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_producer_consumer",
	  .func = _test_semaphore_producer_consumer,
	  .setup = NULL,
	  .teardown = NULL },
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
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "OSAL semaphore tests (POSIX thin wrapper)" }
};

/* 测试套件注册函数 */
void register_osal_semaphore_tests(void)
{
	libutest_register_suite(&test_suite);
}
