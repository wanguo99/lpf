/************************************************************************
 * HAL层 - SPI驱动Linux实现
 ************************************************************************/

/* 必须在所有头文件之前定义，以启用完整的POSIX功能 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "osal.h"
#include "hal.h"
#include "hal_spi_internal.h"
#include "../hal_linux_lock.h"

/**
 * @brief 打开SPI设备
 */
int32_t hal_spi_open(const hal_spi_config_t *config, hal_spi_handle_t *handle)
{
	hal_spi_context_t *impl;
	int32_t ret;

	/* 参数检查 */
	if (NULL == config || NULL == handle)
		return OSAL_ERR_INVALID_POINTER;

	if (NULL == config->device || 0 == osal_strlen(config->device))
		return OSAL_ERR_GENERIC;

	/* 分配句柄 */
	impl = (hal_spi_context_t *)osal_malloc(OSAL_sizeof(hal_spi_context_t));
	if (NULL == impl) {
		LOG_ERROR("HAL_SPI", "Failed to allocate memory");
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(impl, 0, OSAL_sizeof(hal_spi_context_t));
	osal_strncpy(impl->device, config->device, OSAL_sizeof(impl->device) - 1);
	impl->device[OSAL_sizeof(impl->device) - 1] = '\0';
	impl->mode = config->mode;
	impl->bits_per_word = config->bits_per_word;
	impl->max_speed_hz = config->max_speed_hz;
	impl->timeout = config->timeout;
	impl->initialized = false;

	/* 创建文件锁（进程间保护） */
	char lock_file[OSAL_LOCK_PATH_MAX_LEN];
	/* 从设备路径提取设备名，例如 /dev/spidev0.0 -> spidev0.0 */
	const char *dev_name = config->device;
	const char *slash = config->device;
	while (*slash) {
		if (*slash == '/')
			dev_name = slash + 1;
		slash++;
	}

	osal_snprintf(lock_file, OSAL_sizeof(lock_file), HAL_SPI_LOCK_PATH_FMT,
				  dev_name);
	ret = osal_flock_create(lock_file, &impl->flock);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_SPI", "Failed to create file lock: %s", lock_file);
		osal_free(impl);
		return ret;
	}

	/* 创建互斥锁（线程间保护） */
	ret = osal_mutex_init(&impl->mutex, NULL);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_SPI", "Failed to create mutex");
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return ret;
	}

	/* 打开SPI设备 */
	impl->fd = osal_open(config->device, OSAL_O_RDWR, 0);
	if (impl->fd < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Failed to open device %s: %s (%d)",
				  config->device, osal_strerror(err), err);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	/* 设置SPI模式 (参考: Linux内核文档 spidev.txt) */
	ret = osal_ioctl(impl->fd, SPI_IOC_WR_MODE, &impl->mode);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Failed to set SPI mode: %s (%d)",
				  osal_strerror(err), err);
		osal_close(impl->fd);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	/* 设置每字位数 */
	ret = osal_ioctl(impl->fd, SPI_IOC_WR_BITS_PER_WORD, &impl->bits_per_word);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Failed to set bits per word: %s (%d)",
				  osal_strerror(err), err);
		osal_close(impl->fd);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	/* 设置最大速率 */
	ret = osal_ioctl(impl->fd, SPI_IOC_WR_MAX_SPEED_HZ, &impl->max_speed_hz);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Failed to set max speed: %s (%d)",
				  osal_strerror(err), err);
		osal_close(impl->fd);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	impl->initialized = true;
	*handle = (hal_spi_handle_t)impl;

	LOG_INFO("HAL_SPI",
			 "Opened device %s (fd=%d, mode=%d, bits=%d, speed=%u Hz, with "
			 "process/thread protection)",
			 config->device, impl->fd, impl->mode, impl->bits_per_word,
			 impl->max_speed_hz);
	return OSAL_SUCCESS;
}

/**
 * @brief 关闭SPI设备
 */
int32_t hal_spi_close(hal_spi_handle_t handle)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;

	if (NULL == impl)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized)
		return OSAL_ERR_GENERIC;

	if (impl->fd >= 0) {
		osal_close(impl->fd);
		impl->fd = -1;
	}

	/* 销毁锁 */
	osal_mutex_destroy(&impl->mutex);

	if (impl->flock) {
		osal_flock_destroy(impl->flock);
	}

	impl->initialized = false;
	osal_free(impl);

	LOG_INFO("HAL_SPI", "Device closed");
	return OSAL_SUCCESS;
}

/**
 * @brief SPI写操作
 */
int32_t hal_spi_write(hal_spi_handle_t handle, const uint8_t *buffer,
					  uint32_t size)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	ret = _hal_linux_lock_device(impl->flock, &impl->mutex,
								 HAL_SPI_LOCK_TIMEOUT_MS, "HAL_SPI");
	if (ret != OSAL_SUCCESS)
		return ret;

	/* 临界区：写入数据 */
	ret = osal_write(impl->fd, buffer, size);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Write failed: %s (%d)", osal_strerror(err), err);
		result = err;
	} else if ((uint32_t)ret != size) {
		LOG_ERROR("HAL_SPI", "Partial write: %d/%u bytes", ret, size);
		result = OSAL_ERR_GENERIC;
	}

	_hal_linux_unlock_device(impl->flock, &impl->mutex);

	return result;
}

/**
 * @brief SPI读操作
 */
int32_t hal_spi_read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	ret = _hal_linux_lock_device(impl->flock, &impl->mutex,
								 HAL_SPI_LOCK_TIMEOUT_MS, "HAL_SPI");
	if (ret != OSAL_SUCCESS)
		return ret;

	/* 临界区：读取数据 */
	ret = osal_read(impl->fd, buffer, size);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Read failed: %s (%d)", osal_strerror(err), err);
		result = err;
	} else if ((uint32_t)ret != size) {
		LOG_ERROR("HAL_SPI", "Partial read: %d/%u bytes", ret, size);
		result = OSAL_ERR_GENERIC;
	}

	_hal_linux_unlock_device(impl->flock, &impl->mutex);

	return result;
}

/**
 * @brief SPI全双工传输
 */
int32_t hal_spi_transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
						 uint8_t *rx_buffer, uint32_t size)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;
	struct spi_ioc_transfer xfer;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	ret = _hal_linux_lock_device(impl->flock, &impl->mutex,
								 HAL_SPI_LOCK_TIMEOUT_MS, "HAL_SPI");
	if (ret != OSAL_SUCCESS)
		return ret;

	/* 临界区：构造传输结构 (参考: Linux内核 spidev.h) */
	osal_memset(&xfer, 0, OSAL_sizeof(xfer));
	xfer.tx_buf = (uintptr_t)tx_buffer;
	xfer.rx_buf = (uintptr_t)rx_buffer;
	xfer.len = size;
	xfer.speed_hz = impl->max_speed_hz;
	xfer.bits_per_word = impl->bits_per_word;
	xfer.delay_usecs = 0;
	xfer.cs_change = 0;

	/* 执行传输 */
	ret = osal_ioctl(impl->fd, SPI_IOC_MESSAGE(1), &xfer);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Transfer failed: %s (%d)", osal_strerror(err),
				  err);
		result = err;
	} else if ((uint32_t)ret != size) {
		LOG_ERROR("HAL_SPI", "Partial transfer: %d/%u bytes", ret, size);
		result = OSAL_ERR_GENERIC;
	}

	_hal_linux_unlock_device(impl->flock, &impl->mutex);

	return result;
}

/**
 * @brief SPI批量传输
 */
int32_t hal_spi_transfer_multi(hal_spi_handle_t handle,
							   hal_spi_transfer_t *transfers, uint32_t num)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;
	struct spi_ioc_transfer *xfers;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;
	uint32_t i;

	if (NULL == impl || NULL == transfers)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || num == 0)
		return OSAL_ERR_GENERIC;

	/* 分配内核传输结构 */
	xfers = (struct spi_ioc_transfer *)osal_malloc(
		OSAL_sizeof(struct spi_ioc_transfer) * num);
	if (NULL == xfers) {
		LOG_ERROR("HAL_SPI", "Failed to allocate transfer buffer");
		return OSAL_ERR_NO_MEMORY;
	}

	/* 转换传输格式 */
	osal_memset(xfers, 0, OSAL_sizeof(struct spi_ioc_transfer) * num);
	for (i = 0; i < num; i++) {
		xfers[i].tx_buf = (uintptr_t)transfers[i].tx_buf;
		xfers[i].rx_buf = (uintptr_t)transfers[i].rx_buf;
		xfers[i].len = transfers[i].len;
		xfers[i].speed_hz = (transfers[i].speed_hz != 0) ?
								transfers[i].speed_hz :
								impl->max_speed_hz;
		xfers[i].bits_per_word = (transfers[i].bits_per_word != 0) ?
									 transfers[i].bits_per_word :
									 impl->bits_per_word;
		xfers[i].delay_usecs = transfers[i].delay_usecs;
		xfers[i].cs_change = transfers[i].cs_change;
	}

	ret = _hal_linux_lock_device(impl->flock, &impl->mutex,
								 HAL_SPI_LOCK_TIMEOUT_MS, "HAL_SPI");
	if (ret != OSAL_SUCCESS) {
		osal_free(xfers);
		return ret;
	}

	/* 临界区：执行批量传输 */
	ret = osal_ioctl(impl->fd, SPI_IOC_MESSAGE(num), xfers);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_SPI", "Multi-transfer failed: %s (%d)",
				  osal_strerror(err), err);
		result = err;
	}

	_hal_linux_unlock_device(impl->flock, &impl->mutex);

	osal_free(xfers);
	return result;
}

/**
 * @brief 设置SPI配置
 */
int32_t hal_spi_set_config(hal_spi_handle_t handle,
						   const hal_spi_config_t *config)
{
	hal_spi_context_t *impl = (hal_spi_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == config)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized)
		return OSAL_ERR_GENERIC;

	ret = _hal_linux_lock_device(impl->flock, &impl->mutex,
								 HAL_SPI_LOCK_TIMEOUT_MS, "HAL_SPI");
	if (ret != OSAL_SUCCESS)
		return ret;

	/* 临界区：更新配置 */
	/* 更新模式 */
	if (config->mode != impl->mode) {
		uint8_t mode = config->mode;
		ret = osal_ioctl(impl->fd, SPI_IOC_WR_MODE, &mode);
		if (ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_SPI", "Failed to update mode: %s (%d)",
					  osal_strerror(err), err);
			result = err;
			goto unlock;
		}
		impl->mode = config->mode;
	}

	/* 更新每字位数 */
	if (config->bits_per_word != impl->bits_per_word) {
		uint8_t bits = config->bits_per_word;
		ret = osal_ioctl(impl->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if (ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_SPI", "Failed to update bits per word: %s (%d)",
					  osal_strerror(err), err);
			result = err;
			goto unlock;
		}
		impl->bits_per_word = config->bits_per_word;
	}

	/* 更新速率 */
	if (config->max_speed_hz != impl->max_speed_hz) {
		uint32_t speed = config->max_speed_hz;
		ret = osal_ioctl(impl->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if (ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_SPI", "Failed to update max speed: %s (%d)",
					  osal_strerror(err), err);
			result = err;
			goto unlock;
		}
		impl->max_speed_hz = config->max_speed_hz;
	}

	impl->timeout = config->timeout;

	if (result == OSAL_SUCCESS) {
		LOG_INFO("HAL_SPI", "Configuration updated");
	}

unlock:
	_hal_linux_unlock_device(impl->flock, &impl->mutex);

	return result;
}
