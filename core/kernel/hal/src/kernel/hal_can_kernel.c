// SPDX-License-Identifier: GPL-2.0

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/if.h>
#include <linux/jiffies.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/sockptr.h>
#include <linux/uio.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "osal.h"
#include "hal_can.h"

typedef struct {
	struct socket *sock;
	osal_mutex_t tx_lock;
	osal_mutex_t rx_lock;
	char interface[IFNAMSIZ];
	uint32_t baudrate;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
	bool initialized;
} hal_can_kernel_context_t;

static int32_t hal_can_errno_to_status(int ret)
{
	int err;

	if (ret >= 0)
		return OSAL_SUCCESS;

	err = -ret;
	if (err == EAGAIN || err == EWOULDBLOCK || err == ETIMEDOUT)
		return OSAL_ERR_TIMEOUT;

	return err;
}

static long hal_can_timeout_to_jiffies(uint32_t timeout_ms)
{
	if (timeout_ms == OS_PEND)
		return MAX_SCHEDULE_TIMEOUT;

	return msecs_to_jiffies(timeout_ms);
}

static int32_t hal_can_set_socket_timeouts(hal_can_kernel_context_t *ctx)
{
	struct sock *sk;

	if (!ctx || !ctx->sock || !ctx->sock->sk)
		return OSAL_ERR_INVALID_PARAM;

	sk = ctx->sock->sk;
	WRITE_ONCE(sk->sk_rcvtimeo, hal_can_timeout_to_jiffies(ctx->rx_timeout));
	WRITE_ONCE(sk->sk_sndtimeo, hal_can_timeout_to_jiffies(ctx->tx_timeout));

	return OSAL_SUCCESS;
}

static int32_t hal_can_bind_interface(hal_can_kernel_context_t *ctx)
{
	struct net_device *dev;
	struct sockaddr_can addr = { 0 };
	int ifindex;
	int ret;

	dev = dev_get_by_name(&init_net, ctx->interface);
	if (!dev) {
		LOG_ERROR("HAL_CAN", "interface %s not found", ctx->interface);
		return OSAL_ENOENT;
	}

	ifindex = dev->ifindex;
	dev_put(dev);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifindex;

	ret = kernel_bind(ctx->sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERROR("HAL_CAN", "bind %s failed: %d", ctx->interface, ret);
		return hal_can_errno_to_status(ret);
	}

	return OSAL_SUCCESS;
}

int32_t hal_can_init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
	hal_can_kernel_context_t *ctx;
	int ret;

	if (!config || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	if (!config->interface || config->interface[0] == '\0')
		return OSAL_ERR_INVALID_PARAM;

	if (strnlen(config->interface, IFNAMSIZ) >= IFNAMSIZ)
		return OSAL_ERR_INVALID_PARAM;

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ret = osal_mutex_init(&ctx->tx_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_free;

	ret = osal_mutex_init(&ctx->rx_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_tx_mutex;

	strscpy(ctx->interface, config->interface, sizeof(ctx->interface));
	ctx->baudrate = config->baudrate;
	ctx->rx_timeout = config->rx_timeout;
	ctx->tx_timeout = config->tx_timeout;

	ret = sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW, &ctx->sock);
	if (ret < 0) {
		LOG_ERROR("HAL_CAN", "create raw CAN socket failed: %d", ret);
		ret = hal_can_errno_to_status(ret);
		goto err_mutex;
	}

	ret = hal_can_bind_interface(ctx);
	if (ret != OSAL_SUCCESS)
		goto err_socket;

	ret = hal_can_set_socket_timeouts(ctx);
	if (ret != OSAL_SUCCESS)
		goto err_socket;

	ctx->initialized = true;
	*handle = ctx;

	LOG_INFO("HAL_CAN", "initialized %s @ %u bps", ctx->interface,
		 ctx->baudrate);
	return OSAL_SUCCESS;

err_socket:
	sock_release(ctx->sock);
	ctx->sock = NULL;
err_mutex:
	osal_mutex_destroy(&ctx->rx_lock);
err_tx_mutex:
	osal_mutex_destroy(&ctx->tx_lock);
err_free:
	osal_free(ctx);
	return ret;
}

int32_t hal_can_deinit(hal_can_handle_t handle)
{
	hal_can_kernel_context_t *ctx = handle;
	struct socket *sock;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->tx_lock);
	osal_mutex_lock(&ctx->rx_lock);
	ctx->initialized = false;
	sock = ctx->sock;
	ctx->sock = NULL;
	osal_mutex_unlock(&ctx->rx_lock);
	osal_mutex_unlock(&ctx->tx_lock);

	if (sock)
		sock_release(sock);

	osal_mutex_destroy(&ctx->rx_lock);
	osal_mutex_destroy(&ctx->tx_lock);
	osal_free(ctx);

	return OSAL_SUCCESS;
}

int32_t hal_can_send(hal_can_handle_t handle, const hal_can_frame_t *frame)
{
	hal_can_kernel_context_t *ctx = handle;
	struct can_frame can_frame = { 0 };
	struct msghdr msg = { 0 };
	struct kvec iov;
	int ret;

	if (!ctx || !frame)
		return OSAL_ERR_INVALID_PARAM;

	if (frame->dlc > HAL_CAN_MAX_DATA_LEN)
		return OSAL_ERR_INVALID_SIZE;

	osal_mutex_lock(&ctx->tx_lock);
	if (!ctx->initialized || !ctx->sock) {
		osal_mutex_unlock(&ctx->tx_lock);
		return OSAL_ERR_INVALID_ID;
	}

	can_frame.can_id = frame->can_id;
	can_frame.can_dlc = frame->dlc;
	osal_memcpy(can_frame.data, frame->data, frame->dlc);

	iov.iov_base = &can_frame;
	iov.iov_len = sizeof(can_frame);

	ret = kernel_sendmsg(ctx->sock, &msg, &iov, 1, sizeof(can_frame));
	osal_mutex_unlock(&ctx->tx_lock);

	if (ret == sizeof(can_frame))
		return OSAL_SUCCESS;

	if (ret >= 0) {
		LOG_ERROR("HAL_CAN", "short send: %d/%zu", ret,
			  sizeof(can_frame));
		return OSAL_ERR_GENERIC;
	}

	LOG_ERROR("HAL_CAN", "send failed: %d", ret);
	return hal_can_errno_to_status(ret);
}

int32_t hal_can_recv(hal_can_handle_t handle, hal_can_frame_t *frame,
		     int32_t timeout)
{
	hal_can_kernel_context_t *ctx = handle;
	struct can_frame can_frame;
	struct msghdr msg = { 0 };
	struct kvec iov;
	struct sock *sk;
	long old_timeout;
	long recv_timeout;
	int flags = 0;
	int ret;

	if (!ctx || !frame)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->rx_lock);
	if (!ctx->initialized || !ctx->sock || !ctx->sock->sk) {
		osal_mutex_unlock(&ctx->rx_lock);
		return OSAL_ERR_INVALID_ID;
	}

	sk = ctx->sock->sk;
	old_timeout = READ_ONCE(sk->sk_rcvtimeo);
	if (timeout < 0) {
		recv_timeout = 0;
		flags = MSG_DONTWAIT;
	} else {
		recv_timeout = (timeout == OS_PEND) ?
				       MAX_SCHEDULE_TIMEOUT :
				       msecs_to_jiffies((uint32_t)timeout);
	}
	WRITE_ONCE(sk->sk_rcvtimeo, recv_timeout);

	iov.iov_base = &can_frame;
	iov.iov_len = sizeof(can_frame);

	ret = kernel_recvmsg(ctx->sock, &msg, &iov, 1, sizeof(can_frame), flags);
	WRITE_ONCE(sk->sk_rcvtimeo, old_timeout);
	osal_mutex_unlock(&ctx->rx_lock);

	if (ret == sizeof(can_frame)) {
		osal_memset(frame, 0, sizeof(*frame));
		frame->can_id = can_frame.can_id;
		frame->dlc = (can_frame.can_dlc > HAL_CAN_MAX_DATA_LEN) ?
				     HAL_CAN_MAX_DATA_LEN :
				     can_frame.can_dlc;
		osal_memcpy(frame->data, can_frame.data, frame->dlc);
		frame->timestamp = (uint32_t)(osal_get_monotonic_time() / 1000);
		return OSAL_SUCCESS;
	}

	if (ret >= 0) {
		LOG_ERROR("HAL_CAN", "short receive: %d/%zu", ret,
			  sizeof(can_frame));
		return OSAL_ERR_GENERIC;
	}

	return hal_can_errno_to_status(ret);
}

int32_t hal_can_set_filter(hal_can_handle_t handle, uint32_t filter_id,
			   uint32_t filter_mask)
{
	hal_can_kernel_context_t *ctx = handle;
	struct can_filter filter;
	int ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->rx_lock);
	if (!ctx->initialized || !ctx->sock || !ctx->sock->ops ||
	    !ctx->sock->ops->setsockopt) {
		osal_mutex_unlock(&ctx->rx_lock);
		return OSAL_ERR_INVALID_ID;
	}

	filter.can_id = filter_id;
	filter.can_mask = filter_mask;

	ret = ctx->sock->ops->setsockopt(ctx->sock, SOL_CAN_RAW,
					 CAN_RAW_FILTER,
					 KERNEL_SOCKPTR(&filter),
					 sizeof(filter));
	osal_mutex_unlock(&ctx->rx_lock);

	if (ret < 0) {
		LOG_ERROR("HAL_CAN", "set filter failed: %d", ret);
		return hal_can_errno_to_status(ret);
	}

	return OSAL_SUCCESS;
}
