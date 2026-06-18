#include <test_framework/test_framework.h>
/**
 * @file test_osal_cond.c
 * @brief OSAL条件变量单元测试 - Updated for osal_cond_t thin wrapper
 */

#include "osal.h"
#include <errno.h>

static int32_t shared_data = 0;
static bool data_ready = false;

/* 测试用例1: 条件变量初始化成功 */
static void _test_cond_init_success(void)
{
	osal_cond_t cond;
	int32_t ret = osal_pthread_cond_init(&cond, NULL);

	TEST_ASSERT_EQUAL(0, ret);

	osal_pthread_cond_destroy(&cond);
}

/* 测试用例2: 条件变量初始化失败 - 空指针 */
static void _test_cond_init_nullpointer(void)
{
	int32_t ret = osal_pthread_cond_init(NULL, NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例3: 条件变量销毁成功 */
static void _test_cond_destroy_success(void)
{
	osal_cond_t cond;
	osal_pthread_cond_init(&cond, NULL);

	int32_t ret = osal_pthread_cond_destroy(&cond);
	TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例4: 条件变量销毁失败 - 空指针 */
static void _test_cond_destroy_nullpointer(void)
{
	int32_t ret = osal_pthread_cond_destroy(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例5: 条件变量Signal失败 - 空指针 */
static void _test_cond_signal_nullpointer(void)
{
	int32_t ret = osal_pthread_cond_signal(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例6: 条件变量Broadcast失败 - 空指针 */
static void _test_cond_broadcast_nullpointer(void)
{
	int32_t ret = osal_pthread_cond_broadcast(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例7: 条件变量Wait失败 - 空指针 */
static void _test_cond_wait_nullpointer(void)
{
	osal_cond_t cond;
	osal_mutex_t mutex;

	osal_pthread_cond_init(&cond, NULL);
	osal_pthread_mutex_init(&mutex, NULL);

	/* cond 为 NULL */
	int32_t ret = osal_pthread_cond_wait(NULL, &mutex);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);

	/* mutex 为 NULL */
	ret = osal_pthread_cond_wait(&cond, NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);

	osal_pthread_cond_destroy(&cond);
	osal_pthread_mutex_destroy(&mutex);
}

/* 测试用例8: 条件变量超时等待 - 超时 */
static void _test_cond_timedwait_timeout(void)
{
	osal_cond_t cond;
	osal_mutex_t mutex;

	osal_pthread_cond_init(&cond, NULL);
	osal_pthread_mutex_init(&mutex, NULL);

	osal_pthread_mutex_lock(&mutex);
	int32_t ret = osal_pthread_cond_timedwait(&cond, &mutex, 100);
	osal_pthread_mutex_unlock(&mutex);

	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(ETIMEDOUT, errno);

	osal_pthread_cond_destroy(&cond);
	osal_pthread_mutex_destroy(&mutex);
}

/* 生产者线程 */
static void *_producer_thread(void *arg)
{
	osal_cond_t *cond = ((osal_cond_t **)arg)[0];
	osal_mutex_t *mutex = ((osal_mutex_t **)arg)[1];

	osal_msleep(50); /* 等待消费者先运行 */

	osal_pthread_mutex_lock(mutex);
	shared_data = 42;
	data_ready = true;
	osal_pthread_cond_signal(cond);
	osal_pthread_mutex_unlock(mutex);

	return NULL;
}

/* 消费者线程 */
static void *_consumer_thread(void *arg)
{
	osal_cond_t *cond = ((osal_cond_t **)arg)[0];
	osal_mutex_t *mutex = ((osal_mutex_t **)arg)[1];

	osal_pthread_mutex_lock(mutex);
	while (!data_ready) {
		osal_pthread_cond_wait(cond, mutex);
	}
	osal_pthread_mutex_unlock(mutex);

	return NULL;
}

/* 测试用例9: 条件变量Signal唤醒 */
static void _test_cond_signal_wakeup(void)
{
	shared_data = 0;
	data_ready = false;

	osal_cond_t cond;
	osal_mutex_t mutex;
	osal_pthread_cond_init(&cond, NULL);
	osal_pthread_mutex_init(&mutex, NULL);

	osal_thread_t producer, consumer;
	void *args[] = { &cond, &mutex };

	/* 创建生产者和消费者线程 */
	osal_pthread_create(&consumer, NULL, _consumer_thread, args);
	osal_pthread_create(&producer, NULL, _producer_thread, args);

	/* 等待线程完成 */
	osal_pthread_join(producer, NULL);
	osal_pthread_join(consumer, NULL);

	/* 验证数据已传递 */
	TEST_ASSERT_EQUAL(42, shared_data);
	TEST_ASSERT_TRUE(data_ready);

	osal_pthread_cond_destroy(&cond);
	osal_pthread_mutex_destroy(&mutex);
}

/* 多个等待线程 */
static void *_wait_thread(void *arg)
{
	osal_cond_t *cond = ((osal_cond_t **)arg)[0];
	osal_mutex_t *mutex = ((osal_mutex_t **)arg)[1];

	osal_pthread_mutex_lock(mutex);
	while (!data_ready) {
		osal_pthread_cond_wait(cond, mutex);
	}
	shared_data++; /* 每个线程增加计数 */
	osal_pthread_mutex_unlock(mutex);

	return NULL;
}

/* 广播线程 */
static void *_broadcast_thread(void *arg)
{
	osal_cond_t *cond = ((osal_cond_t **)arg)[0];
	osal_mutex_t *mutex = ((osal_mutex_t **)arg)[1];

	osal_msleep(100); /* 等待所有等待线程就绪 */

	osal_pthread_mutex_lock(mutex);
	data_ready = true;
	osal_pthread_cond_broadcast(cond); /* 唤醒所有等待线程 */
	osal_pthread_mutex_unlock(mutex);

	return NULL;
}

/* 测试用例10: 条件变量Broadcast唤醒多个线程 */
static void _test_cond_broadcast_wakeup(void)
{
	shared_data = 0;
	data_ready = false;

	osal_cond_t cond;
	osal_mutex_t mutex;
	osal_pthread_cond_init(&cond, NULL);
	osal_pthread_mutex_init(&mutex, NULL);

	osal_thread_t threads[5];
	void *args[] = { &cond, &mutex };
	int32_t i;

	/* 创建多个等待线程 */
	for (i = 0; i < 4; i++) {
		osal_pthread_create(&threads[i], NULL, _wait_thread, args);
	}

	/* 创建广播线程 */
	osal_pthread_create(&threads[4], NULL, _broadcast_thread, args);

	/* 等待所有线程完成 */
	for (i = 0; i < 5; i++) {
		osal_pthread_join(threads[i], NULL);
	}

	/* 验证所有等待线程都被唤醒 */
	TEST_ASSERT_EQUAL(4, shared_data);

	osal_pthread_cond_destroy(&cond);
	osal_pthread_mutex_destroy(&mutex);
}

/* 注册测试套件 */

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_cond_init_success",
	  .func = _test_cond_init_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_init_nullpointer",
	  .func = _test_cond_init_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_destroy_success",
	  .func = _test_cond_destroy_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_destroy_nullpointer",
	  .func = _test_cond_destroy_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_signal_nullpointer",
	  .func = _test_cond_signal_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_broadcast_nullpointer",
	  .func = _test_cond_broadcast_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_wait_nullpointer",
	  .func = _test_cond_wait_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_timedwait_timeout",
	  .func = _test_cond_timedwait_timeout,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_signal_wakeup",
	  .func = _test_cond_signal_wakeup,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_cond_broadcast_wakeup",
	  .func = _test_cond_broadcast_wakeup,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_cond",
	.module_name = "osal_cond",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "OSAL cond tests (osal_cond_t thin wrapper)" }
};

/* 测试套件注册函数 */
void register_osal_cond_tests(void)
{
	libutest_register_suite(&test_suite);
}
