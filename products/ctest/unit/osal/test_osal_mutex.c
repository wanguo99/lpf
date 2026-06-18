/**
 * @file test_osal_mutex.c
 * @brief OSAL Mutex Unit Tests - Updated for osal_mutex_t thin wrapper
 *
 * Tests OSAL mutex operations using function pointer array registration.
 *
 * Test Coverage:
 * - Mutex initialization and destruction
 * - Mutex locking and unlocking
 * - Null pointer handling
 * - Multi-threaded synchronization
 */

#include <test_framework/test_framework.h>
#include "osal.h"
#include <errno.h>

static int32_t shared_counter = 0;

/* 测试用例1: 互斥锁初始化成功 */
static void _test_mutex_init_success(void)
{
	osal_mutex_t mutex;

	int32_t ret = osal_mutex_init(&mutex, NULL);

	TEST_ASSERT_EQUAL(0, ret);

	osal_mutex_destroy(&mutex);
}

/* 测试用例2: 互斥锁初始化失败 - 空指针 */
static void _test_mutex_init_nullpointer(void)
{
	int32_t ret = osal_mutex_init(NULL, NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例3: 互斥锁加锁解锁 */
static void _test_mutex_lockunlock_success(void)
{
	osal_mutex_t mutex;
	osal_mutex_init(&mutex, NULL);

	int32_t ret = osal_mutex_lock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_unlock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	osal_mutex_destroy(&mutex);
}

/* 测试用例4: 互斥锁加锁失败 - 空指针 */
static void _test_mutex_lock_nullpointer(void)
{
	int32_t ret = osal_mutex_lock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例5: 互斥锁解锁失败 - 空指针 */
static void _test_mutex_unlock_nullpointer(void)
{
	int32_t ret = osal_mutex_unlock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 测试用例6: 互斥锁销毁成功 */
static void _test_mutex_destroy_success(void)
{
	osal_mutex_t mutex;
	osal_mutex_init(&mutex, NULL);

	int32_t ret = osal_mutex_destroy(&mutex);
	TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例7: 互斥锁销毁失败 - 空指针 */
static void _test_mutex_destroy_nullpointer(void)
{
	int32_t ret = osal_mutex_destroy(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}

/* 生产者线程 */
static void *_producer_thread(void *arg)
{
	osal_mutex_t *mutex = (osal_mutex_t *)arg;
	int32_t i;

	for (i = 0; i < 1000; i++) {
		osal_mutex_lock(mutex);
		shared_counter++;
		osal_mutex_unlock(mutex);
	}

	return NULL;
}

/* 消费者线程 */
static void *_consumer_thread(void *arg)
{
	osal_mutex_t *mutex = (osal_mutex_t *)arg;
	int32_t i;

	for (i = 0; i < 1000; i++) {
		osal_mutex_lock(mutex);
		shared_counter--;
		osal_mutex_unlock(mutex);
	}

	return NULL;
}

/* 测试用例8: 多线程互斥 */
static void _test_mutex_multithread(void)
{
	shared_counter = 0;
	osal_mutex_t mutex;
	osal_mutex_init(&mutex, NULL);

	osal_thread_t producer, consumer;

	/* 创建生产者和消费者线程 */
	osal_thread_create(&producer, NULL, _producer_thread, &mutex);
	osal_thread_create(&consumer, NULL, _consumer_thread, &mutex);

	/* 等待线程完成 */
	osal_thread_join(producer, NULL);
	osal_thread_join(consumer, NULL);

	/* 验证计数器归零（如果互斥锁工作正常）*/
	TEST_ASSERT_EQUAL(0, shared_counter);

	osal_mutex_destroy(&mutex);
}

/* 测试用例9: 递归锁 */
static void _test_mutex_recursive(void)
{
	osal_mutex_t mutex;
	osal_mutex_attr_t attr;

	/* 设置递归锁属性 */
	osal_mutex_attr_init(&attr);
	osal_mutex_attr_set_type(&attr, OSAL_MUTEX_RECURSIVE);
	osal_mutex_init(&mutex, &attr);

	/* 多次加锁（递归锁允许） */
	int32_t ret = osal_mutex_lock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_lock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	/* 对应解锁 */
	ret = osal_mutex_unlock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_unlock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	osal_mutex_destroy(&mutex);
	osal_mutex_attr_destroy(&attr);
}

/* 测试用例10: trylock 非阻塞 */
static void _test_mutex_trylock(void)
{
	osal_mutex_t mutex;
	osal_mutex_init(&mutex, NULL);

	/* 第一次 trylock 应该成功 */
	int32_t ret = osal_mutex_try_lock(&mutex);
	TEST_ASSERT_EQUAL(0, ret);

	/* 第二次 trylock 应该失败（锁已被占用）*/
	ret = osal_mutex_try_lock(&mutex);
	TEST_ASSERT_EQUAL(-1, ret);
	TEST_ASSERT_EQUAL(EBUSY, errno);

	/* 解锁 */
	osal_mutex_unlock(&mutex);

	osal_mutex_destroy(&mutex);
}

/* 注册测试套件 */

/* 测试用例数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_mutex_init_success",
	  .func = _test_mutex_init_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_init_nullpointer",
	  .func = _test_mutex_init_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_lockunlock_success",
	  .func = _test_mutex_lockunlock_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_lock_nullpointer",
	  .func = _test_mutex_lock_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_unlock_nullpointer",
	  .func = _test_mutex_unlock_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_destroy_success",
	  .func = _test_mutex_destroy_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_destroy_nullpointer",
	  .func = _test_mutex_destroy_nullpointer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_multithread",
	  .func = _test_mutex_multithread,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_recursive",
	  .func = _test_mutex_recursive,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_mutex_trylock",
	  .func = _test_mutex_trylock,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_mutex",
	.module_name = "osal_mutex",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description =
					  "OSAL mutex tests (osal_mutex_t thin wrapper)" }
};

/* 测试套件注册函数 */
void register_osal_mutex_tests(void)
{
	libutest_register_suite(&test_suite);
}
