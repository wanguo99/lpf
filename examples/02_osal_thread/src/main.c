#include <stdio.h>
#include "osal.h"

// 共享计数器
static int counter = 0;
static osal_mutex_t *mutex = NULL;

// 线程函数
static void* thread_func(void *arg)
{
    int thread_id = *(int*)arg;

    for (int i = 0; i < 5; i++) {
        // 加锁保护共享资源
        OSAL_MutexLock(mutex);
        counter++;
        int current = counter;
        OSAL_MutexUnlock(mutex);

        printf("Thread %d: Count %d\n", thread_id, current);

        // 休眠 100ms
        OSAL_Sleep(100);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("=================================\n");
    printf("  OSAL Thread Example\n");
    printf("=================================\n\n");

    // 创建互斥锁
    int32_t ret = OSAL_MutexCreate(&mutex);
    if (ret != 0) {
        printf("Failed to create mutex\n");
        return -1;
    }

    printf("Creating threads...\n");

    // 创建两个线程
    osal_thread_t *thread1 = NULL;
    osal_thread_t *thread2 = NULL;

    int id1 = 1, id2 = 2;

    ret = OSAL_ThreadCreate(&thread1, thread_func, &id1);
    if (ret != 0) {
        printf("Failed to create thread 1\n");
        OSAL_MutexDestroy(mutex);
        return -1;
    }

    ret = OSAL_ThreadCreate(&thread2, thread_func, &id2);
    if (ret != 0) {
        printf("Failed to create thread 2\n");
        OSAL_MutexDestroy(mutex);
        return -1;
    }

    // 等待线程结束
    OSAL_ThreadJoin(thread1);
    OSAL_ThreadJoin(thread2);

    printf("\nThreads finished.\n");
    printf("Final counter value: %d\n", counter);

    // 清理资源
    OSAL_MutexDestroy(mutex);

    printf("\nTry modifying this example:\n");
    printf("  1. Create more threads\n");
    printf("  2. Remove mutex and see what happens\n");
    printf("  3. Add semaphore for synchronization\n");

    return 0;
}
