/************************************************************************
 * OSAL测试 - 文件锁测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * 测试辅助函数
 *===========================================================================*/

#define TEST_LOCK_FILE "/tmp/osal_test_flock.lock"

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

static void test_flock_create_destroy(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    /* 创建文件锁 */
    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(flock);

    /* 销毁文件锁 */
    ret = OSAL_flock_destroy(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_flock_create_null_path(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    /* 测试NULL路径 */
    ret = OSAL_flock_create(NULL, &flock);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

static void test_flock_create_null_output(void)
{
    int32_t ret;

    /* 测试NULL输出指针 */
    ret = OSAL_flock_create(TEST_LOCK_FILE, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

static void test_flock_destroy_null_pointer(void)
{
    int32_t ret;

    /* 测试NULL指针 */
    ret = OSAL_flock_destroy(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 独占锁测试
 *===========================================================================*/

static void test_flock_exclusive_lock_unlock(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取独占锁 */
    ret = OSAL_flock_lock(flock, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 解锁 */
    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock);
}

static void test_flock_try_lock_exclusive(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 尝试获取独占锁（非阻塞） */
    ret = OSAL_flock_try_lock(flock, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 解锁 */
    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock);
}

static void test_flock_exclusive_blocks_exclusive(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第一个锁获取独占锁 */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个锁尝试获取独占锁（应该失败） */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_ERR_BUSY, ret);

    /* 释放第一个锁 */
    ret = OSAL_flock_unlock(flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 现在第二个锁应该能获取 */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_unlock(flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);
}

/*===========================================================================
 * 共享锁测试
 *===========================================================================*/

static void test_flock_shared_lock_unlock(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取共享锁 */
    ret = OSAL_flock_lock(flock, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 解锁 */
    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock);
}

static void test_flock_multiple_shared_locks(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    osal_flock_t *flock3 = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock3);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 三个锁都获取共享锁（应该都成功） */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_lock(flock2, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_lock(flock3, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 释放所有锁 */
    OSAL_flock_unlock(flock1);
    OSAL_flock_unlock(flock2);
    OSAL_flock_unlock(flock3);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);
    OSAL_flock_destroy(flock3);
}

static void test_flock_shared_blocks_exclusive(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第一个锁获取共享锁 */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个锁尝试获取独占锁（应该失败） */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_ERR_BUSY, ret);

    /* 释放共享锁 */
    ret = OSAL_flock_unlock(flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 现在应该能获取独占锁 */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_unlock(flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);
}

static void test_flock_exclusive_blocks_shared(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第一个锁获取独占锁 */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个锁尝试获取共享锁（应该失败） */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_ERR_BUSY, ret);

    /* 释放独占锁 */
    ret = OSAL_flock_unlock(flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 现在应该能获取共享锁 */
    ret = OSAL_flock_try_lock(flock2, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_unlock(flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);
}

/*===========================================================================
 * 超时锁测试
 *===========================================================================*/

static void test_flock_timed_lock_success(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 带超时的锁（应该立即成功） */
    ret = OSAL_flock_timed_lock(flock, OSAL_FLOCK_EXCLUSIVE, 1000);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock);
}

static void test_flock_timed_lock_timeout(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第一个锁获取独占锁 */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个锁尝试带超时获取（应该超时） */
    uint64_t start_time = OSAL_get_tick_count();
    ret = OSAL_flock_timed_lock(flock2, OSAL_FLOCK_EXCLUSIVE, 100);
    uint64_t elapsed = OSAL_get_tick_count() - start_time;

    TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);
    TEST_ASSERT(elapsed >= 90);  /* 至少接近100ms */
    TEST_ASSERT(elapsed < 200);  /* 不应该等太久 */

    ret = OSAL_flock_unlock(flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);
}

/*===========================================================================
 * 错误处理测试
 *===========================================================================*/

static void test_flock_lock_null_pointer(void)
{
    int32_t ret;

    /* 测试NULL指针 */
    ret = OSAL_flock_lock(NULL, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

static void test_flock_unlock_null_pointer(void)
{
    int32_t ret;

    /* 测试NULL指针 */
    ret = OSAL_flock_unlock(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

static void test_flock_try_lock_null_pointer(void)
{
    int32_t ret;

    /* 测试NULL指针 */
    ret = OSAL_flock_try_lock(NULL, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

static void test_flock_timed_lock_null_pointer(void)
{
    int32_t ret;

    /* 测试NULL指针 */
    ret = OSAL_flock_timed_lock(NULL, OSAL_FLOCK_EXCLUSIVE, 1000);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 多线程测试
 *===========================================================================*/

typedef struct {
    osal_flock_t *flock;
    int32_t *shared_counter;
    int32_t iterations;
} flock_thread_ctx_t;

static void *flock_exclusive_thread(void *arg)
{
    flock_thread_ctx_t *ctx = (flock_thread_ctx_t *)arg;

    for (int i = 0; i < ctx->iterations; i++) {
        OSAL_flock_lock(ctx->flock, OSAL_FLOCK_EXCLUSIVE);
        (*ctx->shared_counter)++;
        OSAL_flock_unlock(ctx->flock);
    }

    return NULL;
}

static void test_flock_multiple_threads_exclusive(void)
{
    osal_flock_t *flock = NULL;
    int32_t shared_counter = 0;
    flock_thread_ctx_t ctx[3];
    osal_thread_t threads[3];
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 创建3个线程，每个递增100次 */
    for (int i = 0; i < 3; i++) {
        ctx[i].flock = flock;
        ctx[i].shared_counter = &shared_counter;
        ctx[i].iterations = 100;
        OSAL_pthread_create(&threads[i], NULL, flock_exclusive_thread, &ctx[i]);
    }

    /* 等待所有线程完成 */
    for (int i = 0; i < 3; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证计数正确（无竞态条件） */
    TEST_ASSERT_EQUAL(300, shared_counter);

    OSAL_flock_destroy(flock);
}

/*===========================================================================
 * 实际场景测试
 *===========================================================================*/

static void test_flock_upgrade_lock(void)
{
    osal_flock_t *flock = NULL;
    int32_t ret;

    ret = OSAL_flock_create(TEST_LOCK_FILE, &flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 先获取共享锁 */
    ret = OSAL_flock_lock(flock, OSAL_FLOCK_SHARED);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 释放共享锁 */
    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 升级为独占锁 */
    ret = OSAL_flock_lock(flock, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 释放独占锁 */
    ret = OSAL_flock_unlock(flock);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_destroy(flock);
}

static void test_flock_different_files(void)
{
    osal_flock_t *flock1 = NULL;
    osal_flock_t *flock2 = NULL;
    int32_t ret;

    /* 创建两个不同的锁文件 */
    ret = OSAL_flock_create("/tmp/test_lock1.lock", &flock1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_create("/tmp/test_lock2.lock", &flock2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 两个锁应该互不影响 */
    ret = OSAL_flock_lock(flock1, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = OSAL_flock_lock(flock2, OSAL_FLOCK_EXCLUSIVE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    OSAL_flock_unlock(flock1);
    OSAL_flock_unlock(flock2);

    OSAL_flock_destroy(flock1);
    OSAL_flock_destroy(flock2);

    /* 清理锁文件 */
    unlink("/tmp/test_lock1.lock");
    unlink("/tmp/test_lock2.lock");
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_flock(void)
{

    /* 基础功能测试 */
    test_flock_create_destroy();
    test_flock_create_null_path();
    test_flock_create_null_output();
    test_flock_destroy_null_pointer();

    /* 独占锁测试 */
    test_flock_exclusive_lock_unlock();
    test_flock_try_lock_exclusive();
    test_flock_exclusive_blocks_exclusive();

    /* 共享锁测试 */
    test_flock_shared_lock_unlock();
    test_flock_multiple_shared_locks();
    test_flock_shared_blocks_exclusive();
    test_flock_exclusive_blocks_shared();

    /* 超时锁测试 */
    test_flock_timed_lock_success();
    test_flock_timed_lock_timeout();

    /* 错误处理测试 */
    test_flock_lock_null_pointer();
    test_flock_unlock_null_pointer();
    test_flock_try_lock_null_pointer();
    test_flock_timed_lock_null_pointer();

    /* 多线程测试 */
    test_flock_multiple_threads_exclusive();

    /* 实际场景测试 */
    test_flock_upgrade_lock();
    test_flock_different_files();

}
