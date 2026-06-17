/************************************************************************
 * 卫星平台 Ethernet 通信实现
 *
 * 职责：
 * - 封装卫星以太网协议（帧格式、消息类型）
 * - TCP/UDP 消息的收发
 * - 协议解析和封装
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pdl.h"
#include "pdl_satellite_internal.h"

/*
 * Ethernet 通信上下文
 */
typedef struct
{
	int32_t socket_fd;           /* Socket 文件描述符 */
	char ip_address[64];         /* IP 地址 */
	uint16_t port;               /* 端口号 */
	uint32_t send_timeout_ms;    /* 发送超时 */
	uint32_t recv_timeout_ms;    /* 接收超时 */
	bool connected;              /* 连接状态 */
	osal_mutex_t mutex;          /* 互斥锁 */
} satellite_ethernet_context_t;

/**
 * @brief 初始化卫星 Ethernet 通信（适配 ops 接口）
 */
static int32_t satellite_ethernet_init_ops(const void *config, void **handle)
{
	const pconfig_satellite_ethernet_config_t *eth_cfg = (const pconfig_satellite_ethernet_config_t *)config;
	satellite_ethernet_context_t *ctx;
	osal_sockaddr_in_t server_addr;
	int32_t ret;
	osal_timeval_t tv;

	if (NULL == config || NULL == handle)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (satellite_ethernet_context_t *)OSAL_malloc(OSAL_sizeof(satellite_ethernet_context_t));
	if (NULL == ctx)
	{
		return OSAL_ERR_NO_MEMORY;
	}

	OSAL_memset(ctx, 0, OSAL_sizeof(satellite_ethernet_context_t));
	OSAL_strncpy(ctx->ip_address, eth_cfg->ip_address, OSAL_sizeof(ctx->ip_address) - 1);
	ctx->port = eth_cfg->port;
	ctx->send_timeout_ms = eth_cfg->send_timeout_ms;
	ctx->recv_timeout_ms = eth_cfg->recv_timeout_ms;

	/* 创建互斥锁 */
	if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
	{
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 创建 TCP Socket */
	ctx->socket_fd = OSAL_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, 0);
	if (ctx->socket_fd < 0)
	{
		OSAL_pthread_mutex_destroy(&ctx->mutex);
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 设置发送超时 */
	tv.tv_sec = eth_cfg->send_timeout_ms / 1000;
	tv.tv_usec = (eth_cfg->send_timeout_ms % 1000) * 1000;
	OSAL_setsockopt(ctx->socket_fd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO, &tv, OSAL_sizeof(tv));

	/* 设置接收超时 */
	tv.tv_sec = eth_cfg->recv_timeout_ms / 1000;
	tv.tv_usec = (eth_cfg->recv_timeout_ms % 1000) * 1000;
	OSAL_setsockopt(ctx->socket_fd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO, &tv, OSAL_sizeof(tv));

	/* 连接到服务器 */
	OSAL_memset(&server_addr, 0, OSAL_sizeof(server_addr));
	server_addr.sin_family = OSAL_AF_INET;
	server_addr.sin_port = OSAL_htons(eth_cfg->port);
	OSAL_inet_pton(OSAL_AF_INET, eth_cfg->ip_address, &server_addr.sin_addr);

	ret = OSAL_connect(ctx->socket_fd, (osal_sockaddr_t *)&server_addr, OSAL_sizeof(server_addr));
	if (ret < 0)
	{
		LOG_ERROR("PDL_Satellite", "Failed to connect to %s:%u", eth_cfg->ip_address, eth_cfg->port);
		OSAL_close(ctx->socket_fd);
		OSAL_pthread_mutex_destroy(&ctx->mutex);
		OSAL_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	ctx->connected = true;
	*handle = ctx;

	LOG_INFO("PDL_Satellite", "Ethernet initialized: %s:%u", eth_cfg->ip_address, eth_cfg->port);
	return OSAL_SUCCESS;
}

/**
 * @brief 反初始化卫星 Ethernet 通信
 */
static int32_t satellite_ethernet_deinit(void *handle)
{
	satellite_ethernet_context_t *ctx;

	if (NULL == handle)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (satellite_ethernet_context_t *)handle;

	OSAL_pthread_mutex_lock(&ctx->mutex);
	if (ctx->socket_fd >= 0)
	{
		OSAL_close(ctx->socket_fd);
		ctx->socket_fd = -1;
	}
	ctx->connected = false;
	OSAL_pthread_mutex_unlock(&ctx->mutex);

	OSAL_pthread_mutex_destroy(&ctx->mutex);
	OSAL_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 接收卫星 Ethernet 消息
 */
static int32_t satellite_ethernet_recv(void *handle, satellite_can_msg_t *msg, uint32_t timeout_ms)
{
	satellite_ethernet_context_t *ctx;
	uint8_t buffer[16];
	osal_ssize_t ret;

	if (NULL == handle || NULL == msg)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (satellite_ethernet_context_t *)handle;

	OSAL_pthread_mutex_lock(&ctx->mutex);

	if (!ctx->connected)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return OSAL_ERR_GENERIC;
	}

	/* 接收消息（格式与 CAN 相同：[msg_type][seq_num][cmd_type][reserved][data(4bytes)]）*/
	ret = OSAL_recv(ctx->socket_fd, buffer, 8, 0);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != 8)
	{
		if (ret == 0)
		{
			LOG_WARN("PDL_Satellite", "Connection closed by peer");
			ctx->connected = false;
		}
		return (ret < 0) ? OSAL_ERR_TIMEOUT : OSAL_ERR_GENERIC;
	}

	/* 解析消息 */
	msg->msg_type = buffer[0];
	msg->seq_num = buffer[1];
	msg->cmd_type = buffer[2];
	/* buffer[3] reserved */

	/* 网络字节序转主机字节序 */
	uint32_t data_be = (buffer[4] << 24) | (buffer[5] << 16) |
	                   (buffer[6] << 8) | buffer[7];
	msg->data = OSAL_ntohl(data_be);

	return OSAL_SUCCESS;
}

/**
 * @brief 发送卫星 Ethernet 消息
 */
static int32_t satellite_ethernet_send(void *handle, const satellite_can_msg_t *msg)
{
	satellite_ethernet_context_t *ctx;
	uint8_t buffer[8];
	osal_ssize_t ret;
	uint32_t data_be;

	if (NULL == handle || NULL == msg)
	{
		return OSAL_ERR_GENERIC;
	}

	ctx = (satellite_ethernet_context_t *)handle;

	OSAL_pthread_mutex_lock(&ctx->mutex);

	if (!ctx->connected)
	{
		OSAL_pthread_mutex_unlock(&ctx->mutex);
		return OSAL_ERR_GENERIC;
	}

	/* 封装消息 */
	buffer[0] = msg->msg_type;
	buffer[1] = msg->seq_num;
	buffer[2] = msg->cmd_type;
	buffer[3] = 0;  /* reserved */

	/* 主机字节序转网络字节序 */
	data_be = OSAL_htonl(msg->data);
	buffer[4] = (uint8_t)(data_be >> 24);
	buffer[5] = (uint8_t)(data_be >> 16);
	buffer[6] = (uint8_t)(data_be >> 8);
	buffer[7] = (uint8_t)(data_be & 0xFF);

	/* 发送 */
	ret = OSAL_send(ctx->socket_fd, buffer, OSAL_sizeof(buffer), 0);

	OSAL_pthread_mutex_unlock(&ctx->mutex);

	if (ret != 8)
	{
		if (ret == 0)
		{
			ctx->connected = false;
		}
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 发送心跳消息
 */
static int32_t satellite_ethernet_send_heartbeat(void *handle, uint8_t status)
{
	satellite_can_msg_t msg = {
		.msg_type = CAN_MSG_TYPE_HEARTBEAT,
		.seq_num = 0,
		.cmd_type = status,
		.data = 0
	};

	return satellite_ethernet_send(handle, &msg);
}

/**
 * @brief 发送响应消息
 */
static int32_t satellite_ethernet_send_response(void *handle, uint8_t seq_num, uint8_t status, uint32_t result)
{
	satellite_can_msg_t msg = {
		.msg_type = CAN_MSG_TYPE_CMD_RESP,
		.seq_num = seq_num,
		.cmd_type = status,
		.data = result
	};

	return satellite_ethernet_send(handle, &msg);
}

/*
 * Ethernet 接口的 ops 结构定义（导出供 pdl_satellite.c 使用）
 */
const pdl_satellite_ops_t satellite_ethernet_ops = {
	.init = satellite_ethernet_init_ops,
	.deinit = satellite_ethernet_deinit,
	.recv = satellite_ethernet_recv,
	.send = satellite_ethernet_send,
	.send_heartbeat = satellite_ethernet_send_heartbeat,
	.send_response = satellite_ethernet_send_response,
};
