/**
 * @file test_system_osal.c
 * @brief OSAL层系统测试
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include <fcntl.h>
#include <sys/mman.h>

/* 线程协同数据结构 */
typedef struct {
	osal_mutex_t mutex;
	osal_cond_t cond;
	osal_atomic_uint32_t counter;
	osal_atomic_uint32_t ready_flags;
	int shutdown;
} thread_coord_ctx_t;

/* 生产者线程 */
static void *_producer_thread(void *arg)
{
	thread_coord_ctx_t *ctx = (thread_coord_ctx_t *)arg;
	uint32_t i;

	for (i = 0; i < 100; i++) {
		osal_pthread_mutex_lock(&ctx->mutex);
		osal_atomic_inc(&ctx->counter);
		osal_pthread_cond_signal(&ctx->cond);
		osal_pthread_mutex_unlock(&ctx->mutex);
		osal_usleep(1000);
	}

	return NULL;
}

/* 消费者线程 */
static void *_consumer_thread(void *arg)
{
	thread_coord_ctx_t *ctx = (thread_coord_ctx_t *)arg;
	uint32_t consumed = 0;

	while (consumed < 100) {
		osal_pthread_mutex_lock(&ctx->mutex);
		while (osal_atomic_load(&ctx->counter) == 0 && !ctx->shutdown) {
			osal_pthread_cond_wait(&ctx->cond, &ctx->mutex);
		}
		if (osal_atomic_load(&ctx->counter) > 0) {
			osal_atomic_dec(&ctx->counter);
			consumed++;
		}
		osal_pthread_mutex_unlock(&ctx->mutex);
	}

	return NULL;
}

/**
 * 测试多线程协同工作
 */
static void _test_system_multi_thread_coordination(void)
{
	osal_printf("[ TEST     ] Multi-thread coordination system test\n");

	thread_coord_ctx_t ctx;
	osal_thread_t producer, consumer1, consumer2;
	int32_t ret;

	/* 初始化同步原语 */
	ret = osal_pthread_mutex_init(&ctx.mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_pthread_cond_init(&ctx.cond, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	osal_atomic_store(&ctx.counter, 0);
	osal_atomic_store(&ctx.ready_flags, 0);
	ctx.shutdown = 0;

	/* 创建生产者和消费者线程 */
	ret = osal_pthread_create(&producer, NULL, _producer_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_pthread_create(&consumer1, NULL, _consumer_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_pthread_create(&consumer2, NULL, _consumer_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待完成 */
	osal_pthread_join(producer, NULL);
	osal_pthread_join(consumer1, NULL);
	osal_pthread_join(consumer2, NULL);

	/* 验证最终状态 */
	TEST_ASSERT_EQUAL(0, osal_atomic_load(&ctx.counter));

	/* 清理 */
	osal_pthread_cond_destroy(&ctx.cond);
	osal_pthread_mutex_destroy(&ctx.mutex);

	osal_printf("[ PASS     ] Multi-thread coordination test passed\n");
}

/* IPC端到端测试数据结构 */
typedef struct {
	osal_sem_t sem_writer;
	osal_sem_t sem_reader;
	char shm_name[64];
	void *shm_ptr;
	size_t shm_size;
	int shm_fd;
} ipc_e2e_ctx_t;

/* 写入线程 */
static void *_writer_thread(void *arg)
{
	ipc_e2e_ctx_t *ctx = (ipc_e2e_ctx_t *)arg;
	uint32_t i;

	for (i = 0; i < 10; i++) {
		osal_sem_wait(&ctx->sem_writer);

		/* 写入数据到共享内存 */
		if (ctx->shm_ptr) {
			uint32_t *data = (uint32_t *)ctx->shm_ptr;
			*data = i * 100;
		}

		osal_sem_post(&ctx->sem_reader);
		osal_msleep(10);
	}

	return NULL;
}

/* 读取线程 */
static void *_reader_thread(void *arg)
{
	ipc_e2e_ctx_t *ctx = (ipc_e2e_ctx_t *)arg;
	uint32_t i;

	for (i = 0; i < 10; i++) {
		osal_sem_post(&ctx->sem_writer);
		osal_sem_wait(&ctx->sem_reader);

		/* 从共享内存读取数据 */
		if (ctx->shm_ptr) {
			uint32_t *data = (uint32_t *)ctx->shm_ptr;
			if (*data != i * 100) {
				osal_printf("[  FAILED  ] Data mismatch: expected %u, got %u\n",
							i * 100, *data);
				g_test_failed = true;
			}
		}
	}

	return NULL;
}

/**
 * 测试进程间通信端到端
 */
static void _test_system_ipc_end_to_end(void)
{
	osal_printf("[ TEST     ] IPC end-to-end system test\n");

	ipc_e2e_ctx_t ctx;
	osal_thread_t writer, reader;
	int32_t ret;

	/* 初始化信号量 */
	ret = osal_sem_init(&ctx.sem_writer, 0, 0);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_sem_init(&ctx.sem_reader, 0, 0);
	TEST_ASSERT_EQUAL(0, ret);

	/* 创建共享内存 */
	osal_snprintf(ctx.shm_name, sizeof(ctx.shm_name), "/test_shm_%d",
				  osal_getpid());
	ctx.shm_size = 4096;

	ctx.shm_fd = osal_shm_open(ctx.shm_name, O_CREAT | O_RDWR, 0666);
	if (ctx.shm_fd >= 0) {
		osal_ftruncate(ctx.shm_fd, ctx.shm_size);
		ctx.shm_ptr = osal_mmap(NULL, ctx.shm_size, PROT_READ | PROT_WRITE,
								MAP_SHARED, ctx.shm_fd, 0);
		TEST_ASSERT_NOT_NULL(ctx.shm_ptr);
	} else {
		ctx.shm_ptr = NULL;
		osal_printf(
			"[ WARN     ] Shared memory not available, skipping SHM test\n");
	}

	/* 创建读写线程 */
	ret = osal_pthread_create(&writer, NULL, _writer_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_pthread_create(&reader, NULL, _reader_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待完成 */
	osal_pthread_join(writer, NULL);
	osal_pthread_join(reader, NULL);

	/* 清理 */
	if (ctx.shm_ptr && ctx.shm_ptr != MAP_FAILED) {
		osal_munmap(ctx.shm_ptr, ctx.shm_size);
	}
	if (ctx.shm_fd >= 0) {
		osal_close(ctx.shm_fd);
		osal_shm_unlink(ctx.shm_name);
	}

	osal_sem_destroy(&ctx.sem_reader);
	osal_sem_destroy(&ctx.sem_writer);

	osal_printf("[ PASS     ] IPC end-to-end test passed\n");
}

/* 资源管理测试数据结构 */
typedef struct {
	osal_mutex_t *mutexes;
	osal_sem_t *semaphores;
	osal_thread_t *threads;
	void **memory_blocks;
	uint32_t num_resources;
	osal_atomic_uint32_t active_count;
} resource_mgmt_ctx_t;

/**
 * 测试资源管理完整流程
 */
static void _test_system_resource_management_full_flow(void)
{
	osal_printf("[ TEST     ] Resource management full flow system test\n");

	resource_mgmt_ctx_t ctx;
	uint32_t i;
	int32_t ret;

	ctx.num_resources = 10;
	osal_atomic_store(&ctx.active_count, 0);

	/* 阶段1: 分配资源 */
	ctx.mutexes = osal_malloc(ctx.num_resources * sizeof(osal_mutex_t));
	TEST_ASSERT_NOT_NULL(ctx.mutexes);

	ctx.semaphores = osal_malloc(ctx.num_resources * sizeof(osal_sem_t));
	TEST_ASSERT_NOT_NULL(ctx.semaphores);

	ctx.memory_blocks = osal_malloc(ctx.num_resources * sizeof(void *));
	TEST_ASSERT_NOT_NULL(ctx.memory_blocks);

	/* 阶段2: 初始化资源 */
	for (i = 0; i < ctx.num_resources; i++) {
		ret = osal_pthread_mutex_init(&ctx.mutexes[i], NULL);
		TEST_ASSERT_EQUAL(0, ret);

		ret = osal_sem_init(&ctx.semaphores[i], 0, 1);
		TEST_ASSERT_EQUAL(0, ret);

		ctx.memory_blocks[i] = osal_malloc(1024);
		TEST_ASSERT_NOT_NULL(ctx.memory_blocks[i]);

		osal_atomic_inc(&ctx.active_count);
	}

	TEST_ASSERT_EQUAL(ctx.num_resources, osal_atomic_load(&ctx.active_count));

	/* 阶段3: 使用资源 */
	for (i = 0; i < ctx.num_resources; i++) {
		ret = osal_pthread_mutex_lock(&ctx.mutexes[i]);
		TEST_ASSERT_EQUAL(0, ret);

		osal_sem_wait(&ctx.semaphores[i]);

		/* 模拟资源操作 */
		osal_memset(ctx.memory_blocks[i], i, 1024);

		osal_sem_post(&ctx.semaphores[i]);

		ret = osal_pthread_mutex_unlock(&ctx.mutexes[i]);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* 阶段4: 清理资源 */
	for (i = 0; i < ctx.num_resources; i++) {
		osal_free(ctx.memory_blocks[i]);
		osal_sem_destroy(&ctx.semaphores[i]);
		osal_pthread_mutex_destroy(&ctx.mutexes[i]);
		osal_atomic_dec(&ctx.active_count);
	}

	TEST_ASSERT_EQUAL(0, osal_atomic_load(&ctx.active_count));

	osal_free(ctx.memory_blocks);
	osal_free(ctx.semaphores);
	osal_free(ctx.mutexes);

	/* 验证堆状态 */
	uint32_t free_bytes, total_bytes;
	ret = osal_heap_get_info(&free_bytes, &total_bytes);
	if (ret == 0) {
		osal_printf("[ INFO     ] Heap free: %u bytes, total: %u bytes\n",
					free_bytes, total_bytes);
	}

	osal_printf("[ PASS     ] Resource management full flow test passed\n");
}

/* 启动关闭流程测试数据结构 */
typedef struct {
	osal_atomic_uint32_t init_flags;
	osal_atomic_uint32_t shutdown_flags;
	osal_mutex_t lifecycle_mutex;
	osal_sem_t lifecycle_sem;
	osal_thread_t worker_threads[5];
	int worker_should_exit;
} lifecycle_ctx_t;

static lifecycle_ctx_t g_lifecycle_ctx;

/* 工作线程 */
static void *_lifecycle_worker_thread(void *arg)
{
	uint32_t thread_id = (uint32_t)(uintptr_t)arg;
	uint32_t flag = (1U << thread_id);

	/* 标记已启动 */
	uint32_t old_val = osal_atomic_load(&g_lifecycle_ctx.init_flags);
	while (!osal_atomic_compare_exchange_strong(&g_lifecycle_ctx.init_flags,
												&old_val, old_val | flag)) {
		old_val = osal_atomic_load(&g_lifecycle_ctx.init_flags);
	}

	/* 等待信号量 */
	osal_sem_wait(&g_lifecycle_ctx.lifecycle_sem);

	/* 工作循环 */
	while (!g_lifecycle_ctx.worker_should_exit) {
		osal_msleep(10);
	}

	/* 标记已关闭 */
	old_val = osal_atomic_load(&g_lifecycle_ctx.shutdown_flags);
	while (!osal_atomic_compare_exchange_strong(&g_lifecycle_ctx.shutdown_flags,
												&old_val, old_val | flag)) {
		old_val = osal_atomic_load(&g_lifecycle_ctx.shutdown_flags);
	}

	return NULL;
}

/**
 * 测试系统启动关闭流程
 */
static void _test_system_startup_shutdown_flow(void)
{
	osal_printf("[ TEST     ] Startup/shutdown flow system test\n");

	uint32_t i;
	int32_t ret;

	/* 阶段1: 初始化系统 */
	osal_memset(&g_lifecycle_ctx, 0, sizeof(g_lifecycle_ctx));
	osal_atomic_store(&g_lifecycle_ctx.init_flags, 0);
	osal_atomic_store(&g_lifecycle_ctx.shutdown_flags, 0);
	g_lifecycle_ctx.worker_should_exit = 0;

	ret = osal_pthread_mutex_init(&g_lifecycle_ctx.lifecycle_mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_sem_init(&g_lifecycle_ctx.lifecycle_sem, 0, 0);
	TEST_ASSERT_EQUAL(0, ret);

	osal_printf("[ INFO     ] System initialization complete\n");

	/* 阶段2: 启动工作线程 */
	for (i = 0; i < 5; i++) {
		ret = osal_pthread_create(&g_lifecycle_ctx.worker_threads[i], NULL,
								  _lifecycle_worker_thread,
								  (void *)(uintptr_t)i);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* 等待所有线程启动 */
	osal_msleep(100);
	TEST_ASSERT_EQUAL(0x1F, osal_atomic_load(&g_lifecycle_ctx.init_flags));
	osal_printf("[ INFO     ] All worker threads started (flags: 0x%X)\n",
				osal_atomic_load(&g_lifecycle_ctx.init_flags));

	/* 阶段3: 运行系统 */
	for (i = 0; i < 5; i++) {
		osal_sem_post(&g_lifecycle_ctx.lifecycle_sem);
	}
	osal_msleep(50);
	osal_printf("[ INFO     ] System running\n");

	/* 阶段4: 优雅关闭 */
	g_lifecycle_ctx.worker_should_exit = 1;

	for (i = 0; i < 5; i++) {
		osal_pthread_join(g_lifecycle_ctx.worker_threads[i], NULL);
	}

	TEST_ASSERT_EQUAL(0x1F, osal_atomic_load(&g_lifecycle_ctx.shutdown_flags));
	osal_printf("[ INFO     ] All worker threads shutdown (flags: 0x%X)\n",
				osal_atomic_load(&g_lifecycle_ctx.shutdown_flags));

	/* 阶段5: 清理系统 */
	osal_sem_destroy(&g_lifecycle_ctx.lifecycle_sem);
	osal_pthread_mutex_destroy(&g_lifecycle_ctx.lifecycle_mutex);

	osal_printf("[ PASS     ] Startup/shutdown flow test passed\n");
}

/* 注册系统测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_system_multi_thread_coordination",
	  .func = _test_system_multi_thread_coordination,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_system_ipc_end_to_end",
	  .func = _test_system_ipc_end_to_end,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_system_resource_management_full_flow",
	  .func = _test_system_resource_management_full_flow,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_system_startup_shutdown_flow",
	  .func = _test_system_startup_shutdown_flow,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "system_osal",
	.module_name = "system_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_SYSTEM,
				  .tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
				  .timeout_ms = 10000,
				  .description = "OSAL system integration tests" }
};

/* 测试套件注册函数 */
void register_system_osal_tests(void)
{
	libutest_register_suite(&test_suite);
}
