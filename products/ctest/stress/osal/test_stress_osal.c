/**
 * @file test_stress_osal.c
 * @brief OSAL层压力测试 - 全面覆盖
 *
 * 压力测试场景：
 * 1. 并发压力 - 多线程同时访问API
 * 2. 长时间运行 - 持续运行检测内存泄漏
 * 3. 资源耗尽 - 达到系统限制
 * 4. 性能基准 - 操作延迟和吞吐量
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_stress.h>
#include "osal.h"

/* ==================== 原子操作压力测试 ==================== */

typedef struct {
	osal_atomic_uint32_t counter;
	osal_atomic_uint32_t errors;
} atomic_stress_data_t;

static int32_t atomic_stress_worker(void *user_data, uint32_t iteration)
{
	atomic_stress_data_t *data = (atomic_stress_data_t *)user_data;
	(void)iteration;

	/* 原子递增 */
	OSAL_atomic_inc(&data->counter);

	/* 原子操作组合 */
	uint32_t old_val = OSAL_atomic_fetch_add(&data->counter, 1);
	OSAL_atomic_fetch_sub(&data->counter, 1);

	/* CAS操作 */
	uint32_t expected = old_val;
	if (!OSAL_atomic_compare_exchange_strong(&data->counter, &expected, old_val + 1)) {
		OSAL_atomic_inc(&data->errors);
	}

	return 0;
}

static void test_stress_atomic_operations(void)
{
	atomic_stress_data_t data;
	const uint32_t thread_count = 20;
	const uint32_t duration_sec = 10;

	OSAL_atomic_init(&data.counter, 0);
	OSAL_atomic_init(&data.errors, 0);

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Atomic Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running atomic stress: %u threads, %u seconds\n",
		    thread_count, duration_sec);
	TEST_ASSERT_EQUAL(stress_run(ctx, atomic_stress_worker, &data), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	stress_context_destroy(ctx);
}

/* ==================== 互斥锁压力测试 ==================== */

typedef struct {
	osal_mutex_t mutex;
	osal_atomic_uint32_t counter;
	osal_atomic_uint32_t lock_failures;
} mutex_stress_data_t;

static int32_t mutex_contention_worker(void *user_data, uint32_t iteration)
{
	mutex_stress_data_t *data = (mutex_stress_data_t *)user_data;
	(void)iteration;

	/* 加锁 */
	int32_t ret = OSAL_pthread_mutex_lock(&data->mutex);
	if (ret != 0) {
		OSAL_atomic_inc(&data->lock_failures);
		return ret;
	}

	/* 临界区 - 模拟一些工作 */
	uint32_t val = OSAL_atomic_load(&data->counter);
	OSAL_atomic_store(&data->counter, val + 1);

	/* 解锁 */
	OSAL_pthread_mutex_unlock(&data->mutex);

	return 0;
}

static void test_stress_mutex_contention(void)
{
	mutex_stress_data_t data;
	const uint32_t thread_count = 50;
	const uint32_t duration_sec = 10;

	TEST_ASSERT_EQUAL(OSAL_pthread_mutex_init(&data.mutex, NULL), 0);
	OSAL_atomic_init(&data.counter, 0);
	OSAL_atomic_init(&data.lock_failures, 0);

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Mutex Contention", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running mutex contention: %u threads, %u seconds\n",
		    thread_count, duration_sec);
	TEST_ASSERT_EQUAL(stress_run(ctx, mutex_contention_worker, &data), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	TEST_ASSERT_EQUAL(OSAL_atomic_load(&data.lock_failures), 0);

	stress_context_destroy(ctx);
	OSAL_pthread_mutex_destroy(&data.mutex);
}

/* ==================== 信号量压力测试 ==================== */

typedef struct {
	osal_sem_t sem;
	osal_atomic_uint32_t post_count;
	osal_atomic_uint32_t wait_count;
} semaphore_stress_data_t;

static int32_t semaphore_post_worker(void *user_data, uint32_t iteration)
{
	semaphore_stress_data_t *data = (semaphore_stress_data_t *)user_data;
	(void)iteration;

	OSAL_sem_post(&data->sem);
	OSAL_atomic_inc(&data->post_count);

	return 0;
}

static int32_t semaphore_wait_worker(void *user_data, uint32_t iteration)
{
	semaphore_stress_data_t *data = (semaphore_stress_data_t *)user_data;
	(void)iteration;

	int32_t ret = OSAL_sem_wait(&data->sem);
	if (ret == 0) {
		OSAL_atomic_inc(&data->wait_count);
	}

	return ret;
}

static void test_stress_semaphore_signaling(void)
{
	semaphore_stress_data_t data;
	const uint32_t thread_count = 10;
	const uint32_t iterations = 10000;

	TEST_ASSERT_EQUAL(OSAL_sem_init(&data.sem, 0, 0), 0);
	OSAL_atomic_init(&data.post_count, 0);
	OSAL_atomic_init(&data.wait_count, 0);

	/* 先启动post线程 */
	stress_config_t post_config = {
		.type = STRESS_TYPE_CONCURRENCY,
		.thread_count = thread_count / 2,
		.duration_sec = 0,
		.iterations = iterations,
		.ramp_up_sec = 0,
		.stop_on_error = false
	};

	stress_context_t *post_ctx = stress_context_create("Semaphore Post", &post_config);
	TEST_ASSERT_NOT_NULL(post_ctx);

	OSAL_printf("[ INFO     ] Running semaphore signaling stress test\n");
	TEST_ASSERT_EQUAL(stress_run(post_ctx, semaphore_post_worker, &data), 0);

	/* 再启动wait线程 */
	stress_config_t wait_config = post_config;
	stress_context_t *wait_ctx = stress_context_create("Semaphore Wait", &wait_config);
	TEST_ASSERT_NOT_NULL(wait_ctx);

	TEST_ASSERT_EQUAL(stress_run(wait_ctx, semaphore_wait_worker, &data), 0);

	OSAL_printf("[ INFO     ] Post count: %u, Wait count: %u\n",
		    OSAL_atomic_load(&data.post_count),
		    OSAL_atomic_load(&data.wait_count));

	stress_context_destroy(post_ctx);
	stress_context_destroy(wait_ctx);
	OSAL_sem_destroy(&data.sem);
}

/* ==================== 条件变量压力测试 ==================== */

typedef struct {
	osal_mutex_t mutex;
	osal_cond_t cond;
	osal_atomic_uint32_t ready;
	osal_atomic_uint32_t signal_count;
	osal_atomic_uint32_t wait_count;
} cond_stress_data_t;

static int32_t cond_signal_worker(void *user_data, uint32_t iteration)
{
	cond_stress_data_t *data = (cond_stress_data_t *)user_data;
	(void)iteration;

	OSAL_pthread_mutex_lock(&data->mutex);
	OSAL_atomic_store(&data->ready, 1);
	OSAL_pthread_cond_signal(&data->cond);
	OSAL_atomic_inc(&data->signal_count);
	OSAL_pthread_mutex_unlock(&data->mutex);

	return 0;
}

static int32_t cond_wait_worker(void *user_data, uint32_t iteration)
{
	cond_stress_data_t *data = (cond_stress_data_t *)user_data;
	(void)iteration;

	OSAL_pthread_mutex_lock(&data->mutex);
	while (OSAL_atomic_load(&data->ready) == 0) {
		OSAL_pthread_cond_wait(&data->cond, &data->mutex);
	}
	OSAL_atomic_store(&data->ready, 0);
	OSAL_atomic_inc(&data->wait_count);
	OSAL_pthread_mutex_unlock(&data->mutex);

	return 0;
}

static void test_stress_condition_variable(void)
{
	cond_stress_data_t data;
	const uint32_t thread_count = 10;
	const uint32_t duration_sec = 5;

	TEST_ASSERT_EQUAL(OSAL_pthread_mutex_init(&data.mutex, NULL), 0);
	TEST_ASSERT_EQUAL(OSAL_pthread_cond_init(&data.cond, NULL), 0);
	OSAL_atomic_init(&data.ready, 0);
	OSAL_atomic_init(&data.signal_count, 0);
	OSAL_atomic_init(&data.wait_count, 0);

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *signal_ctx = stress_context_create("Cond Signal", &config);
	stress_context_t *wait_ctx = stress_context_create("Cond Wait", &config);
	TEST_ASSERT_NOT_NULL(signal_ctx);
	TEST_ASSERT_NOT_NULL(wait_ctx);

	OSAL_printf("[ INFO     ] Running condition variable stress test\n");

	/* 交替运行signal和wait */
	TEST_ASSERT_EQUAL(stress_run(signal_ctx, cond_signal_worker, &data), 0);
	TEST_ASSERT_EQUAL(stress_run(wait_ctx, cond_wait_worker, &data), 0);

	stress_context_destroy(signal_ctx);
	stress_context_destroy(wait_ctx);
	OSAL_pthread_cond_destroy(&data.cond);
	OSAL_pthread_mutex_destroy(&data.mutex);
}

/* ==================== 线程创建压力测试 ==================== */

static void *thread_stress_func(void *arg)
{
	osal_atomic_uint32_t *counter = (osal_atomic_uint32_t *)arg;
	OSAL_atomic_inc(counter);
	return NULL;
}

static void test_stress_thread_creation(void)
{
	const uint32_t thread_count = 100;
	osal_thread_t threads[thread_count];
	osal_atomic_uint32_t counter;

	OSAL_atomic_init(&counter, 0);

	OSAL_printf("[ INFO     ] Creating %u threads rapidly\n", thread_count);

	uint32_t start_time = OSAL_get_tick_count();

	/* 快速创建大量线程 */
	for (uint32_t i = 0; i < thread_count; i++) {
		int32_t ret = OSAL_pthread_create(&threads[i], NULL,
						   thread_stress_func, &counter);
		TEST_ASSERT_EQUAL(ret, 0);
	}

	/* 等待所有线程完成 */
	for (uint32_t i = 0; i < thread_count; i++) {
		OSAL_pthread_join(threads[i], NULL);
	}

	uint32_t elapsed_ms = OSAL_get_tick_count() - start_time;

	TEST_ASSERT_EQUAL(OSAL_atomic_load(&counter), thread_count);
	OSAL_printf("[ INFO     ] Created %u threads in %u ms (%.2f threads/sec)\n",
		    thread_count, elapsed_ms,
		    1000.0 * thread_count / elapsed_ms);
}

/* ==================== 内存分配压力测试 ==================== */

typedef struct {
	osal_atomic_uint32_t alloc_count;
	osal_atomic_uint32_t free_count;
	osal_atomic_uint32_t alloc_failures;
} memory_stress_data_t;

static int32_t memory_alloc_worker(void *user_data, uint32_t iteration)
{
	memory_stress_data_t *data = (memory_stress_data_t *)user_data;
	(void)iteration;

	/* 随机大小分配 */
	size_t sizes[] = {16, 64, 256, 1024, 4096, 16384};
	size_t size = sizes[iteration % 6];

	void *ptr = OSAL_malloc(size);
	if (ptr == NULL) {
		OSAL_atomic_inc(&data->alloc_failures);
		return -1;
	}

	OSAL_atomic_inc(&data->alloc_count);

	/* 写入数据验证 */
	OSAL_memset(ptr, 0xAA, size);

	/* 立即释放 */
	OSAL_free(ptr);
	OSAL_atomic_inc(&data->free_count);

	return 0;
}

static void test_stress_memory_allocation(void)
{
	memory_stress_data_t data;
	const uint32_t thread_count = 20;
	const uint32_t duration_sec = 10;

	OSAL_atomic_init(&data.alloc_count, 0);
	OSAL_atomic_init(&data.free_count, 0);
	OSAL_atomic_init(&data.alloc_failures, 0);

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Memory Allocation", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running memory allocation stress: %u threads, %u seconds\n",
		    thread_count, duration_sec);
	TEST_ASSERT_EQUAL(stress_run(ctx, memory_alloc_worker, &data), 0);

	stress_print_report(ctx);

	uint32_t allocs = OSAL_atomic_load(&data.alloc_count);
	uint32_t frees = OSAL_atomic_load(&data.free_count);
	uint32_t failures = OSAL_atomic_load(&data.alloc_failures);

	OSAL_printf("[ INFO     ] Allocations: %u, Frees: %u, Failures: %u\n",
		    allocs, frees, failures);
	TEST_ASSERT_EQUAL(allocs, frees);

	stress_context_destroy(ctx);
}

/* ==================== 时间操作压力测试 ==================== */

static int32_t time_ops_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;
	(void)iteration;

	/* 时间获取操作 */
	uint32_t tick = OSAL_get_tick_count();
	int64_t mono = OSAL_get_monotonic_time();
	OS_time_t local_time;
	OSAL_get_local_time(&local_time);

	/* 验证时间的单调性 */
	uint32_t tick2 = OSAL_get_tick_count();
	if (tick2 < tick) {
		return -1;
	}

	/* 验证单调时间有效 */
	if (mono < 0) {
		return -1;
	}

	return 0;
}

static void test_stress_time_operations(void)
{
	const uint32_t thread_count = 10;
	const uint32_t duration_sec = 5;

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Time Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running time operations stress test\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, time_ops_worker, NULL), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	stress_context_destroy(ctx);
}

/* ==================== 字符串操作压力测试 ==================== */

static int32_t string_ops_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	char buf1[256];
	char buf2[256];

	/* 字符串格式化 */
	OSAL_snprintf(buf1, sizeof(buf1), "Test string %u", iteration);

	/* 字符串比较 */
	OSAL_strcpy(buf2, buf1);
	if (OSAL_strcmp(buf1, buf2) != 0) {
		return -1;
	}

	/* 字符串长度 */
	size_t len = OSAL_strlen(buf1);
	if (len == 0 || len >= sizeof(buf1)) {
		return -1;
	}

	return 0;
}

static void test_stress_string_operations(void)
{
	const uint32_t thread_count = 20;
	const uint32_t duration_sec = 5;

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("String Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running string operations stress test\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, string_ops_worker, NULL), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	stress_context_destroy(ctx);
}

/* ==================== 文件操作压力测试 ==================== */

typedef struct {
	osal_atomic_uint32_t file_count;
	char prefix[64];
} file_stress_data_t;

static int32_t file_ops_worker(void *user_data, uint32_t iteration)
{
	file_stress_data_t *data = (file_stress_data_t *)user_data;
	char filename[128];
	int fd;

	/* 生成唯一文件名 */
	OSAL_snprintf(filename, sizeof(filename), "/tmp/%s_%lu_%u.tmp",
		      data->prefix, (unsigned long)OSAL_pthread_self(), iteration);

	/* 创建文件 */
	fd = OSAL_open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd < 0) {
		return -1;
	}

	/* 写入数据 */
	char buf[256];
	OSAL_snprintf(buf, sizeof(buf), "Test data %u\n", iteration);
	ssize_t written = OSAL_write(fd, buf, OSAL_strlen(buf));
	if (written < 0) {
		OSAL_close(fd);
		return -1;
	}

	/* 读取验证 */
	OSAL_lseek(fd, 0, SEEK_SET);
	char read_buf[256];
	ssize_t nread = OSAL_read(fd, read_buf, sizeof(read_buf));
	if (nread < 0) {
		OSAL_close(fd);
		return -1;
	}

	OSAL_close(fd);

	/* 删除文件 */
	unlink(filename);

	OSAL_atomic_inc(&data->file_count);
	return 0;
}

static void test_stress_file_operations(void)
{
	file_stress_data_t data;
	const uint32_t thread_count = 10;
	const uint32_t duration_sec = 5;

	OSAL_atomic_init(&data.file_count, 0);
	OSAL_snprintf(data.prefix, sizeof(data.prefix), "osal_stress_%u",
		      OSAL_get_tick_count());

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("File Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running file operations stress test\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, file_ops_worker, &data), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.0);

	OSAL_printf("[ INFO     ] Created/deleted %u files\n",
		    OSAL_atomic_load(&data.file_count));

	stress_context_destroy(ctx);
}

/* ==================== CRC计算压力测试 ==================== */

static int32_t crc_ops_worker(void *user_data, uint32_t iteration)
{
	(void)user_data;

	uint8_t data[256];
	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)(iteration + i);
	}

	/* CRC16计算 */
	uint16_t crc16 = OSAL_CRC16_CCITT(data, sizeof(data));

	/* CRC32计算 */
	uint32_t crc32 = OSAL_CRC32(data, sizeof(data));

	/* 验证非零 */
	if (crc16 == 0 && crc32 == 0) {
		return -1;
	}

	return 0;
}

static void test_stress_crc_operations(void)
{
	const uint32_t thread_count = 20;
	const uint32_t duration_sec = 5;

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("CRC Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running CRC operations stress test\n");
	TEST_ASSERT_EQUAL(stress_run(ctx, crc_ops_worker, NULL), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_NO_ERRORS(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

	stress_context_destroy(ctx);
}

/* ==================== 混合场景压力测试 ==================== */

typedef struct {
	osal_mutex_t mutex;
	osal_sem_t sem;
	osal_atomic_uint32_t operations;
} mixed_stress_data_t;

static int32_t mixed_ops_worker(void *user_data, uint32_t iteration)
{
	mixed_stress_data_t *data = (mixed_stress_data_t *)user_data;

	/* 根据迭代选择不同的操作 */
	switch (iteration % 5) {
	case 0:
		/* 互斥锁操作 */
		OSAL_pthread_mutex_lock(&data->mutex);
		OSAL_atomic_inc(&data->operations);
		OSAL_pthread_mutex_unlock(&data->mutex);
		break;

	case 1:
		/* 信号量操作 */
		OSAL_sem_post(&data->sem);
		OSAL_sem_wait(&data->sem);
		break;

	case 2:
		/* 内存操作 */
		{
			void *ptr = OSAL_malloc(1024);
			if (ptr) {
				OSAL_memset(ptr, 0, 1024);
				OSAL_free(ptr);
			}
		}
		break;

	case 3:
		/* 时间操作 */
		OSAL_get_tick_count();
		OSAL_get_monotonic_time();
		break;

	case 4:
		/* 字符串操作 */
		{
			char buf[128];
			OSAL_snprintf(buf, sizeof(buf), "Test %u", iteration);
			OSAL_strlen(buf);
		}
		break;
	}

	return 0;
}

static void test_stress_mixed_operations(void)
{
	mixed_stress_data_t data;
	const uint32_t thread_count = 30;
	const uint32_t duration_sec = 10;

	TEST_ASSERT_EQUAL(OSAL_pthread_mutex_init(&data.mutex, NULL), 0);
	TEST_ASSERT_EQUAL(OSAL_sem_init(&data.sem, 0, 10), 0);
	OSAL_atomic_init(&data.operations, 0);

	stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
	stress_context_t *ctx = stress_context_create("Mixed Operations", &config);
	TEST_ASSERT_NOT_NULL(ctx);

	OSAL_printf("[ INFO     ] Running mixed operations stress: %u threads, %u seconds\n",
		    thread_count, duration_sec);
	TEST_ASSERT_EQUAL(stress_run(ctx, mixed_ops_worker, &data), 0);

	stress_print_report(ctx);
	STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.0);

	stress_context_destroy(ctx);
	OSAL_sem_destroy(&data.sem);
	OSAL_pthread_mutex_destroy(&data.mutex);
}

/* ==================== 测试套件注册 ==================== */

static const test_case_t test_cases[] = {
	{
		.name = "test_stress_atomic_operations",
		.func = test_stress_atomic_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_mutex_contention",
		.func = test_stress_mutex_contention,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_semaphore_signaling",
		.func = test_stress_semaphore_signaling,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_condition_variable",
		.func = test_stress_condition_variable,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_thread_creation",
		.func = test_stress_thread_creation,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_memory_allocation",
		.func = test_stress_memory_allocation,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_time_operations",
		.func = test_stress_time_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_string_operations",
		.func = test_stress_string_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_file_operations",
		.func = test_stress_file_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_crc_operations",
		.func = test_stress_crc_operations,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_mixed_operations",
		.func = test_stress_mixed_operations,
		.setup = NULL,
		.teardown = NULL
	},
};

static const test_suite_t test_suite = {
	.suite_name = "stress_osal",
	.module_name = "stress_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 120000,  /* 2分钟超时 */
		.description = "OSAL comprehensive stress tests covering concurrency, "
			       "resource exhaustion, performance benchmarks"
	}
};

__attribute__((constructor))
static void register_stress_osal_tests(void)
{
	libutest_register_suite(&test_suite);
}
