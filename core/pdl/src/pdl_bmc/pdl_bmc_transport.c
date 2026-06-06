/************************************************************************
 * BMC传输层实现
 *
 * 职责：
 * - 封装底层网络（TCP Socket）和串口通信
 * - 提供统一的发送/接收接口供协议层使用
 * - 不涉及具体协议（IPMI/Redfish）的封装和解析
 ************************************************************************/

#include "pdl_bmc_internal.h"
#include "osal.h"
#include "hal.h"
#include "hal_serial.h"

/*
 * 网络传输上下文
 */
typedef struct
{
    int32_t sockfd;
    uint32_t timeout_ms;
} bmc_transport_net_context_t;

/*
 * 串口传输上下文
 */
typedef struct
{
    hal_serial_handle_t serial_handle;
    uint32_t timeout_ms;
} bmc_transport_serial_context_t;

/**
 * @brief 初始化网络传输
 */
int32_t bmc_transport_net_init(const char *ip_addr, uint16_t port, uint32_t timeout_ms, void **handle)
{
    bmc_transport_net_context_t *ctx;
    osal_timeval_t tv;
    osal_sockaddr_in_t server_addr;

    if (NULL == ip_addr || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_net_context_t *)OSAL_Malloc(sizeof(bmc_transport_net_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_transport_net_context_t));
    ctx->timeout_ms = timeout_ms;

    /* 创建TCP Socket */
    ctx->sockfd = OSAL_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, 0);
    if (ctx->sockfd < 0)
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 设置超时 */
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    OSAL_setsockopt(ctx->sockfd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO, &tv, sizeof(tv));
    OSAL_setsockopt(ctx->sockfd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO, &tv, sizeof(tv));

    /* 连接到远程地址 */
    OSAL_Memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = OSAL_AF_INET;
    server_addr.sin_port = OSAL_htons(port);
    if (OSAL_inet_pton(OSAL_AF_INET, ip_addr, &server_addr.sin_addr) <= 0)
    {
        OSAL_close(ctx->sockfd);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    if (OSAL_connect(ctx->sockfd, (osal_sockaddr_t *)&server_addr, sizeof(server_addr)) < 0)
    {
        OSAL_close(ctx->sockfd);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化网络传输
 */
int32_t bmc_transport_net_deinit(void *handle)
{
    bmc_transport_net_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_net_context_t *)handle;

    OSAL_close(ctx->sockfd);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 网络发送并接收
 */
int32_t bmc_transport_net_send_recv(void *handle,
                                    const uint8_t *request,
                                    uint32_t req_size,
                                    uint8_t *response,
                                    uint32_t resp_size,
                                    uint32_t *actual_size)
{
    bmc_transport_net_context_t *ctx;
    osal_ssize_t sent;
    osal_ssize_t recv_len;

    if (NULL == handle || NULL == request)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_net_context_t *)handle;

    /* 发送请求 */
    sent = OSAL_send(ctx->sockfd, request, req_size, 0);
    if (sent != (osal_ssize_t)req_size)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 接收响应 */
    if (NULL != response && resp_size > 0)
    {
        recv_len = OSAL_recv(ctx->sockfd, response, resp_size, 0);
        if (recv_len < 0)
        {
            return OSAL_ERR_GENERIC;
        }

        if (NULL != actual_size)
        {
            *actual_size = (uint32_t)recv_len;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 初始化串口传输
 */
int32_t bmc_transport_serial_init(const char *device, uint32_t baudrate, uint32_t timeout_ms, void **handle)
{
    bmc_transport_serial_context_t *ctx;
    hal_serial_config_t serial_config;

    if (NULL == device || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_serial_context_t *)OSAL_Malloc(sizeof(bmc_transport_serial_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_transport_serial_context_t));
    ctx->timeout_ms = timeout_ms;

    /* 打开串口 */
    serial_config.baud_rate = baudrate;
    serial_config.data_bits = 8;
    serial_config.stop_bits = 1;
    serial_config.parity = HAL_SERIAL_PARITY_NONE;

    if (OSAL_SUCCESS != HAL_Serial_Open(device, &serial_config, &ctx->serial_handle))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化串口传输
 */
int32_t bmc_transport_serial_deinit(void *handle)
{
    bmc_transport_serial_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_serial_context_t *)handle;

    HAL_Serial_Close(ctx->serial_handle);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 串口发送并接收
 */
int32_t bmc_transport_serial_send_recv(void *handle,
                                       const uint8_t *request,
                                       uint32_t req_size,
                                       uint8_t *response,
                                       uint32_t resp_size,
                                       uint32_t *actual_size)
{
    bmc_transport_serial_context_t *ctx;
    int32_t recv_len;

    if (NULL == handle || NULL == request)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_transport_serial_context_t *)handle;

    /* 发送请求 */
    if (HAL_Serial_Write(ctx->serial_handle, request, req_size, ctx->timeout_ms) != (int32_t)req_size)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 接收响应 */
    if (NULL != response && resp_size > 0)
    {
        recv_len = HAL_Serial_Read(ctx->serial_handle, response, resp_size, ctx->timeout_ms);
        if (recv_len < 0)
        {
            return OSAL_ERR_GENERIC;
        }

        if (NULL != actual_size)
        {
            *actual_size = recv_len;
        }
    }

    return OSAL_SUCCESS;
}
