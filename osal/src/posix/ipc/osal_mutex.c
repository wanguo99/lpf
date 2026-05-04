/************************************************************************
 * OSAL POSIX实现 - 互斥锁
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

typedef struct
{
    bool          is_used;
    osal_id_t     id;
    char          name[OS_MAX_API_NAME];
    pthread_mutex_t mutex;
    bool          valid;
} osal_mutex_record_t;

static osal_mutex_record_t g_osal_mutex_table[OS_MAX_MUTEXES] = {0};  /* 静态变量自动初始化为0 */
static pthread_mutex_t g_mutex_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t g_next_mutex_id = 1;

static uint32_t g_deadlock_threshold_msec = OSAL_MUTEX_DEADLOCK_TIMEOUT_MSEC;
static deadlock_callback_t g_deadlock_callback = NULL;

/* 移除 osal_mutex_table_init() - 静态变量已自动初始化 */

int32_t OSAL_MutexCreate(osal_id_t *mutex_id, const char *mutex_name,
                     uint32_t flags __attribute__((unused)))
{
    uint32_t slot = 0;
    bool found_slot = false;

    if (NULL == mutex_id)
        return OSAL_ERR_INVALID_POINTER;

    if (NULL == mutex_name || strlen(mutex_name) >= OS_MAX_API_NAME)
        return OSAL_ERR_NAME_TOO_LONG;

    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (!g_osal_mutex_table[i].is_used)
        {
            slot = i;
            found_slot = true;
            break;
        }
    }

    if (!found_slot)
    {
        pthread_mutex_unlock(&g_mutex_table_mutex);
        return OSAL_ERR_NO_FREE_IDS;
    }

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used &&
            0 == strcmp(g_osal_mutex_table[i].name, mutex_name))
        {
            pthread_mutex_unlock(&g_mutex_table_mutex);
            return OSAL_ERR_NAME_TAKEN;
        }
    }

    if (0 != pthread_mutex_init(&g_osal_mutex_table[slot].mutex, NULL))
    {
        pthread_mutex_unlock(&g_mutex_table_mutex);
        return OSAL_ERR_GENERIC;
    }

    g_osal_mutex_table[slot].is_used = true;
    g_osal_mutex_table[slot].id = g_next_mutex_id++;
    g_osal_mutex_table[slot].valid = true;
    strncpy(g_osal_mutex_table[slot].name, mutex_name, OS_MAX_API_NAME - 1);
    g_osal_mutex_table[slot].name[OS_MAX_API_NAME - 1] = '\0';

    *mutex_id = g_osal_mutex_table[slot].id;

    pthread_mutex_unlock(&g_mutex_table_mutex);

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexDelete(osal_id_t mutex_id)
{
    pthread_mutex_t *mutex_to_destroy = NULL;
    uint32_t slot_to_clear = OS_MAX_MUTEXES;

    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used && g_osal_mutex_table[i].id == mutex_id)
        {
            g_osal_mutex_table[i].valid = false;
            mutex_to_destroy = &g_osal_mutex_table[i].mutex;
            slot_to_clear = i;
            break;
        }
    }

    pthread_mutex_unlock(&g_mutex_table_mutex);

    if (NULL == mutex_to_destroy)
        return OSAL_ERR_INVALID_ID;

    /* 在锁外销毁互斥锁 */
    pthread_mutex_destroy(mutex_to_destroy);

    /* 清理表项 */
    pthread_mutex_lock(&g_mutex_table_mutex);
    if (slot_to_clear < OS_MAX_MUTEXES)
    {
        g_osal_mutex_table[slot_to_clear].is_used = false;
    }
    pthread_mutex_unlock(&g_mutex_table_mutex);

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexLock(osal_id_t mutex_id)
{
    pthread_mutex_t *target_mutex = NULL;
    bool is_valid = false;

    /* 查找互斥锁 */
    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used &&
            g_osal_mutex_table[i].id == mutex_id &&
            g_osal_mutex_table[i].valid)
        {
            target_mutex = &g_osal_mutex_table[i].mutex;
            is_valid = true;
            break;
        }
    }

    pthread_mutex_unlock(&g_mutex_table_mutex);

    if (!is_valid || NULL == target_mutex)
        return OSAL_ERR_INVALID_ID;

    /* 在锁外获取用户互斥锁 */
    if (0 != pthread_mutex_lock(target_mutex))
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexUnlock(osal_id_t mutex_id)
{
    pthread_mutex_t *target_mutex = NULL;
    bool is_valid = false;

    /* 查找互斥锁 */
    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used &&
            g_osal_mutex_table[i].id == mutex_id &&
            g_osal_mutex_table[i].valid)
        {
            target_mutex = &g_osal_mutex_table[i].mutex;
            is_valid = true;
            break;
        }
    }

    pthread_mutex_unlock(&g_mutex_table_mutex);

    if (!is_valid || NULL == target_mutex)
        return OSAL_ERR_INVALID_ID;

    /* 在锁外释放用户互斥锁 */
    if (0 != pthread_mutex_unlock(target_mutex))
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexGetIdByName(osal_id_t *mutex_id, const char *mutex_name)
{
    if (NULL == mutex_id || NULL == mutex_name)
        return OSAL_ERR_INVALID_POINTER;

    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used &&
            0 == strcmp(g_osal_mutex_table[i].name, mutex_name))
        {
            *mutex_id = g_osal_mutex_table[i].id;
            pthread_mutex_unlock(&g_mutex_table_mutex);
            return OSAL_SUCCESS;
        }
    }

    pthread_mutex_unlock(&g_mutex_table_mutex);
    return OSAL_ERR_NAME_NOT_FOUND;
}

int32_t OSAL_MutexLockTimeout(osal_id_t mutex_id, uint32_t timeout_msec)
{
    pthread_mutex_t *target_mutex = NULL;
    bool is_valid = false;
    char mutex_name[OS_MAX_API_NAME] = {0};
    struct timespec start_time, current_time, abs_timeout;
    int32_t ret;

    pthread_mutex_lock(&g_mutex_table_mutex);

    for (uint32_t i = 0; i < OS_MAX_MUTEXES; i++)
    {
        if (g_osal_mutex_table[i].is_used &&
            g_osal_mutex_table[i].id == mutex_id &&
            g_osal_mutex_table[i].valid)
        {
            target_mutex = &g_osal_mutex_table[i].mutex;
            strncpy(mutex_name, g_osal_mutex_table[i].name, OS_MAX_API_NAME - 1);
            is_valid = true;
            break;
        }
    }

    pthread_mutex_unlock(&g_mutex_table_mutex);

    if (!is_valid || NULL == target_mutex)
        return OSAL_ERR_INVALID_ID;

    clock_gettime(CLOCK_REALTIME, &start_time);
    abs_timeout.tv_sec = start_time.tv_sec + timeout_msec / OSAL_MSEC_PER_SEC;
    abs_timeout.tv_nsec = start_time.tv_nsec + (timeout_msec % OSAL_MSEC_PER_SEC) * OSAL_NSEC_PER_MSEC;
    if (abs_timeout.tv_nsec >= OSAL_NSEC_PER_SEC)
    {
        abs_timeout.tv_sec++;
        abs_timeout.tv_nsec -= OSAL_NSEC_PER_SEC;
    }

#ifdef __APPLE__
    /* macOS 不支持 pthread_mutex_timedlock，使用 trylock + sleep 模拟 */
    struct timespec sleep_time = {0, 1000000};  /* 1ms */
    while (1)
    {
        ret = pthread_mutex_trylock(target_mutex);
        if (ret == 0)
        {
            break;  /* 成功获取锁 */
        }

        /* 检查是否超时 */
        clock_gettime(CLOCK_REALTIME, &current_time);
        if (current_time.tv_sec > abs_timeout.tv_sec ||
            (current_time.tv_sec == abs_timeout.tv_sec &&
             current_time.tv_nsec >= abs_timeout.tv_nsec))
        {
            ret = ETIMEDOUT;
            break;
        }

        /* 短暂休眠后重试 */
        nanosleep(&sleep_time, NULL);
    }
#else
    ret = pthread_mutex_timedlock(target_mutex, &abs_timeout);
#endif

    if (ETIMEDOUT == ret)
    {
        clock_gettime(CLOCK_REALTIME, &current_time);
        uint32_t wait_time = (current_time.tv_sec - start_time.tv_sec) * OSAL_MSEC_PER_SEC +
                          (current_time.tv_nsec - start_time.tv_nsec) / OSAL_NSEC_PER_MSEC;

        /* 读取死锁检测配置时加锁保护 */
        pthread_mutex_lock(&g_mutex_table_mutex);
        uint32_t threshold = g_deadlock_threshold_msec;
        deadlock_callback_t callback = g_deadlock_callback;
        pthread_mutex_unlock(&g_mutex_table_mutex);

        if (wait_time >= threshold && NULL != callback)
        {
            callback(mutex_name, wait_time);
        }

        return OSAL_ERR_TIMEOUT;
    }
    else if (0 != ret)
    {
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexSetDeadlockDetection(uint32_t threshold_msec, deadlock_callback_t callback)
{
    pthread_mutex_lock(&g_mutex_table_mutex);
    g_deadlock_threshold_msec = threshold_msec;
    g_deadlock_callback = callback;
    pthread_mutex_unlock(&g_mutex_table_mutex);
    return OSAL_SUCCESS;
}
