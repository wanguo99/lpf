/************************************************************************
 * HAL层 - CAN驱动Linux实现（简化版）
 ************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <poll.h>
#include "hal_can.h"
#include "hal_error.h"
#include "osal.h"
#include "osal_flock.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

/**
 * @brief CAN 设备上下文（带双重保护）
 */
typedef struct
{
    int32_t sockfd;
    char interface[IFNAMSIZ];
    uint32_t baudrate;
    bool initialized;

    /* 双重保护机制 */
    osal_flock_t *flock;    /* 文件锁（进程间保护） */
    osal_mutex_t *mutex;    /* 互斥锁（线程间保护） */
} hal_can_context_t;

int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
    hal_can_context_t *impl;
    struct sockaddr_can addr;
    struct ifreq ifr;
    int32_t ret;

    if (NULL == config || NULL == handle)
        return OSAL_ERR_INVALID_POINTER;

    if (NULL == config->interface || 0 == OSAL_Strlen(config->interface))
        return OSAL_ERR_GENERIC;

    if (OSAL_Strlen(config->interface) >= IFNAMSIZ)
        return OSAL_ERR_NAME_TOO_LONG;

    impl = (hal_can_context_t *)OSAL_Malloc(sizeof(hal_can_context_t));
    if (NULL == impl)
    {
        LOG_ERROR("HAL_CAN", "Failed to allocate memory");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(impl, 0, sizeof(hal_can_context_t));
    OSAL_Strncpy(impl->interface, config->interface, IFNAMSIZ - 1);
    impl->interface[IFNAMSIZ - 1] = '\0';
    impl->baudrate = config->baudrate;
    impl->initialized = false;

    /* 创建文件锁（进程间保护） */
    char lock_file[OSAL_LOCK_PATH_MAX_LEN];
    OSAL_Snprintf(lock_file, sizeof(lock_file), HAL_CAN_LOCK_PATH_FMT, config->interface);
    ret = OSAL_FlockCreate(lock_file, &impl->flock);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to create file lock: %s", lock_file);
        OSAL_Free(impl);
        return ret;
    }

    /* 创建互斥锁（线程间保护） */
    ret = OSAL_MutexCreate(&impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to create mutex");
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return ret;
    }

    impl->sockfd = OSAL_socket(OSAL_PF_CAN, OSAL_SOCK_RAW, OSAL_CAN_RAW);
    if (impl->sockfd < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to create socket: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_CAN", "Failed to create socket: %s (errno=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return hal_err;
    }

    OSAL_Memset(&ifr, 0, sizeof(ifr));
    OSAL_Strncpy(ifr.ifr_name, config->interface, IFNAMSIZ - 1);
    ret = OSAL_ioctl(impl->sockfd, SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Interface %s not found: %s",
                      config->interface, OSAL_StrError(err));
        LOG_ERROR("HAL_CAN", "Interface %s not found: %s (errno=%d, hal_err=%d)",
                  config->interface, OSAL_StrError(err), err, hal_err);
        OSAL_close(impl->sockfd);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return hal_err;
    }

    OSAL_Memset(&addr, 0, sizeof(addr));
    addr.can_family = OSAL_AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    ret = OSAL_bind(impl->sockfd, (const osal_sockaddr_t *)&addr, sizeof(addr));
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to bind interface: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_CAN", "Failed to bind interface: %s (errno=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        OSAL_close(impl->sockfd);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return hal_err;
    }

    if (config->rx_timeout > 0)
    {
        struct timeval tv;
        tv.tv_sec = config->rx_timeout / 1000;
        tv.tv_usec = (config->rx_timeout % 1000) * 1000;
        OSAL_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO, &tv, sizeof(tv));
    }

    if (config->tx_timeout > 0)
    {
        struct timeval tv;
        tv.tv_sec = config->tx_timeout / 1000;
        tv.tv_usec = (config->tx_timeout % 1000) * 1000;
        OSAL_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO, &tv, sizeof(tv));
    }

    impl->initialized = true;
    *handle = (hal_can_handle_t)impl;

    LOG_INFO("HAL_CAN", "Initialized successfully: %s @ %u bps (with dual-lock protection)",
             config->interface, config->baudrate);
    return OSAL_SUCCESS;
}

int32_t HAL_CAN_Deinit(hal_can_handle_t handle)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_ID;

    if (impl->initialized && impl->sockfd >= 0)
    {
        OSAL_close(impl->sockfd);
        impl->sockfd = -1;
    }

    /* 销毁锁 */
    if (impl->mutex)
    {
        OSAL_MutexDelete(impl->mutex);
    }

    if (impl->flock)
    {
        OSAL_FlockDestroy(impl->flock);
    }

    impl->initialized = false;
    OSAL_Free(impl);

    LOG_INFO("HAL_CAN", "Deinitialized successfully");
    return OSAL_SUCCESS;
}

int32_t HAL_CAN_Send(hal_can_handle_t handle, const hal_can_frame_t *frame)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_frame can_frame;
    osal_ssize_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    if (frame->dlc > 8)
    {
        LOG_ERROR("HAL_CAN", "Invalid DLC: %u", frame->dlc);
        return OSAL_ERR_GENERIC;
    }

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_CAN_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：硬件访问 */
    OSAL_Memset(&can_frame, 0, sizeof(can_frame));
    can_frame.can_id = frame->can_id;
    can_frame.can_dlc = frame->dlc;
    OSAL_Memcpy(can_frame.data, frame->data, frame->dlc);

    ret = OSAL_write(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret != sizeof(struct can_frame))
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Send failed: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_CAN", "Send failed: %s (errno=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

int32_t HAL_CAN_Recv(hal_can_handle_t handle, hal_can_frame_t *frame, int32_t timeout)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_frame can_frame;
    osal_ssize_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    /*
     * 注意：poll 等待期间不应持有锁，否则会阻塞其他进程/线程
     * 策略：先 poll，有数据后再加锁读取
     */
    if (timeout >= 0)
    {
        struct pollfd pfd = {
            .fd = impl->sockfd,
            .events = POLLIN,
        };

        int32_t poll_ret = poll(&pfd, 1, timeout);
        if (poll_ret == 0)
            return OSAL_ERR_TIMEOUT;
        else if (poll_ret < 0)
        {
            int32_t err = OSAL_GetErrno();
            int32_t hal_err = HAL_ErrnoToError(err);
            HAL_SET_ERROR(hal_err, err, "Poll failed: %s", OSAL_StrError(err));
            LOG_ERROR("HAL_CAN", "Poll failed: %s (errno=%d, hal_err=%d)",
                      OSAL_StrError(err), err, hal_err);
            return hal_err;
        }
    }

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_CAN_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：读取数据 */
    ret = OSAL_read(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (err == OSAL_EAGAIN || err == OSAL_EWOULDBLOCK)
        {
            result = OSAL_ERR_TIMEOUT;
        }
        else
        {
            int32_t hal_err = HAL_ErrnoToError(err);
            HAL_SET_ERROR(hal_err, err, "Receive failed: %s", OSAL_StrError(err));
            LOG_ERROR("HAL_CAN", "Receive failed: %s (errno=%d, hal_err=%d)",
                      OSAL_StrError(err), err, hal_err);
            result = hal_err;
        }
    }
    else if (ret != sizeof(struct can_frame))
    {
        LOG_ERROR("HAL_CAN", "Incomplete receive: %d/%u bytes", (int32_t)ret, (uint32_t)sizeof(struct can_frame));
        result = OSAL_ERR_GENERIC;
    }
    else
    {
        OSAL_Memset(frame, 0, sizeof(hal_can_frame_t));
        frame->can_id = can_frame.can_id;
        frame->dlc = (can_frame.can_dlc > 8) ? 8 : can_frame.can_dlc;
        OSAL_Memcpy(frame->data, can_frame.data, frame->dlc);
        frame->timestamp = OSAL_GetTickCount();
        result = OSAL_SUCCESS;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_filter rfilter;
    int32_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_CAN_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：设置过滤器 */
    rfilter.can_id = filter_id;
    rfilter.can_mask = filter_mask;

    if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to set filter: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_CAN", "Failed to set filter: %s (errno=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
    }
    else
    {
        LOG_INFO("HAL_CAN", "Filter set: ID=0x%X, Mask=0x%X", filter_id, filter_mask);
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}
