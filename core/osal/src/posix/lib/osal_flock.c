/**
 * @file osal_flock.c
 * @brief OSAL 文件锁实现（基于 fcntl）
 */

#include "osal.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief 文件锁内部结构
 */
struct osal_flock_s {
    int fd;                       /* 锁文件描述符 */
    char lock_file[256];          /* 锁文件路径 */
    osal_flock_type_t current_type; /* 当前锁类型 */
    bool is_locked;               /* 是否已加锁 */
};

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 执行 fcntl 加锁操作
 */
static int32_t flock_do_lock(int fd, osal_flock_type_t type, int cmd)
{
    struct flock lock;

    OSAL_memset(&lock, 0, OSAL_sizeof(lock));
    lock.l_type = (type == OSAL_FLOCK_SHARED) ? F_RDLCK : F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;  /* 锁整个文件 */

    if (fcntl(fd, cmd, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            return OSAL_ERR_BUSY;
        }
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 执行 fcntl 解锁操作
 */
static int32_t flock_do_unlock(int fd)
{
    struct flock lock;

    OSAL_memset(&lock, 0, OSAL_sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 公共 API 实现
 *===========================================================================*/

/**
 * @brief 创建文件锁
 */
int32_t OSAL_flock_create(const char *lock_file, osal_flock_t **flock)
{
    if (!lock_file || !flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 分配内存 */
    osal_flock_t *fl = (osal_flock_t *)OSAL_malloc(OSAL_sizeof(osal_flock_t));
    if (!fl) {
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(fl, 0, OSAL_sizeof(osal_flock_t));

    /* 保存锁文件路径 */
    OSAL_strncpy(fl->lock_file, lock_file, OSAL_sizeof(fl->lock_file) - 1);

    /* 打开或创建锁文件 */
    fl->fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (fl->fd < 0) {
        OSAL_free(fl);
        return OSAL_ERR_GENERIC;
    }

    fl->is_locked = false;

    *flock = fl;
    return OSAL_SUCCESS;
}

/**
 * @brief 销毁文件锁
 */
int32_t OSAL_flock_destroy(osal_flock_t *flock)
{
    if (!flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 如果还持有锁，先释放 */
    if (flock->is_locked) {
        OSAL_flock_unlock(flock);
    }

    /* 关闭文件描述符 */
    if (flock->fd >= 0) {
        close(flock->fd);
    }

    /* 释放内存 */
    OSAL_free(flock);

    return OSAL_SUCCESS;
}

/**
 * @brief 加锁（阻塞模式）
 */
int32_t OSAL_flock_lock(osal_flock_t *flock, osal_flock_type_t type)
{
    if (!flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 使用 F_SETLKW（阻塞等待） */
    int32_t ret = flock_do_lock(flock->fd, type, F_SETLKW);
    if (ret == OSAL_SUCCESS) {
        flock->is_locked = true;
        flock->current_type = type;
    }

    return ret;
}

/**
 * @brief 尝试加锁（非阻塞模式）
 */
int32_t OSAL_flock_try_lock(osal_flock_t *flock, osal_flock_type_t type)
{
    if (!flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 使用 F_SETLK（非阻塞） */
    int32_t ret = flock_do_lock(flock->fd, type, F_SETLK);
    if (ret == OSAL_SUCCESS) {
        flock->is_locked = true;
        flock->current_type = type;
    }

    return ret;
}

/**
 * @brief 带超时的加锁
 */
int32_t OSAL_flock_timed_lock(osal_flock_t *flock, osal_flock_type_t type,
                            uint32_t timeout_ms)
{
    if (!flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 如果超时为 0，使用非阻塞模式 */
    if (timeout_ms == 0) {
        return OSAL_flock_try_lock(flock, type);
    }

    /* 记录开始时间 */
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* 轮询尝试加锁 */
    while (1) {
        int32_t ret = OSAL_flock_try_lock(flock, type);

        if (ret == OSAL_SUCCESS) {
            return OSAL_SUCCESS;
        }

        if (ret != OSAL_ERR_BUSY) {
            return ret;
        }

        /* 检查超时 */
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                              (now.tv_nsec - start.tv_nsec) / 1000000;

        if (elapsed_ms >= timeout_ms) {
            return OSAL_ERR_TIMEOUT;
        }

        /* 短暂休眠，避免 CPU 空转 */
        struct timespec sleep_time = {
            .tv_sec = 0,
            .tv_nsec = 10000000  /* 10ms */
        };
        nanosleep(&sleep_time, NULL);
    }
}

/**
 * @brief 解锁
 */
int32_t OSAL_flock_unlock(osal_flock_t *flock)
{
    if (!flock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (!flock->is_locked) {
        return OSAL_SUCCESS;  /* 已经解锁 */
    }

    int32_t ret = flock_do_unlock(flock->fd);
    if (ret == OSAL_SUCCESS) {
        flock->is_locked = false;
    }

    return ret;
}
