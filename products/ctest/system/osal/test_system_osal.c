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
static void *producer_thread(void *arg)
{
    thread_coord_ctx_t *ctx = (thread_coord_ctx_t *)arg;
    uint32_t i;

    for (i = 0; i < 100; i++) {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        OSAL_atomic_inc(&ctx->counter);
        OSAL_pthread_cond_signal(&ctx->cond);
        OSAL_pthread_mutex_unlock(&ctx->mutex);
        OSAL_usleep(1000);
    }

    return NULL;
}

/* 消费者线程 */
static void *consumer_thread(void *arg)
{
    thread_coord_ctx_t *ctx = (thread_coord_ctx_t *)arg;
    uint32_t consumed = 0;

    while (consumed < 100) {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        while (OSAL_atomic_load(&ctx->counter) == 0 && !ctx->shutdown) {
            OSAL_pthread_cond_wait(&ctx->cond, &ctx->mutex);
        }
        if (OSAL_atomic_load(&ctx->counter) > 0) {
            OSAL_atomic_dec(&ctx->counter);
            consumed++;
        }
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}

/**
 * 测试多线程协同工作
 */
static void test_system_multi_thread_coordination(void)
{
    OSAL_printf("[ TEST     ] Multi-thread coordination system test\n");

    thread_coord_ctx_t ctx;
    osal_thread_t producer, consumer1, consumer2;
    int32_t ret;

    /* 初始化同步原语 */
    ret = OSAL_pthread_mutex_init(&ctx.mutex, NULL);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_cond_init(&ctx.cond, NULL);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_atomic_store(&ctx.counter, 0);
    OSAL_atomic_store(&ctx.ready_flags, 0);
    ctx.shutdown = 0;

    /* 创建生产者和消费者线程 */
    ret = OSAL_pthread_create(&producer, NULL, producer_thread, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_create(&consumer1, NULL, consumer_thread, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_create(&consumer2, NULL, consumer_thread, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待完成 */
    OSAL_pthread_join(producer, NULL);
    OSAL_pthread_join(consumer1, NULL);
    OSAL_pthread_join(consumer2, NULL);

    /* 验证最终状态 */
    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&ctx.counter));

    /* 清理 */
    OSAL_pthread_cond_destroy(&ctx.cond);
    OSAL_pthread_mutex_destroy(&ctx.mutex);

    OSAL_printf("[ PASS     ] Multi-thread coordination test passed\n");
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
static void *writer_thread(void *arg)
{
    ipc_e2e_ctx_t *ctx = (ipc_e2e_ctx_t *)arg;
    uint32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_sem_wait(&ctx->sem_writer);

        /* 写入数据到共享内存 */
        if (ctx->shm_ptr) {
            uint32_t *data = (uint32_t *)ctx->shm_ptr;
            *data = i * 100;
        }

        OSAL_sem_post(&ctx->sem_reader);
        OSAL_msleep(10);
    }

    return NULL;
}

/* 读取线程 */
static void *reader_thread(void *arg)
{
    ipc_e2e_ctx_t *ctx = (ipc_e2e_ctx_t *)arg;
    uint32_t i;

    for (i = 0; i < 10; i++) {
        OSAL_sem_post(&ctx->sem_writer);
        OSAL_sem_wait(&ctx->sem_reader);

        /* 从共享内存读取数据 */
        if (ctx->shm_ptr) {
            uint32_t *data = (uint32_t *)ctx->shm_ptr;
            if (*data != i * 100) {
                OSAL_printf("[  FAILED  ] Data mismatch: expected %u, got %u\n",
                            i * 100,
                            *data);
                g_test_failed = true;
            }
        }
    }

    return NULL;
}

/**
 * 测试进程间通信端到端
 */
static void test_system_ipc_end_to_end(void)
{
    OSAL_printf("[ TEST     ] IPC end-to-end system test\n");

    ipc_e2e_ctx_t ctx;
    osal_thread_t writer, reader;
    int32_t ret;

    /* 初始化信号量 */
    ret = OSAL_sem_init(&ctx.sem_writer, 0, 0);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_sem_init(&ctx.sem_reader, 0, 0);
    TEST_ASSERT_EQUAL(0, ret);

    /* 创建共享内存 */
    OSAL_snprintf(ctx.shm_name,
                  sizeof(ctx.shm_name),
                  "/test_shm_%d",
                  OSAL_getpid());
    ctx.shm_size = 4096;

    ctx.shm_fd = OSAL_shm_open(ctx.shm_name, O_CREAT | O_RDWR, 0666);
    if (ctx.shm_fd >= 0) {
        OSAL_ftruncate(ctx.shm_fd, ctx.shm_size);
        ctx.shm_ptr = OSAL_mmap(NULL,
                                ctx.shm_size,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                ctx.shm_fd,
                                0);
        TEST_ASSERT_NOT_NULL(ctx.shm_ptr);
    } else {
        ctx.shm_ptr = NULL;
        OSAL_printf(
            "[ WARN     ] Shared memory not available, skipping SHM test\n");
    }

    /* 创建读写线程 */
    ret = OSAL_pthread_create(&writer, NULL, writer_thread, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_pthread_create(&reader, NULL, reader_thread, &ctx);
    TEST_ASSERT_EQUAL(0, ret);

    /* 等待完成 */
    OSAL_pthread_join(writer, NULL);
    OSAL_pthread_join(reader, NULL);

    /* 清理 */
    if (ctx.shm_ptr && ctx.shm_ptr != MAP_FAILED) {
        OSAL_munmap(ctx.shm_ptr, ctx.shm_size);
    }
    if (ctx.shm_fd >= 0) {
        OSAL_close(ctx.shm_fd);
        OSAL_shm_unlink(ctx.shm_name);
    }

    OSAL_sem_destroy(&ctx.sem_reader);
    OSAL_sem_destroy(&ctx.sem_writer);

    OSAL_printf("[ PASS     ] IPC end-to-end test passed\n");
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
static void test_system_resource_management_full_flow(void)
{
    OSAL_printf("[ TEST     ] Resource management full flow system test\n");

    resource_mgmt_ctx_t ctx;
    uint32_t i;
    int32_t ret;

    ctx.num_resources = 10;
    OSAL_atomic_store(&ctx.active_count, 0);

    /* 阶段1: 分配资源 */
    ctx.mutexes = OSAL_malloc(ctx.num_resources * sizeof(osal_mutex_t));
    TEST_ASSERT_NOT_NULL(ctx.mutexes);

    ctx.semaphores = OSAL_malloc(ctx.num_resources * sizeof(osal_sem_t));
    TEST_ASSERT_NOT_NULL(ctx.semaphores);

    ctx.memory_blocks = OSAL_malloc(ctx.num_resources * sizeof(void *));
    TEST_ASSERT_NOT_NULL(ctx.memory_blocks);

    /* 阶段2: 初始化资源 */
    for (i = 0; i < ctx.num_resources; i++) {
        ret = OSAL_pthread_mutex_init(&ctx.mutexes[i], NULL);
        TEST_ASSERT_EQUAL(0, ret);

        ret = OSAL_sem_init(&ctx.semaphores[i], 0, 1);
        TEST_ASSERT_EQUAL(0, ret);

        ctx.memory_blocks[i] = OSAL_malloc(1024);
        TEST_ASSERT_NOT_NULL(ctx.memory_blocks[i]);

        OSAL_atomic_inc(&ctx.active_count);
    }

    TEST_ASSERT_EQUAL(ctx.num_resources, OSAL_atomic_load(&ctx.active_count));

    /* 阶段3: 使用资源 */
    for (i = 0; i < ctx.num_resources; i++) {
        ret = OSAL_pthread_mutex_lock(&ctx.mutexes[i]);
        TEST_ASSERT_EQUAL(0, ret);

        OSAL_sem_wait(&ctx.semaphores[i]);

        /* 模拟资源操作 */
        OSAL_memset(ctx.memory_blocks[i], i, 1024);

        OSAL_sem_post(&ctx.semaphores[i]);

        ret = OSAL_pthread_mutex_unlock(&ctx.mutexes[i]);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 阶段4: 清理资源 */
    for (i = 0; i < ctx.num_resources; i++) {
        OSAL_free(ctx.memory_blocks[i]);
        OSAL_sem_destroy(&ctx.semaphores[i]);
        OSAL_pthread_mutex_destroy(&ctx.mutexes[i]);
        OSAL_atomic_dec(&ctx.active_count);
    }

    TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&ctx.active_count));

    OSAL_free(ctx.memory_blocks);
    OSAL_free(ctx.semaphores);
    OSAL_free(ctx.mutexes);

    /* 验证堆状态 */
    uint32_t free_bytes, total_bytes;
    ret = OSAL_heap_get_info(&free_bytes, &total_bytes);
    if (ret == 0) {
        OSAL_printf("[ INFO     ] Heap free: %u bytes, total: %u bytes\n",
                    free_bytes,
                    total_bytes);
    }

    OSAL_printf("[ PASS     ] Resource management full flow test passed\n");
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
static void *lifecycle_worker_thread(void *arg)
{
    uint32_t thread_id = (uint32_t)(uintptr_t)arg;
    uint32_t flag = (1U << thread_id);

    /* 标记已启动 */
    uint32_t old_val = OSAL_atomic_load(&g_lifecycle_ctx.init_flags);
    while (!OSAL_atomic_compare_exchange_strong(&g_lifecycle_ctx.init_flags,
                                                &old_val,
                                                old_val | flag)) {
        old_val = OSAL_atomic_load(&g_lifecycle_ctx.init_flags);
    }

    /* 等待信号量 */
    OSAL_sem_wait(&g_lifecycle_ctx.lifecycle_sem);

    /* 工作循环 */
    while (!g_lifecycle_ctx.worker_should_exit) {
        OSAL_msleep(10);
    }

    /* 标记已关闭 */
    old_val = OSAL_atomic_load(&g_lifecycle_ctx.shutdown_flags);
    while (!OSAL_atomic_compare_exchange_strong(&g_lifecycle_ctx.shutdown_flags,
                                                &old_val,
                                                old_val | flag)) {
        old_val = OSAL_atomic_load(&g_lifecycle_ctx.shutdown_flags);
    }

    return NULL;
}

/**
 * 测试系统启动关闭流程
 */
static void test_system_startup_shutdown_flow(void)
{
    OSAL_printf("[ TEST     ] Startup/shutdown flow system test\n");

    uint32_t i;
    int32_t ret;

    /* 阶段1: 初始化系统 */
    OSAL_memset(&g_lifecycle_ctx, 0, sizeof(g_lifecycle_ctx));
    OSAL_atomic_store(&g_lifecycle_ctx.init_flags, 0);
    OSAL_atomic_store(&g_lifecycle_ctx.shutdown_flags, 0);
    g_lifecycle_ctx.worker_should_exit = 0;

    ret = OSAL_pthread_mutex_init(&g_lifecycle_ctx.lifecycle_mutex, NULL);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_sem_init(&g_lifecycle_ctx.lifecycle_sem, 0, 0);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_printf("[ INFO     ] System initialization complete\n");

    /* 阶段2: 启动工作线程 */
    for (i = 0; i < 5; i++) {
        ret = OSAL_pthread_create(&g_lifecycle_ctx.worker_threads[i],
                                  NULL,
                                  lifecycle_worker_thread,
                                  (void *)(uintptr_t)i);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* 等待所有线程启动 */
    OSAL_msleep(100);
    TEST_ASSERT_EQUAL(0x1F, OSAL_atomic_load(&g_lifecycle_ctx.init_flags));
    OSAL_printf("[ INFO     ] All worker threads started (flags: 0x%X)\n",
                OSAL_atomic_load(&g_lifecycle_ctx.init_flags));

    /* 阶段3: 运行系统 */
    for (i = 0; i < 5; i++) {
        OSAL_sem_post(&g_lifecycle_ctx.lifecycle_sem);
    }
    OSAL_msleep(50);
    OSAL_printf("[ INFO     ] System running\n");

    /* 阶段4: 优雅关闭 */
    g_lifecycle_ctx.worker_should_exit = 1;

    for (i = 0; i < 5; i++) {
        OSAL_pthread_join(g_lifecycle_ctx.worker_threads[i], NULL);
    }

    TEST_ASSERT_EQUAL(0x1F, OSAL_atomic_load(&g_lifecycle_ctx.shutdown_flags));
    OSAL_printf("[ INFO     ] All worker threads shutdown (flags: 0x%X)\n",
                OSAL_atomic_load(&g_lifecycle_ctx.shutdown_flags));

    /* 阶段5: 清理系统 */
    OSAL_sem_destroy(&g_lifecycle_ctx.lifecycle_sem);
    OSAL_pthread_mutex_destroy(&g_lifecycle_ctx.lifecycle_mutex);

    OSAL_printf("[ PASS     ] Startup/shutdown flow test passed\n");
}

/* 注册系统测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
    { .name = "test_system_multi_thread_coordination",
      .func = test_system_multi_thread_coordination,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_system_ipc_end_to_end",
      .func = test_system_ipc_end_to_end,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_system_resource_management_full_flow",
      .func = test_system_resource_management_full_flow,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_system_startup_shutdown_flow",
      .func = test_system_startup_shutdown_flow,
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
__attribute__((constructor)) static void register_system_osal_tests(void)
{
    libutest_register_suite(&test_suite);
}
