/**
 * @file test_system_osal_threading_ipc.c
 * @brief Thread + IPC integration tests
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include <fcntl.h>
#include <sys/mman.h>

/* Producer-consumer context */
typedef struct {
	osal_mutex_t mutex;
	osal_cond_t cond_not_full;
	osal_cond_t cond_not_empty;
	int buffer[10];
	int count;
	int read_pos;
	int write_pos;
	int done;
} producer_consumer_t;

static void *_producer_func(void *arg)
{
	producer_consumer_t *pc = (producer_consumer_t *)arg;
	int i;

	for (i = 0; i < 50; i++) {
		osal_mutex_lock(&pc->mutex);

		/* Wait if buffer is full */
		while (pc->count == 10) {
			osal_cond_wait(&pc->cond_not_full, &pc->mutex);
		}

		/* Produce item */
		pc->buffer[pc->write_pos] = i;
		pc->write_pos = (pc->write_pos + 1) % 10;
		pc->count++;

		osal_cond_signal(&pc->cond_not_empty);
		osal_mutex_unlock(&pc->mutex);

		osal_usleep(100);
	}

	osal_mutex_lock(&pc->mutex);
	pc->done = 1;
	osal_cond_broadcast(&pc->cond_not_empty);
	osal_mutex_unlock(&pc->mutex);

	return NULL;
}

static void *_consumer_func(void *arg)
{
	producer_consumer_t *pc = (producer_consumer_t *)arg;
	int consumed = 0;

	while (consumed < 25) {
		osal_mutex_lock(&pc->mutex);

		/* Wait if buffer is empty */
		while (pc->count == 0 && !pc->done) {
			osal_cond_wait(&pc->cond_not_empty, &pc->mutex);
		}

		if (pc->count > 0) {
			/* Consume item */
			(void)pc->buffer[pc->read_pos]; /* Read but don't use */
			pc->read_pos = (pc->read_pos + 1) % 10;
			pc->count--;
			consumed++;

			osal_cond_signal(&pc->cond_not_full);
		}

		osal_mutex_unlock(&pc->mutex);
	}

	return NULL;
}

/**
 * Test producer-consumer pattern
 */
static void _test_producer_consumer_pattern(void)
{
	osal_printf("[ TEST     ] Producer-consumer pattern\n");

	producer_consumer_t pc;
	osal_thread_t producer, consumer1, consumer2;
	int32_t ret;

	/* Initialize */
	osal_memset(&pc, 0, sizeof(pc));
	ret = osal_mutex_init(&pc.mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_cond_init(&pc.cond_not_full, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_cond_init(&pc.cond_not_empty, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	/* Create threads */
	ret = osal_thread_create(&producer, NULL, _producer_func, &pc);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_thread_create(&consumer1, NULL, _consumer_func, &pc);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_thread_create(&consumer2, NULL, _consumer_func, &pc);
	TEST_ASSERT_EQUAL(0, ret);

	/* Wait for completion */
	osal_thread_join(producer, NULL);
	osal_thread_join(consumer1, NULL);
	osal_thread_join(consumer2, NULL);

	/* Cleanup */
	osal_cond_destroy(&pc.cond_not_empty);
	osal_cond_destroy(&pc.cond_not_full);
	osal_mutex_destroy(&pc.mutex);

	osal_printf("[ PASS     ] Producer-consumer pattern test passed\n");
}

/* Thread pool context */
typedef struct {
	osal_sem_t work_sem;
	osal_mutex_t queue_mutex;
	osal_atomic_uint32_t tasks_completed;
	int shutdown;
} thread_pool_t;

static void *_worker_func(void *arg)
{
	thread_pool_t *pool = (thread_pool_t *)arg;

	while (!pool->shutdown) {
		/* Wait for work */
		if (osal_sem_wait(&pool->work_sem) == 0) {
			if (pool->shutdown)
				break;

			/* Simulate work */
			osal_usleep(1000);

			/* Mark task complete */
			osal_atomic_inc(&pool->tasks_completed);
		}
	}

	return NULL;
}

/**
 * Test thread pool with semaphore coordination
 */
static void _test_thread_pool_semaphore(void)
{
	osal_printf("[ TEST     ] Thread pool with semaphore coordination\n");

	thread_pool_t pool;
	osal_thread_t workers[4];
	uint32_t i;
	int32_t ret;

	/* Initialize pool */
	osal_memset(&pool, 0, sizeof(pool));
	ret = osal_sem_init(&pool.work_sem, 0, 0);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_mutex_init(&pool.queue_mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	osal_atomic_store(&pool.tasks_completed, 0);
	pool.shutdown = 0;

	/* Create worker threads */
	for (i = 0; i < 4; i++) {
		ret = osal_thread_create(&workers[i], NULL, _worker_func, &pool);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Submit tasks */
	for (i = 0; i < 20; i++) {
		osal_sem_post(&pool.work_sem);
	}

	/* Wait for completion */
	while (osal_atomic_load(&pool.tasks_completed) < 20) {
		osal_msleep(10);
	}

	TEST_ASSERT_EQUAL(20, osal_atomic_load(&pool.tasks_completed));

	/* Shutdown */
	pool.shutdown = 1;
	for (i = 0; i < 4; i++) {
		osal_sem_post(&pool.work_sem);
	}

	for (i = 0; i < 4; i++) {
		osal_thread_join(workers[i], NULL);
	}

	/* Cleanup */
	osal_mutex_destroy(&pool.queue_mutex);
	osal_sem_destroy(&pool.work_sem);

	osal_printf("[ PASS     ] Thread pool test passed\n");
}

/* Shared memory context */
typedef struct {
	char shm_name[64];
	int shm_fd;
	void *shm_ptr;
	size_t shm_size;
	osal_mutex_t *shm_mutex;
	osal_atomic_uint32_t *shm_counter;
} shm_ctx_t;

static void *_shm_writer_func(void *arg)
{
	shm_ctx_t *ctx = (shm_ctx_t *)arg;
	uint32_t i;

	if (!ctx->shm_ptr)
		return NULL;

	for (i = 0; i < 100; i++) {
		osal_mutex_lock(ctx->shm_mutex);
		osal_atomic_inc(ctx->shm_counter);
		osal_mutex_unlock(ctx->shm_mutex);
		osal_usleep(100);
	}

	return NULL;
}

/**
 * Test shared memory access from multiple threads
 */
static void _test_shared_memory_multithread(void)
{
	osal_printf("[ TEST     ] Shared memory multi-thread access\n");

	shm_ctx_t ctx;
	osal_thread_t threads[3];
	uint32_t i;
	int32_t ret;

	/* Create shared memory */
	osal_snprintf(ctx.shm_name, sizeof(ctx.shm_name), "/test_shm_mt_%d",
				  osal_getpid());
	ctx.shm_size = 4096;

	ctx.shm_fd = osal_shm_open(ctx.shm_name, O_CREAT | O_RDWR, 0666);
	if (ctx.shm_fd < 0) {
		osal_printf("[ SKIP     ] Shared memory not available\n");
		return;
	}

	osal_ftruncate(ctx.shm_fd, ctx.shm_size);
	ctx.shm_ptr = osal_mmap(NULL, ctx.shm_size, PROT_READ | PROT_WRITE,
							MAP_SHARED, ctx.shm_fd, 0);
	TEST_ASSERT_NOT_NULL(ctx.shm_ptr);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, ctx.shm_ptr);

	/* Initialize shared data */
	ctx.shm_mutex = (osal_mutex_t *)ctx.shm_ptr;
	ctx.shm_counter =
		(osal_atomic_uint32_t *)((char *)ctx.shm_ptr + sizeof(osal_mutex_t));

	ret = osal_mutex_init(ctx.shm_mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	osal_atomic_store(ctx.shm_counter, 0);

	/* Create writer threads */
	for (i = 0; i < 3; i++) {
		ret = osal_thread_create(&threads[i], NULL, _shm_writer_func, &ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Wait for completion */
	for (i = 0; i < 3; i++) {
		osal_thread_join(threads[i], NULL);
	}

	/* Verify result */
	TEST_ASSERT_EQUAL(300, osal_atomic_load(ctx.shm_counter));

	/* Cleanup */
	osal_mutex_destroy(ctx.shm_mutex);
	osal_munmap(ctx.shm_ptr, ctx.shm_size);
	osal_close(ctx.shm_fd);
	osal_shm_unlink(ctx.shm_name);

	osal_printf("[ PASS     ] Shared memory multi-thread test passed\n");
}

/**
 * Test condition variable wakeup patterns
 */
static void _test_condition_wakeup_patterns(void)
{
	osal_printf("[ TEST     ] Condition variable wakeup patterns\n");

	osal_mutex_t mutex;
	osal_cond_t cond;
	osal_atomic_uint32_t wakeup_count;
	osal_thread_t waiter_threads[5];
	uint32_t i;
	int32_t ret;

	/* Initialize */
	ret = osal_mutex_init(&mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_cond_init(&cond, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	osal_atomic_store(&wakeup_count, 0);

	/* Create waiter threads */
	typedef struct {
		osal_mutex_t *mutex;
		osal_cond_t *cond;
		osal_atomic_uint32_t *counter;
	} waiter_ctx_t;

	waiter_ctx_t waiter_ctx = { &mutex, &cond, &wakeup_count };

	void *waiter_func(void *arg)
	{
		waiter_ctx_t *ctx = (waiter_ctx_t *)arg;
		osal_mutex_lock(ctx->mutex);
		osal_cond_wait(ctx->cond, ctx->mutex);
		osal_atomic_inc(ctx->counter);
		osal_mutex_unlock(ctx->mutex);
		return NULL;
	}

	for (i = 0; i < 5; i++) {
		ret = osal_thread_create(&waiter_threads[i], NULL, waiter_func,
								 &waiter_ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Give threads time to wait */
	osal_msleep(100);

	/* Test broadcast wakeup */
	osal_mutex_lock(&mutex);
	osal_cond_broadcast(&cond);
	osal_mutex_unlock(&mutex);

	/* Wait for all threads */
	for (i = 0; i < 5; i++) {
		osal_thread_join(waiter_threads[i], NULL);
	}

	/* Verify all threads woke up */
	TEST_ASSERT_EQUAL(5, osal_atomic_load(&wakeup_count));

	/* Cleanup */
	osal_cond_destroy(&cond);
	osal_mutex_destroy(&mutex);

	osal_printf("[ PASS     ] Condition wakeup patterns test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
	{ .name = "test_producer_consumer_pattern",
	  .func = _test_producer_consumer_pattern,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_thread_pool_semaphore",
	  .func = _test_thread_pool_semaphore,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shared_memory_multithread",
	  .func = _test_shared_memory_multithread,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_condition_wakeup_patterns",
	  .func = _test_condition_wakeup_patterns,
	  .setup = NULL,
	  .teardown = NULL },
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_osal_threading_ipc",
	.module_name = "system_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_SYSTEM,
				  .tags = TEST_TAG_SLOW | TEST_TAG_IPC,
				  .timeout_ms = 15000,
				  .description = "Thread + IPC integration tests" }
};

/* Register test suite */
void register_system_osal_threading_ipc_tests(void)
{
	libutest_register_suite(&test_suite);
}
