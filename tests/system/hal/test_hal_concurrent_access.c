/**
 * @file test_hal_concurrent_access.c
 * @brief HAL 层并发访问测试
 *
 * 测试场景：
 * 1. 多进程并发访问同一 CAN 设备
 * 2. 多线程并发访问同一 I2C 设备
 * 3. 验证双重锁保护机制的有效性
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "osal.h"
#include "osal_flock.h"

/*===========================================================================
 * 测试 1：文件锁基本功能
 *===========================================================================*/

void test_flock_basic(void)
{
    printf("\n=== 测试 1: 文件锁基本功能 ===\n");

    osal_flock_t *flock = NULL;
    int32_t ret;

    /* 创建文件锁 */
    ret = OSAL_FlockCreate("/tmp/test_hal.lock", &flock);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 创建文件锁失败: %d\n", ret);
        return;
    }
    printf("✅ 创建文件锁成功\n");

    /* 加独占锁 */
    ret = OSAL_FlockLock(flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 加锁失败: %d\n", ret);
        OSAL_FlockDestroy(flock);
        return;
    }
    printf("✅ 加独占锁成功\n");

    /* 解锁 */
    ret = OSAL_FlockUnlock(flock);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 解锁失败: %d\n", ret);
    } else {
        printf("✅ 解锁成功\n");
    }

    /* 销毁文件锁 */
    ret = OSAL_FlockDestroy(flock);
    if (ret != OSAL_SUCCESS) {
        printf("❌ 销毁文件锁失败: %d\n", ret);
    } else {
        printf("✅ 销毁文件锁成功\n");
    }
}

/*===========================================================================
 * 测试 2：多进程并发访问
 *===========================================================================*/

void test_multiprocess_concurrent(void)
{
    printf("\n=== 测试 2: 多进程并发访问 ===\n");

    const char *lock_file = "/tmp/test_multiprocess.lock";
    const int num_processes = 3;
    const int iterations = 5;

    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            /* 子进程 */
            osal_flock_t *flock = NULL;

            if (OSAL_FlockCreate(lock_file, &flock) != OSAL_SUCCESS) {
                printf("进程 %d: 创建文件锁失败\n", getpid());
                exit(1);
            }

            for (int j = 0; j < iterations; j++) {
                /* 获取锁 */
                if (OSAL_FlockTimedLock(flock, OSAL_FLOCK_EXCLUSIVE, 5000) != OSAL_SUCCESS) {
                    printf("进程 %d: 加锁超时\n", getpid());
                    continue;
                }

                /* 临界区：模拟硬件访问 */
                printf("进程 %d: 进入临界区 (迭代 %d/%d)\n", getpid(), j+1, iterations);
                usleep(100000);  /* 100ms */

                /* 释放锁 */
                OSAL_FlockUnlock(flock);
            }

            OSAL_FlockDestroy(flock);
            exit(0);
        } else if (pid < 0) {
            printf("❌ fork 失败\n");
            return;
        }
    }

    /* 父进程等待所有子进程 */
    for (int i = 0; i < num_processes; i++) {
        int status;
        wait(&status);
    }

    printf("✅ 多进程并发测试完成\n");
}

/*===========================================================================
 * 测试 3：多线程并发访问
 *===========================================================================*/

typedef struct {
    int thread_id;
    osal_flock_t *flock;
    osal_mutex_t *mutex;
} thread_data_t;

void* thread_worker(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    const int iterations = 5;

    for (int i = 0; i < iterations; i++) {
        /* 双重锁保护 */
        if (OSAL_FlockTimedLock(data->flock, OSAL_FLOCK_EXCLUSIVE, 5000) != OSAL_SUCCESS) {
            printf("线程 %d: 文件锁超时\n", data->thread_id);
            continue;
        }

        OSAL_MutexLock(data->mutex);

        /* 临界区 */
        printf("线程 %d: 进入临界区 (迭代 %d/%d)\n", data->thread_id, i+1, iterations);
        usleep(50000);  /* 50ms */

        /* 释放锁（逆序） */
        OSAL_MutexUnlock(data->mutex);
        OSAL_FlockUnlock(data->flock);
    }

    return NULL;
}

void test_multithread_concurrent(void)
{
    printf("\n=== 测试 3: 多线程并发访问 ===\n");

    const int num_threads = 3;
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    /* 创建共享的锁 */
    osal_flock_t *flock = NULL;
    osal_mutex_t *mutex = NULL;

    if (OSAL_FlockCreate("/tmp/test_multithread.lock", &flock) != OSAL_SUCCESS) {
        printf("❌ 创建文件锁失败\n");
        return;
    }

    if (OSAL_MutexCreate(&mutex) != OSAL_SUCCESS) {
        printf("❌ 创建互斥锁失败\n");
        OSAL_FlockDestroy(flock);
        return;
    }

    /* 创建线程 */
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i + 1;
        thread_data[i].flock = flock;
        thread_data[i].mutex = mutex;

        if (pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]) != 0) {
            printf("❌ 创建线程 %d 失败\n", i);
        }
    }

    /* 等待所有线程完成 */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* 清理 */
    OSAL_MutexDestroy(mutex);
    OSAL_FlockDestroy(flock);

    printf("✅ 多线程并发测试完成\n");
}

/*===========================================================================
 * 测试 4：超时机制
 *===========================================================================*/

void test_timeout_mechanism(void)
{
    printf("\n=== 测试 4: 超时机制 ===\n");

    osal_flock_t *flock = NULL;

    if (OSAL_FlockCreate("/tmp/test_timeout.lock", &flock) != OSAL_SUCCESS) {
        printf("❌ 创建文件锁失败\n");
        return;
    }

    /* 第一个进程持有锁 */
    pid_t pid = fork();

    if (pid == 0) {
        /* 子进程：持有锁 3 秒 */
        OSAL_FlockLock(flock, OSAL_FLOCK_EXCLUSIVE);
        printf("子进程: 持有锁 3 秒...\n");
        sleep(3);
        OSAL_FlockUnlock(flock);
        printf("子进程: 释放锁\n");
        OSAL_FlockDestroy(flock);
        exit(0);
    } else {
        /* 父进程：等待子进程先获取锁 */
        sleep(1);

        /* 尝试获取锁（1 秒超时，应该失败） */
        printf("父进程: 尝试获取锁（1 秒超时）...\n");
        int32_t ret = OSAL_FlockTimedLock(flock, OSAL_FLOCK_EXCLUSIVE, 1000);
        if (ret == OSAL_ERR_TIMEOUT) {
            printf("✅ 超时机制正常工作\n");
        } else if (ret == OSAL_SUCCESS) {
            printf("❌ 不应该获取到锁\n");
            OSAL_FlockUnlock(flock);
        } else {
            printf("❌ 意外的错误: %d\n", ret);
        }

        /* 等待子进程结束 */
        wait(NULL);

        /* 现在应该能获取锁 */
        printf("父进程: 再次尝试获取锁...\n");
        ret = OSAL_FlockTimedLock(flock, OSAL_FLOCK_EXCLUSIVE, 1000);
        if (ret == OSAL_SUCCESS) {
            printf("✅ 成功获取锁\n");
            OSAL_FlockUnlock(flock);
        } else {
            printf("❌ 获取锁失败: %d\n", ret);
        }

        OSAL_FlockDestroy(flock);
    }
}

/*===========================================================================
 * 主函数
 *===========================================================================*/

int main(int argc, char *argv[])
{
    printf("========================================\n");
    printf("HAL 层并发访问测试\n");
    printf("========================================\n");

    /* 运行所有测试 */
    test_flock_basic();
    test_multiprocess_concurrent();
    test_multithread_concurrent();
    test_timeout_mechanism();

    printf("\n========================================\n");
    printf("所有测试完成\n");
    printf("========================================\n");

    return 0;
}
