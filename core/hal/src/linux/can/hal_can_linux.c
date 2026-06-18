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
#include "osal.h"
#include "hal.h"
#include "hal_can_internal.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

int32_t hal_can_init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
	hal_can_context_t *impl;
	struct sockaddr_can addr;
	struct ifreq ifr;
	int32_t ret;

	if (NULL == config || NULL == handle)
		return OSAL_ERR_INVALID_POINTER;

	if (NULL == config->interface || 0 == osal_strlen(config->interface))
		return OSAL_ERR_GENERIC;

	if (osal_strlen(config->interface) >= IFNAMSIZ)
		return OSAL_ERR_NAME_TOO_LONG;

	impl = (hal_can_context_t *)osal_malloc(OSAL_sizeof(hal_can_context_t));
	if (NULL == impl) {
		LOG_ERROR("HAL_CAN", "Failed to allocate memory");
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(impl, 0, OSAL_sizeof(hal_can_context_t));
	osal_strncpy(impl->interface, config->interface, IFNAMSIZ - 1);
	impl->interface[IFNAMSIZ - 1] = '\0';
	impl->baudrate = config->baudrate;
	impl->initialized = false;

	/* 创建文件锁（进程间保护） */
	char lock_file[OSAL_LOCK_PATH_MAX_LEN];
	osal_snprintf(lock_file, OSAL_sizeof(lock_file), HAL_CAN_LOCK_PATH_FMT,
				  config->interface);
	ret = osal_flock_create(lock_file, &impl->flock);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to create file lock: %s", lock_file);
		osal_free(impl);
		return ret;
	}

	/* 创建互斥锁（线程间保护） */
	ret = osal_mutex_init(&impl->mutex, NULL);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to create mutex");
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return ret;
	}

	impl->sockfd = osal_socket(OSAL_PF_CAN, OSAL_SOCK_RAW, OSAL_CAN_RAW);
	if (impl->sockfd < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_CAN", "Failed to create socket: %s (%d)",
				  osal_strerror(err), err);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	osal_memset(&ifr, 0, OSAL_sizeof(ifr));
	osal_strncpy(ifr.ifr_name, config->interface, IFNAMSIZ - 1);
	ret = osal_ioctl(impl->sockfd, SIOCGIFINDEX, &ifr);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_CAN", "Interface %s not found: %s (%d)",
				  config->interface, osal_strerror(err), err);
		osal_close(impl->sockfd);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	osal_memset(&addr, 0, OSAL_sizeof(addr));
	addr.can_family = OSAL_AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	ret = osal_bind(impl->sockfd, (const osal_sockaddr_t *)&addr,
					OSAL_sizeof(addr));
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_CAN", "Failed to bind interface: %s (%d)",
				  osal_strerror(err), err);
		osal_close(impl->sockfd);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	if (config->rx_timeout > 0) {
		struct timeval tv;
		tv.tv_sec = config->rx_timeout / 1000;
		tv.tv_usec = (config->rx_timeout % 1000) * 1000;
		osal_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_RCVTIMEO, &tv,
						OSAL_sizeof(tv));
	}

	if (config->tx_timeout > 0) {
		struct timeval tv;
		tv.tv_sec = config->tx_timeout / 1000;
		tv.tv_usec = (config->tx_timeout % 1000) * 1000;
		osal_setsockopt(impl->sockfd, OSAL_SOL_SOCKET, OSAL_SO_SNDTIMEO, &tv,
						OSAL_sizeof(tv));
	}

	impl->initialized = true;
	*handle = (hal_can_handle_t)impl;

	LOG_INFO(
		"HAL_CAN",
		"Initialized successfully: %s @ %u bps (with dual-lock protection)",
		config->interface, config->baudrate);
	return OSAL_SUCCESS;
}

int32_t hal_can_deinit(hal_can_handle_t handle)
{
	hal_can_context_t *impl = (hal_can_context_t *)handle;

	if (NULL == impl)
		return OSAL_ERR_INVALID_ID;

	if (impl->initialized && impl->sockfd >= 0) {
		osal_close(impl->sockfd);
		impl->sockfd = -1;
	}

	/* 销毁锁 */
	osal_mutex_destroy(&impl->mutex);

	if (impl->flock) {
		osal_flock_destroy(impl->flock);
	}

	impl->initialized = false;
	osal_free(impl);

	LOG_INFO("HAL_CAN", "Deinitialized successfully");
	return OSAL_SUCCESS;
}

int32_t hal_can_send(hal_can_handle_t handle, const hal_can_frame_t *frame)
{
	hal_can_context_t *impl = (hal_can_context_t *)handle;
	struct can_frame can_frame;
	osal_ssize_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == frame)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || impl->sockfd < 0)
		return OSAL_ERR_INVALID_ID;

	if (frame->dlc > 8) {
		LOG_ERROR("HAL_CAN", "Invalid DLC: %u", frame->dlc);
		return OSAL_ERR_GENERIC;
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_CAN_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：硬件访问 */
	osal_memset(&can_frame, 0, OSAL_sizeof(can_frame));
	can_frame.can_id = frame->can_id;
	can_frame.can_dlc = frame->dlc;
	osal_memcpy(can_frame.data, frame->data, frame->dlc);

	ret = osal_write(impl->sockfd, &can_frame, OSAL_sizeof(struct can_frame));
	if (ret != OSAL_sizeof(struct can_frame)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_CAN", "Send failed: %s (%d)", osal_strerror(err), err);
		result = err;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}

int32_t hal_can_recv(hal_can_handle_t handle, hal_can_frame_t *frame,
					 int32_t timeout)
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
	if (timeout >= 0) {
		osal_pollfd_t pfd = {
			.fd = impl->sockfd,
			.events = OSAL_POLLIN,
		};

		int32_t poll_ret = osal_poll(&pfd, 1, timeout);
		if (poll_ret == 0)
			return OSAL_ERR_TIMEOUT;
		else if (poll_ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_CAN", "Poll failed: %s (%d)", osal_strerror(err),
					  err);
			return err;
		}
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_CAN_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：读取数据 */
	ret = osal_read(impl->sockfd, &can_frame, OSAL_sizeof(struct can_frame));
	if (ret < 0) {
		int32_t err = osal_get_errno();
		if (err == OSAL_EAGAIN || err == OSAL_EWOULDBLOCK) {
			result = OSAL_ERR_TIMEOUT;
		} else {
			LOG_ERROR("HAL_CAN", "Receive failed: %s (%d)", osal_strerror(err),
					  err);
			result = err;
		}
	} else if (ret != OSAL_sizeof(struct can_frame)) {
		LOG_ERROR("HAL_CAN", "Incomplete receive: %d/%u bytes", (int32_t)ret,
				  (uint32_t)OSAL_sizeof(struct can_frame));
		result = OSAL_ERR_GENERIC;
	} else {
		osal_memset(frame, 0, OSAL_sizeof(hal_can_frame_t));
		frame->can_id = can_frame.can_id;
		frame->dlc = (can_frame.can_dlc > 8) ? 8 : can_frame.can_dlc;
		osal_memcpy(frame->data, can_frame.data, frame->dlc);
		frame->timestamp = osal_get_tick_count();
		result = OSAL_SUCCESS;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}

int32_t hal_can_set_filter(hal_can_handle_t handle, uint32_t filter_id,
						   uint32_t filter_mask)
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
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_CAN_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_CAN", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：设置过滤器 */
	rfilter.can_id = filter_id;
	rfilter.can_mask = filter_mask;

	if (osal_setsockopt(impl->sockfd, OSAL_SOL_CAN_RAW, CAN_RAW_FILTER,
						&rfilter, OSAL_sizeof(rfilter)) < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_CAN", "Failed to set filter: %s (%d)",
				  osal_strerror(err), err);
		result = err;
	} else {
		LOG_INFO("HAL_CAN", "Filter set: ID=0x%X, Mask=0x%X", filter_id,
				 filter_mask);
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}
