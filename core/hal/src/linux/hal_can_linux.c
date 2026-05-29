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
#include "osal.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

typedef struct
{
    int32_t sockfd;
    char interface[IFNAMSIZ];
    uint32_t baudrate;
    bool initialized;
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

    impl->sockfd = OSAL_socket(OSAL_PF_CAN, OSAL_SOCK_RAW, OSAL_CAN_RAW);
    if (impl->sockfd < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to create socket: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(&ifr, 0, sizeof(ifr));
    OSAL_Strncpy(ifr.ifr_name, config->interface, IFNAMSIZ - 1);
    ret = OSAL_ioctl(impl->sockfd, SIOCGIFINDEX, &ifr);
    if (ret < 0)
    {
        LOG_ERROR("HAL_CAN", "Interface %s not found: %s", config->interface, OSAL_StrError(OSAL_GetErrno()));
        OSAL_close(impl->sockfd);
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(&addr, 0, sizeof(addr));
    addr.can_family = OSAL_AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    ret = OSAL_bind(impl->sockfd, (const osal_sockaddr_t *)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to bind interface: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_close(impl->sockfd);
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
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

    LOG_INFO("HAL_CAN", "Initialized successfully: %s @ %u bps", config->interface, config->baudrate);
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

    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    if (frame->dlc > 8)
    {
        LOG_ERROR("HAL_CAN", "Invalid DLC: %u", frame->dlc);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(&can_frame, 0, sizeof(can_frame));
    can_frame.can_id = frame->can_id;
    can_frame.can_dlc = frame->dlc;
    OSAL_Memcpy(can_frame.data, frame->data, frame->dlc);

    ret = OSAL_write(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret != sizeof(struct can_frame))
    {
        LOG_ERROR("HAL_CAN", "Send failed: %s", OSAL_StrError(OSAL_GetErrno()));
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t HAL_CAN_Recv(hal_can_handle_t handle, hal_can_frame_t *frame, int32_t timeout)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_frame can_frame;
    osal_ssize_t ret;

    if (NULL == impl || NULL == frame)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

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
            LOG_ERROR("HAL_CAN", "Poll failed: %s", OSAL_StrError(OSAL_GetErrno()));
            return OSAL_ERR_GENERIC;
        }
    }

    ret = OSAL_read(impl->sockfd, &can_frame, sizeof(struct can_frame));
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (err == OSAL_EAGAIN || err == OSAL_EWOULDBLOCK)
            return OSAL_ERR_TIMEOUT;
        LOG_ERROR("HAL_CAN", "Receive failed: %s", OSAL_StrError(err));
        return OSAL_ERR_GENERIC;
    }

    if (ret != sizeof(struct can_frame))
    {
        LOG_ERROR("HAL_CAN", "Incomplete receive: %d/%u bytes", (int32_t)ret, (uint32_t)sizeof(struct can_frame));
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(frame, 0, sizeof(hal_can_frame_t));
    frame->can_id = can_frame.can_id;
    frame->dlc = (can_frame.can_dlc > 8) ? 8 : can_frame.can_dlc;
    OSAL_Memcpy(frame->data, can_frame.data, frame->dlc);
    frame->timestamp = OSAL_GetTickCount();

    return OSAL_SUCCESS;
}

int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask)
{
    hal_can_context_t *impl = (hal_can_context_t *)handle;
    struct can_filter rfilter;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || impl->sockfd < 0)
        return OSAL_ERR_INVALID_ID;

    rfilter.can_id = filter_id;
    rfilter.can_mask = filter_mask;

    if (OSAL_setsockopt(impl->sockfd, OSAL_SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
    {
        LOG_ERROR("HAL_CAN", "Failed to set filter: %s", OSAL_StrError(OSAL_GetErrno()));
        return OSAL_ERR_GENERIC;
    }

    LOG_INFO("HAL_CAN", "Filter set: ID=0x%X, Mask=0x%X", filter_id, filter_mask);
    return OSAL_SUCCESS;
}
