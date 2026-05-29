/************************************************************************
 * CCM 以太网通信模块实现
 *
 * 职责：
 * - 封装 OSAL_Socket 接口
 * - 管理 TCP 连接状态
 * - 提供消息收发接口
 ************************************************************************/

#include "pdl_ccm_internal.h"
#include "net/osal_socket.h"
#include "sys/osal_time.h"
#include "util/osal_log.h"
#include "lib/osal_errno.h"
#include "osal.h"
#include <string.h>

/*
 * 本地 timeval 结构体定义（用于 socket 超时设置）
 */
struct timeval {
    int32_t tv_sec;   /* 秒 */
    int32_t tv_usec;  /* 微秒 */
};

/*
 * 以太网通信上下文
 */
typedef struct
{
    int32_t sockfd;              /* Socket 文件描述符 */
    char ccm_ip[16];             /* CCM IP 地址 */
    uint16_t ccm_port;           /* CCM 端口号 */
    uint32_t timeout_ms;         /* 超时时间（毫秒） */
    bool connected;              /* 连接状态 */
    uint32_t seq_num;            /* 消息序列号 */
    osal_mutex_t *mutex;         /* 互斥锁 */
} ccm_eth_context_t;

/*
 * 初始化以太网通信
 */
int32_t ccm_eth_init(const pdl_ccm_config_t *config, void **handle)
{
    ccm_eth_context_t *ctx;
    osal_sockaddr_in_t server_addr;
    struct timeval tv;
    int32_t ret;

    if (!config || !handle)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 分配上下文 */
    ctx = (ccm_eth_context_t *)OSAL_Malloc(sizeof(ccm_eth_context_t));
    if (!ctx)
    {
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_Memset(ctx, 0, sizeof(ccm_eth_context_t));
    strncpy(ctx->ccm_ip, config->ccm_ip, sizeof(ctx->ccm_ip) - 1);
    ctx->ccm_port = config->ccm_port;
    ctx->timeout_ms = config->send_timeout_ms;
    ctx->seq_num = 0;
    ctx->sockfd = -1;
    ctx->connected = false;

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&ctx->mutex);
    if (ret != OSAL_SUCCESS)
    {
        OSAL_Free(ctx);
        return ret;
    }

    /* 创建 TCP socket */
    ctx->sockfd = OSAL_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
    if (ctx->sockfd < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to create socket");
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 设置 socket 选项 */
    int32_t reuse = 1;
    OSAL_setsockopt(ctx->sockfd, OSAL_SOL_SOCKET, OSAL_SO_REUSEADDR,
                    &reuse, sizeof(reuse));

    /* 设置接收超时 */
    tv.tv_sec = config->recv_timeout_ms / 1000;
    tv.tv_usec = (config->recv_timeout_ms % 1000) * 1000;
    OSAL_setsockopt(ctx->sockfd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO,
                    &tv, sizeof(tv));

    /* 设置发送超时 */
    tv.tv_sec = config->send_timeout_ms / 1000;
    tv.tv_usec = (config->send_timeout_ms % 1000) * 1000;
    OSAL_setsockopt(ctx->sockfd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO,
                    &tv, sizeof(tv));

    /* 禁用 Nagle 算法（降低延迟） */
    int32_t nodelay = 1;
    OSAL_setsockopt(ctx->sockfd, OSAL_IPPROTO_TCP, OSAL_TCP_NODELAY,
                    &nodelay, sizeof(nodelay));

    /* 构造服务器地址 */
    OSAL_Memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = OSAL_AF_INET;
    server_addr.sin_port = OSAL_htons(config->ccm_port);

    /* 转换 IP 地址 */
    ret = OSAL_inet_pton(OSAL_AF_INET, config->ccm_ip, &server_addr.sin_addr);
    if (ret != 1)
    {
        LOG_ERROR("PDL_CCM", "Invalid IP address: %s", config->ccm_ip);
        OSAL_shutdown(ctx->sockfd, OSAL_SHUT_RDWR);
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 连接到服务器 */
    ret = OSAL_connect(ctx->sockfd, (osal_sockaddr_t *)&server_addr, sizeof(server_addr));
    if (ret == 0)
    {
        ctx->connected = true;
        LOG_INFO("PDL_CCM", "Connected to %s:%u", config->ccm_ip, config->ccm_port);
    }
    else
    {
        LOG_WARN("PDL_CCM", "Failed to connect to %s:%u, will retry later",
                 config->ccm_ip, config->ccm_port);
        ctx->connected = false;
    }

    *handle = ctx;
    return OSAL_SUCCESS;
}

/*
 * 反初始化以太网通信
 */
int32_t ccm_eth_deinit(void *handle)
{
    ccm_eth_context_t *ctx = (ccm_eth_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 关闭 socket */
    if (ctx->sockfd >= 0)
    {
        OSAL_shutdown(ctx->sockfd, OSAL_SHUT_RDWR);
        /* 注意：OSAL_Socket 没有提供 close 接口，需要添加 */
        /* 临时使用 shutdown，实际应该调用 close(ctx->sockfd) */
    }

    /* 销毁互斥锁 */
    OSAL_MutexDelete(ctx->mutex);

    /* 释放上下文 */
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/*
 * 发送以太网消息
 */
int32_t ccm_eth_send(void *handle, const ccm_eth_msg_t *msg, uint32_t timeout_ms)
{
    ccm_eth_context_t *ctx = (ccm_eth_context_t *)handle;
    uint8_t send_buf[CCM_ETH_MAX_MSG_SIZE + 16];  /* 预留协议头空间 */
    size_t send_len;
    int32_t ret;

    if (!ctx || !msg)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 检查连接状态 */
    if (!ctx->connected || ctx->sockfd < 0)
    {
        LOG_ERROR("PDL_CCM", "Not connected");
        return OSAL_ERR_GENERIC;
    }

    /* 构造以太网帧 */
    OSAL_MutexLock(ctx->mutex);
    uint32_t seq = ctx->seq_num++;
    OSAL_MutexUnlock(ctx->mutex);

    /* 帧格式：
     * [0-1]   魔术字 0xAA55
     * [2]     消息类型
     * [3]     保留
     * [4-7]   序列号
     * [8-11]  负载长度
     * [12-15] CRC32
     * [16...] 负载数据
     */
    send_buf[0] = 0xAA;
    send_buf[1] = 0x55;
    send_buf[2] = msg->msg_type;
    send_buf[3] = 0x00;
    send_buf[4] = (seq >> 24) & 0xFF;
    send_buf[5] = (seq >> 16) & 0xFF;
    send_buf[6] = (seq >> 8) & 0xFF;
    send_buf[7] = seq & 0xFF;
    send_buf[8] = (msg->payload_len >> 24) & 0xFF;
    send_buf[9] = (msg->payload_len >> 16) & 0xFF;
    send_buf[10] = (msg->payload_len >> 8) & 0xFF;
    send_buf[11] = msg->payload_len & 0xFF;

    /* 拷贝负载 */
    if (msg->payload_len > 0)
    {
        OSAL_Memcpy(&send_buf[16], msg->payload, msg->payload_len);
    }

    /* 计算 CRC32（简化版，实际应使用标准 CRC32） */
    uint32_t crc = 0;
    for (size_t i = 0; i < msg->payload_len; i++)
    {
        crc += msg->payload[i];
    }
    send_buf[12] = (crc >> 24) & 0xFF;
    send_buf[13] = (crc >> 16) & 0xFF;
    send_buf[14] = (crc >> 8) & 0xFF;
    send_buf[15] = crc & 0xFF;

    send_len = 16 + msg->payload_len;

    /* 发送数据 */
    ret = OSAL_send(ctx->sockfd, send_buf, send_len, 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to send message");
        ctx->connected = false;
        return OSAL_ERR_GENERIC;
    }

    if ((size_t)ret != send_len)
    {
        LOG_ERROR("PDL_CCM", "Partial send (%d/%zu bytes)", ret, send_len);
        ctx->connected = false;
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*
 * 接收以太网消息
 */
int32_t ccm_eth_recv(void *handle, ccm_eth_msg_t *msg, uint32_t timeout_ms)
{
    ccm_eth_context_t *ctx = (ccm_eth_context_t *)handle;
    uint8_t recv_buf[CCM_ETH_MAX_MSG_SIZE + 16];
    int32_t ret;

    if (!ctx || !msg)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 检查连接状态 */
    if (!ctx->connected || ctx->sockfd < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 接收数据（至少接收帧头 16 字节） */
    ret = OSAL_recv(ctx->sockfd, recv_buf, 16, OSAL_MSG_WAITALL);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to receive header");
        ctx->connected = false;
        return OSAL_ERR_GENERIC;
    }

    if (ret == 0)
    {
        LOG_INFO("PDL_CCM", "Connection closed by peer");
        ctx->connected = false;
        return OSAL_ERR_GENERIC;
    }

    if (ret < 16)
    {
        LOG_ERROR("PDL_CCM", "Received header too short (%d bytes)", ret);
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 检查魔术字 */
    if (recv_buf[0] != 0xAA || recv_buf[1] != 0x55)
    {
        LOG_ERROR("PDL_CCM", "Invalid magic number");
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解析帧头 */
    msg->msg_type = recv_buf[2];
    msg->seq_num = ((uint32_t)recv_buf[4] << 24) |
                   ((uint32_t)recv_buf[5] << 16) |
                   ((uint32_t)recv_buf[6] << 8) |
                   ((uint32_t)recv_buf[7]);
    msg->payload_len = ((uint32_t)recv_buf[8] << 24) |
                       ((uint32_t)recv_buf[9] << 16) |
                       ((uint32_t)recv_buf[10] << 8) |
                       ((uint32_t)recv_buf[11]);

    /* 检查负载长度 */
    if (msg->payload_len > CCM_ETH_MAX_MSG_SIZE)
    {
        LOG_ERROR("PDL_CCM", "Payload too large (%u bytes)", msg->payload_len);
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 接收负载数据 */
    if (msg->payload_len > 0)
    {
        ret = OSAL_recv(ctx->sockfd, msg->payload, msg->payload_len, OSAL_MSG_WAITALL);
        if (ret < 0)
        {
            LOG_ERROR("PDL_CCM", "Failed to receive payload");
            ctx->connected = false;
            return OSAL_ERR_GENERIC;
        }

        if ((uint32_t)ret != msg->payload_len)
        {
            LOG_ERROR("PDL_CCM", "Incomplete payload (%d/%u bytes)", ret, msg->payload_len);
            return OSAL_ERR_INVALID_PARAM;
        }
    }

    /* 验证 CRC32（简化版） */
    uint32_t crc_recv = ((uint32_t)recv_buf[12] << 24) |
                        ((uint32_t)recv_buf[13] << 16) |
                        ((uint32_t)recv_buf[14] << 8) |
                        ((uint32_t)recv_buf[15]);
    uint32_t crc_calc = 0;
    for (size_t i = 0; i < msg->payload_len; i++)
    {
        crc_calc += msg->payload[i];
    }
    if (crc_recv != crc_calc)
    {
        LOG_ERROR("PDL_CCM", "CRC mismatch (recv=0x%08X, calc=0x%08X)", crc_recv, crc_calc);
        return OSAL_ERR_INVALID_PARAM;
    }

    return OSAL_SUCCESS;
}

/*
 * 检查连接状态
 */
int32_t ccm_eth_is_connected(void *handle)
{
    ccm_eth_context_t *ctx = (ccm_eth_context_t *)handle;

    if (!ctx)
    {
        return 0;
    }

    return (ctx->connected && (ctx->sockfd >= 0)) ? 1 : 0;
}
