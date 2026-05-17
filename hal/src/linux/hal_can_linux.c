/************************************************************************
 * HAL层 - CAN驱动Linux实现
 ************************************************************************/

/* 必须在所有头文件之前定义，以启用完整的POSIX功能 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <net/if.h>           /* struct ifreq 定义 */
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>        /* SIOCGIFINDEX 宏定义 */
#include <poll.h>             /* poll() 函数 */
#include <stdatomic.h>        /* C11 原子操作 */
#include "hal_can.h"
#include "osal.h"

/* 兼容性定义 */
#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

typedef struct
{
    int32_t sockfd;
    _Atomic uint32_t tx_count;      /* 使用 C11 原子操作，固定大小类型 */
    _Atomic uint32_t rx_count;      /* 使用 C11 原子操作，固定大小类型 */
    _Atomic uint32_t err_count;     /* 使用 C11 原子操作，固定大小类型 */
    char interface[IFNAMSIZ];
    uint32_t baudrate;
    bool initialized;
    uint32_t consecutive_errors;
    uint32_t error_threshold;
    void (*error_callback)(hal_can_handle_t handle, int32_t error_code);
} hal_can_context_t;

/**
 * @brief CAN总线错误恢复
 */
static int32_t hal_can_recover(hal_can_handle_t handle)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct sockaddr_can addr;
    struct ifreq ifr;

    LOG_INFO("HAL_CAN", "Attempting CAN bus recovery on %s...", impl->interface);

    /* 关闭旧连接 */
    if (impl->sockfd >= 0)
    {
        OSAL_close(impl->sockfd);
    }

    /* 重新创建socket */
    impl->sockfd = OSAL_socket(OSAL_PF_CAN, OSAL_SOCK_RAW, OSAL_CAN_RAW);
    if (impl->sockfd < 0)
    {
        LOG_ERROR("HAL_CAN", "Recovery failed: cannot create socket");
        atomic_fetch_add(&impl->err_count, 1);
        return OSAL_ERR_GENERIC;
    }

    /* 获取接口索引 */
    OSAL_Memset(&ifr, 0, sizeof(ifr));
    OSAL_Strncpy(ifr.ifr_name, (const char *)impl->interface, IFNAMSIZ - 1);
    if (OSAL_ioctl(impl->sockfd, SIOCGIFINDEX, &ifr) < 0)
    {
        LOG_ERROR("HAL_CAN", "Recovery failed: interface %s not found", impl->interface);
        OSAL_close(impl->sockfd);
        impl->sockfd = -1;
        atomic_fetch_add(&impl->err_count, 1);
        return OSAL_ERR_GENERIC;
    }

    /* 绑定到CAN接口 */
    OSAL_Memset(&addr, 0, sizeof(addr));
    addr.can_family = OSAL_AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (OSAL_bind(impl->sockfd, (const osal_sockaddr_t *)&addr, sizeof(addr)) < 0)
    {
        LOG_ERROR("HAL_CAN", "Recovery failed: cannot bind to %s", impl->interface);
        OSAL_close(impl->sockfd);
        impl->sockfd = -1;
        atomic_fetch_add(&impl->err_count, 1);
        return OSAL_ERR_GENERIC;
    }

    /* 重置错误计数 */
    impl->consecutive_errors = 0;
    LOG_INFO("HAL_CAN", "CAN bus recovery successful on %s", impl->interface);

    return OSAL_SUCCESS;
}

/**
 * @brief 初始化CAN驱动
 */
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
    hal_can_context_t *impl;
    struct sockaddr_can addr;
    struct ifreq ifr;
    int32_t ret;

    /* 参数检查 */
    if (NULL == config || NULL == handle)
        return OSAL_ERR_INVALID_POINTER;

    if (NULL == config->interface || 0 == OSAL_Strlen(config->interface))
        return OSAL_ERR_GENERIC;

    if (OSAL_Strlen(config->interface) >= IFNAMSIZ)
        return OSAL_ERR_NAME_TOO_LONG;

    /* 分配句柄 */
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
    impl->consecutive_errors = 0;
    impl->error_threshold = 10;  /* 默认连续10次错误触发恢复 */
    impl->error_callback = NULL;

    /* 创建SocketCAN */
    impl->sockfd = OSAL_socket(OSAL_PF_CAN, OSAL_SOCK_RAW, OSAL_CAN_RAW);
    if (impl->sockfd < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to create socket: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    /* 禁用 SO_REUSEADDR，防止多进程绑定同一CAN接口 */
    int32_t reuse = 0;
    if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        LOG_WARN("HAL_CAN", "Failed to disable SO_REUSEADDR: %s", OSAL_StrError(OSAL_GetErrno()));
    }

    /* 获取接口索引 */
    OSAL_Memset(&ifr, 0, sizeof(ifr));
    OSAL_Strncpy(ifr.ifr_name, config->interface, IFNAMSIZ - 1);
    ret = OSAL_ioctl(impl->sockfd, SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to get interface index: %s (interface: %s)",
                   OSAL_StrError(OSAL_GetErrno()), config->interface);
        /* 提示: CAN接口必须先启动 (sudo ip link set can0 up) */
        OSAL_close(impl->sockfd);
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    /* 绑定到CAN接口 */
    OSAL_Memset(&addr, 0, sizeof(addr));
    addr.can_family = OSAL_AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    ret = OSAL_bind(impl->sockfd, (const osal_sockaddr_t *)&addr, sizeof(addr));
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (OSAL_EADDRINUSE == err)
        {
            LOG_ERROR("HAL_CAN", "CAN interface %s is already in use by another process", config->interface);
            OSAL_close(impl->sockfd);
            OSAL_Free(impl);
            return OSAL_ERR_BUSY;
        }
        LOG_ERROR("HAL_CAN", "Failed to bind interface: %s", OSAL_StrError(err));
        OSAL_close(impl->sockfd);
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    /* 设置接收超时 */
    if (config->rx_timeout > 0)
    {
        struct timeval tv;
        tv.tv_sec = config->rx_timeout / 1000;
        tv.tv_usec = (config->rx_timeout % 1000) * 1000;

        if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            LOG_WARN("HAL_CAN", "Failed to set receive timeout: %s", OSAL_StrError(OSAL_GetErrno()));
        }
    }

    if (config->tx_timeout > 0)
    {
        struct timeval tv;
        tv.tv_sec = config->tx_timeout / 1000;
        tv.tv_usec = (config->tx_timeout % 1000) * 1000;

        if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        {
            LOG_WARN("HAL_CAN", "Failed to set send timeout: %s", OSAL_StrError(OSAL_GetErrno()));
        }
    }

    impl->initialized = true;
    *handle = (hal_can_handle_t)impl;

    LOG_INFO("HAL_CAN", "Initialized successfully: %s @ %u bps",
              config->interface, config->baudrate);
    return OSAL_SUCCESS;
}

/**
 * @brief 关闭CAN驱动
 */
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

    impl->initialized = false;
    OSAL_Free(impl);

    LOG_INFO("HAL_CAN", "Deinitialized successfully");
    return OSAL_SUCCESS;
}

/**
 * @brief 发送CAN帧
 */
int32_t HAL_CAN_Send(hal_can_handle_t handle, const can_frame_t *frame)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_frame can_frame;
    osal_ssize_t ret;

    /* 参数检查 */
    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    if (frame->dlc > 8)
    {
        LOG_ERROR("HAL_CAN", "Invalid DLC: %u", frame->dlc);
        return OSAL_ERR_GENERIC;
    }

    /* 转换为SocketCAN格式 */
    OSAL_Memset(&can_frame, 0, sizeof(can_frame));
    can_frame.can_id = frame->can_id;
    can_frame.can_dlc = frame->dlc;
    OSAL_Memcpy(can_frame.data, frame->data, frame->dlc);

    /* 发送 */
    ret = OSAL_write(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret != sizeof(struct can_frame))
    {
        if (ret < 0)
        {
            LOG_ERROR("HAL_CAN", "Send failed: %s", OSAL_StrError(OSAL_GetErrno()));
        }
        else
        {
            LOG_ERROR("HAL_CAN", "Incomplete send: %d/%u bytes",
                       (int32_t)ret, (uint32_t)sizeof(struct can_frame));
        }
        atomic_fetch_add(&impl->err_count, 1);

        /* 错误恢复机制 */
        impl->consecutive_errors++;
        if (impl->consecutive_errors >= impl->error_threshold)
        {
            LOG_WARN("HAL_CAN", "Consecutive errors (%u) reached threshold, attempting recovery",
                     impl->consecutive_errors);
            if (impl->error_callback)
            {
                impl->error_callback(handle, OSAL_ERR_GENERIC);
            }
            hal_can_recover(handle);
        }

        return OSAL_ERR_GENERIC;
    }

    /* 成功时重置错误计数 */
    impl->consecutive_errors = 0;
    atomic_fetch_add(&impl->tx_count, 1);
    return OSAL_SUCCESS;
}

/**
 * @brief 接收CAN帧
 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, can_frame_t *frame, int32_t timeout)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_frame can_frame;
    osal_ssize_t ret;

    /* 参数检查 */
    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    /* 使用 poll() 实现超时，避免修改 socket 属性导致的并发问题 */
    if (timeout >= 0)
    {
        struct pollfd pfd = {
            .fd = impl->sockfd,
            .events = POLLIN,
        };

        int32_t poll_ret = poll(&pfd, 1, timeout);
        if (poll_ret == 0)
        {
            return OSAL_ERR_TIMEOUT;
        }
        else if (poll_ret < 0)
        {
            LOG_ERROR("HAL_CAN", "Poll failed: %s", OSAL_StrError(OSAL_GetErrno()));
            atomic_fetch_add(&impl->err_count, 1);

            /* 错误恢复机制 */
            impl->consecutive_errors++;
            if (impl->consecutive_errors >= impl->error_threshold)
            {
                LOG_WARN("HAL_CAN", "Consecutive errors (%u) reached threshold, attempting recovery",
                         impl->consecutive_errors);
                if (impl->error_callback)
                {
                    impl->error_callback(handle, OSAL_ERR_GENERIC);
                }
                hal_can_recover(handle);
            }

            return OSAL_ERR_GENERIC;
        }
    }

    /* 接收 */
    ret = OSAL_read(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (err == OSAL_EAGAIN || err == OSAL_EWOULDBLOCK)
            return OSAL_ERR_TIMEOUT;

        LOG_ERROR("HAL_CAN", "Receive failed: %s", OSAL_StrError(err));
        atomic_fetch_add(&impl->err_count, 1);

        /* 错误恢复机制 */
        impl->consecutive_errors++;
        if (impl->consecutive_errors >= impl->error_threshold)
        {
            LOG_WARN("HAL_CAN", "Consecutive errors (%u) reached threshold, attempting recovery",
                     impl->consecutive_errors);
            if (impl->error_callback)
            {
                impl->error_callback(handle, OSAL_ERR_GENERIC);
            }
            hal_can_recover(handle);
        }

        return OSAL_ERR_GENERIC;
    }

    if (ret != sizeof(struct can_frame))
    {
        LOG_ERROR("HAL_CAN", "Incomplete receive: %d/%u bytes",
                   (int32_t)ret, (uint32_t)sizeof(struct can_frame));
        atomic_fetch_add(&impl->err_count, 1);

        /* 错误恢复机制 */
        impl->consecutive_errors++;
        if (impl->consecutive_errors >= impl->error_threshold)
        {
            LOG_WARN("HAL_CAN", "Consecutive errors (%u) reached threshold, attempting recovery",
                     impl->consecutive_errors);
            if (impl->error_callback)
            {
                impl->error_callback(handle, OSAL_ERR_GENERIC);
            }
            hal_can_recover(handle);
        }

        return OSAL_ERR_GENERIC;
    }

    /* 转换为内部格式 */
    OSAL_Memset(frame, 0, sizeof(can_frame_t));
    frame->can_id = can_frame.can_id;

    /* 先校验 DLC，再赋值和拷贝数据 */
    uint8_t dlc = can_frame.can_dlc;
    if (dlc > 8) {
        LOG_WARN("HAL_CAN", "Invalid DLC %u, clamping to 8", dlc);
        dlc = 8;
    }

    frame->dlc = dlc;
    OSAL_Memcpy(frame->data, can_frame.data, dlc);
    frame->timestamp = OSAL_GetTickCount();

    /* 成功时重置错误计数 */
    impl->consecutive_errors = 0;
    atomic_fetch_add(&impl->rx_count, 1);
    return OSAL_SUCCESS;
}

/**
 * @brief 设置CAN过滤器
 */
int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_filter rfilter[1];

    if (NULL == impl)
        return OSAL_ERR_INVALID_ID;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    rfilter[0].can_id = filter_id;
    rfilter[0].can_mask = filter_mask;

    if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_CAN_RAW, OSAL_CAN_RAW_FILTER,
                   &rfilter, sizeof(rfilter)) < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to set filter: %s", OSAL_StrError(OSAL_GetErrno()));
        return OSAL_ERR_GENERIC;
    }

    LOG_INFO("HAL_CAN", "Filter set: ID=0x%X, Mask=0x%X",
              filter_id, filter_mask);
    return OSAL_SUCCESS;
}

/**
 * @brief 获取CAN统计信息
 */
int32_t HAL_CAN_GetStats(hal_can_handle_t handle,
                       uint32_t *tx_count,
                       uint32_t *rx_count,
                       uint32_t *err_count)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_ID;

    if (!impl->initialized)
        return OSAL_ERR_INVALID_ID;

    /* 原子读取统计信息 */
    if (tx_count)  *tx_count = atomic_load(&impl->tx_count);
    if (rx_count)  *rx_count = atomic_load(&impl->rx_count);
    if (err_count) *err_count = atomic_load(&impl->err_count);

    return OSAL_SUCCESS;
}

/**
 * @brief 设置CAN错误回调函数
 */
int32_t HAL_CAN_SetErrorCallback(hal_can_handle_t handle,
                                  void (*callback)(hal_can_handle_t handle, int32_t error_code))
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized)
        return OSAL_ERR_INVALID_ID;

    impl->error_callback = callback;
    LOG_INFO("HAL_CAN", "Error callback %s", callback ? "set" : "cleared");

    return OSAL_SUCCESS;
}

/**
 * @brief 设置CAN错误恢复阈值
 */
int32_t HAL_CAN_SetErrorThreshold(hal_can_handle_t handle, uint32_t threshold)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized)
        return OSAL_ERR_INVALID_ID;

    impl->error_threshold = threshold;
    LOG_INFO("HAL_CAN", "Error threshold set to %u", threshold);

    return OSAL_SUCCESS;
}
