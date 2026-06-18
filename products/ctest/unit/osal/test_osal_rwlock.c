/************************************************************************
 * OSAL测试 - 读写锁测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

static void _test_rwlock_init_destroy(void)
{
	osal_rwlock_t rwlock;

	/* 测试初始化 */
	int32_t ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试销毁 */
	ret = osal_rwlock_destroy(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_rwlock_init_destroy_null_pointer(void)
{
	/* 测试NULL指针 */
	int32_t ret = osal_rwlock_init(NULL, NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_rwlock_destroy(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
}

static void _test_rwlock_rdlock_unlock(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试获取读锁 */
	ret = osal_rwlock_read_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试释放锁 */
	ret = osal_rwlock_unlock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_wrlock_unlock(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试获取写锁 */
	ret = osal_rwlock_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试释放锁 */
	ret = osal_rwlock_unlock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_tryrdlock(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试非阻塞获取读锁（应该成功） */
	ret = osal_rwlock_try_read_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 再次尝试获取读锁（多个读锁可以共存） */
	ret = osal_rwlock_try_read_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	osal_rwlock_unlock(&rwlock);
	osal_rwlock_unlock(&rwlock);
	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_trywrlock(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 测试非阻塞获取写锁（应该成功） */
	ret = osal_rwlock_try_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	osal_rwlock_unlock(&rwlock);
	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_tryrdlock_when_write_locked(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 先获取写锁 */
	ret = osal_rwlock_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 尝试获取读锁（应该失败，因为有写锁） */
	ret = osal_rwlock_try_read_lock(&rwlock);
	TEST_ASSERT_EQUAL(-1, ret);

	osal_rwlock_unlock(&rwlock);
	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_trywrlock_when_read_locked(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 先获取读锁 */
	ret = osal_rwlock_read_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 尝试获取写锁（应该失败，因为有读锁） */
	ret = osal_rwlock_try_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(-1, ret);

	osal_rwlock_unlock(&rwlock);
	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_trywrlock_when_write_locked(void)
{
	osal_rwlock_t rwlock;
	int32_t ret;

	ret = osal_rwlock_init(&rwlock, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* 先获取写锁 */
	ret = osal_rwlock_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(0, ret);

	/* 再次尝试获取写锁（应该失败） */
	ret = osal_rwlock_try_write_lock(&rwlock);
	TEST_ASSERT_EQUAL(-1, ret);

	osal_rwlock_unlock(&rwlock);
	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_null_pointer_operations(void)
{
	/* 测试NULL指针操作 */
	int32_t ret;

	ret = osal_rwlock_read_lock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_rwlock_write_lock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_rwlock_try_read_lock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_rwlock_try_write_lock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_rwlock_unlock(NULL);
	TEST_ASSERT_EQUAL(-1, ret);
}

/*===========================================================================
 * 多线程测试
 *===========================================================================*/

typedef struct {
	osal_rwlock_t *rwlock;
	uint32_t *shared_data;
	uint32_t read_count;
} rwlock_test_ctx_t;

static void *_reader_thread(void *arg)
{
	rwlock_test_ctx_t *ctx = (rwlock_test_ctx_t *)arg;
	uint32_t local_data;

	for (uint32_t i = 0; i < 100; i++) {
		osal_rwlock_read_lock(ctx->rwlock);
		local_data = *(ctx->shared_data);
		(void)local_data; /* Suppress unused warning - we're just testing read
                             access */
		ctx->read_count++;
		osal_rwlock_unlock(ctx->rwlock);
		osal_usleep(1000); /* 1ms */
	}

	return NULL;
}

static void *_writer_thread(void *arg)
{
	rwlock_test_ctx_t *ctx = (rwlock_test_ctx_t *)arg;

	for (uint32_t i = 0; i < 50; i++) {
		osal_rwlock_write_lock(ctx->rwlock);
		(*(ctx->shared_data))++;
		osal_rwlock_unlock(ctx->rwlock);
		osal_usleep(2000); /* 2ms */
	}

	return NULL;
}

static void _test_rwlock_multiple_readers(void)
{
	osal_rwlock_t rwlock;
	uint32_t shared_data = 0;
	rwlock_test_ctx_t ctx[3];
	osal_thread_t threads[3];

	osal_rwlock_init(&rwlock, NULL);

	/* 创建3个读线程 */
	for (int i = 0; i < 3; i++) {
		ctx[i].rwlock = &rwlock;
		ctx[i].shared_data = &shared_data;
		ctx[i].read_count = 0;
		osal_thread_create(&threads[i], NULL, _reader_thread, &ctx[i]);
	}

	/* 等待所有线程完成 */
	for (int i = 0; i < 3; i++) {
		osal_thread_join(threads[i], NULL);
	}

	/* 验证读取次数 */
	uint32_t total_reads = 0;
	for (int i = 0; i < 3; i++) {
		total_reads += ctx[i].read_count;
	}
	TEST_ASSERT_EQUAL(300, total_reads);

	osal_rwlock_destroy(&rwlock);
}

static void _test_rwlock_readers_and_writers(void)
{
	osal_rwlock_t rwlock;
	uint32_t shared_data = 0;
	rwlock_test_ctx_t reader_ctx[2];
	rwlock_test_ctx_t writer_ctx[2];
	osal_thread_t reader_threads[2];
	osal_thread_t writer_threads[2];

	osal_rwlock_init(&rwlock, NULL);

	/* 创建2个读线程和2个写线程 */
	for (int i = 0; i < 2; i++) {
		reader_ctx[i].rwlock = &rwlock;
		reader_ctx[i].shared_data = &shared_data;
		reader_ctx[i].read_count = 0;
		osal_thread_create(&reader_threads[i], NULL, _reader_thread,
						   &reader_ctx[i]);

		writer_ctx[i].rwlock = &rwlock;
		writer_ctx[i].shared_data = &shared_data;
		writer_ctx[i].read_count = 0;
		osal_thread_create(&writer_threads[i], NULL, _writer_thread,
						   &writer_ctx[i]);
	}

	/* 等待所有线程完成 */
	for (int i = 0; i < 2; i++) {
		osal_thread_join(reader_threads[i], NULL);
		osal_thread_join(writer_threads[i], NULL);
	}

	/* 验证写入结果（2个写线程，每个写50次） */
	TEST_ASSERT_EQUAL(100, shared_data);

	osal_rwlock_destroy(&rwlock);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_rwlock(void)
{
	/* 基础功能测试 */
	_test_rwlock_init_destroy();
	_test_rwlock_init_destroy_null_pointer();
	_test_rwlock_rdlock_unlock();
	_test_rwlock_wrlock_unlock();
	_test_rwlock_tryrdlock();
	_test_rwlock_trywrlock();
	_test_rwlock_tryrdlock_when_write_locked();
	_test_rwlock_trywrlock_when_read_locked();
	_test_rwlock_trywrlock_when_write_locked();
	_test_rwlock_null_pointer_operations();

	/* 多线程测试 */
	_test_rwlock_multiple_readers();
	_test_rwlock_readers_and_writers();
}

static const test_case_t test_cases[] = {
	{ .name = "test_osal_rwlock",
	  .func = test_osal_rwlock,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_rwlock",
	.module_name = "osal_rwlock",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 3000,
				  .description = "OSAL rwlock tests" }
};

void register_osal_rwlock_tests(void)
{
	libutest_register_suite(&test_suite);
}
