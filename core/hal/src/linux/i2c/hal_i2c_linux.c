/************************************************************************
 * HAL层 - I2C驱动Linux实现
 ************************************************************************/

/* 必须在所有头文件之前定义，以启用完整的POSIX功能 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "osal.h"
#include "hal.h"
#include "hal_i2c_internal.h"

/**
 * @brief 打开I2C设备
 */
int32_t hal_i2c_open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle)
{
	hal_i2c_context_t *impl;
	int32_t ret;

	/* 参数检查 */
	if (NULL == config || NULL == handle)
		return OSAL_ERR_INVALID_POINTER;

	if (NULL == config->device || 0 == osal_strlen(config->device))
		return OSAL_ERR_GENERIC;

	/* 分配句柄 */
	impl = (hal_i2c_context_t *)osal_malloc(OSAL_sizeof(hal_i2c_context_t));
	if (NULL == impl) {
		LOG_ERROR("HAL_I2C", "Failed to allocate memory");
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(impl, 0, OSAL_sizeof(hal_i2c_context_t));
	osal_strncpy(impl->device, config->device, OSAL_sizeof(impl->device) - 1);
	impl->device[OSAL_sizeof(impl->device) - 1] = '\0';
	impl->timeout = config->timeout;
	impl->initialized = false;

	/* 创建文件锁（进程间保护） */
	char lock_file[OSAL_LOCK_PATH_MAX_LEN];
	/* 从设备路径提取总线号，例如 /dev/i2c-0 -> 0 */
	const char *dev_name = config->device;
	const char *slash = config->device;
	while (*slash) {
		if (*slash == '/')
			dev_name = slash + 1;
		slash++;
	}
	/* 提取数字部分，例如 i2c-0 -> 0 */
	int bus_num = 0;
	if (osal_sscanf(dev_name, "i2c-%d", &bus_num) == 1) {
		osal_snprintf(lock_file, OSAL_sizeof(lock_file), HAL_I2C_LOCK_PATH_FMT,
					  bus_num);
	} else {
		/* 如果无法解析，使用设备名 */
		osal_snprintf(lock_file, OSAL_sizeof(lock_file),
					  OSAL_LOCK_DIR "/hal_i2c_%s.lock", dev_name);
	}

	ret = osal_flock_create(lock_file, &impl->flock);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to create file lock: %s", lock_file);
		osal_free(impl);
		return ret;
	}

	/* 创建互斥锁（线程间保护） */
	ret = osal_mutex_init(&impl->mutex, NULL);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to create mutex");
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return ret;
	}

	/* 打开I2C设备（不使用 O_EXCL，由文件锁保护） */
	impl->fd = osal_open(config->device, OSAL_O_RDWR, 0);
	if (impl->fd < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Failed to open device %s: %s (%d)",
				  config->device, osal_strerror(err), err);
		osal_mutex_destroy(&impl->mutex);
		osal_flock_destroy(impl->flock);
		osal_free(impl);
		return err;
	}

	impl->initialized = true;
	*handle = (hal_i2c_handle_t)impl;

	LOG_INFO("HAL_I2C", "Opened device %s (fd=%d, with dual-lock protection)",
			 config->device, impl->fd);
	return OSAL_SUCCESS;
}

/**
 * @brief 关闭I2C设备
 */
int32_t hal_i2c_close(hal_i2c_handle_t handle)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;

	if (NULL == impl)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized)
		return OSAL_ERR_GENERIC;

	if (impl->fd >= 0) {
		osal_close(impl->fd);
		impl->fd = -1;
	}

	impl->initialized = false;

	/* 销毁锁 */
	osal_mutex_destroy(&impl->mutex);

	if (impl->flock) {
		osal_flock_destroy(impl->flock);
	}

	osal_free(impl);

	LOG_INFO("HAL_I2C", "Device closed");
	return OSAL_SUCCESS;
}

/**
 * @brief 写入数据到I2C从设备
 */
int32_t hal_i2c_write(hal_i2c_handle_t handle, uint16_t slave_addr,
					  const uint8_t *buffer, uint32_t size)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_I2C_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：硬件访问 */
	/* 设置从设备地址 (参考: Linux内核文档 i2c-dev.txt) */
	ret = osal_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s (%d)",
				  slave_addr, osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	/* 写入数据 */
	ret = osal_write(impl->fd, buffer, size);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Write failed: %s (%d)", osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	if ((uint32_t)ret != size) {
		LOG_ERROR("HAL_I2C", "Partial write: %d/%u bytes", ret, size);
		result = OSAL_ERR_GENERIC;
	}

unlock:
	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}

/**
 * @brief 从I2C从设备读取数据
 */
int32_t hal_i2c_read(hal_i2c_handle_t handle, uint16_t slave_addr,
					 uint8_t *buffer, uint32_t size)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_I2C_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：硬件访问 */
	/* 设置从设备地址 */
	ret = osal_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s (%d)",
				  slave_addr, osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	/* 读取数据 */
	ret = osal_read(impl->fd, buffer, size);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Read failed: %s (%d)", osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	if ((uint32_t)ret != size) {
		LOG_ERROR("HAL_I2C", "Partial read: %d/%u bytes", ret, size);
		result = OSAL_ERR_GENERIC;
	}

unlock:
	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}

/**
 * @brief 写寄存器操作
 */
int32_t hal_i2c_write_reg(hal_i2c_handle_t handle, uint16_t slave_addr,
						  uint8_t reg_addr, const uint8_t *buffer,
						  uint32_t size)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
	uint8_t *write_buf;
	int32_t ret;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	/* 分配临时缓冲区: [寄存器地址][数据] */
	write_buf = (uint8_t *)osal_malloc(size + 1);
	if (NULL == write_buf) {
		LOG_ERROR("HAL_I2C", "Failed to allocate write buffer");
		return OSAL_ERR_NO_MEMORY;
	}

	write_buf[0] = reg_addr;
	osal_memcpy(&write_buf[1], buffer, size);

	/* 执行写操作 */
	ret = hal_i2c_write(handle, slave_addr, write_buf, size + 1);

	osal_free(write_buf);
	return ret;
}

/**
 * @brief 读寄存器操作
 */
int32_t hal_i2c_read_reg(hal_i2c_handle_t handle, uint16_t slave_addr,
						 uint8_t reg_addr, uint8_t *buffer, uint32_t size)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
	struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data msgset;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == impl || NULL == buffer)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || size == 0)
		return OSAL_ERR_GENERIC;

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_I2C_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		return ret;
	}

	/* 临界区：硬件访问 */
	/* 组合传输: 先写寄存器地址，再读数据 (参考: Linux内核 i2c.h) */
	msgs[0].addr = slave_addr;
	msgs[0].flags = 0; /* 写操作 */
	msgs[0].len = 1;
	msgs[0].buf = &reg_addr;

	msgs[1].addr = slave_addr;
	msgs[1].flags = I2C_M_RD; /* 读操作 */
	msgs[1].len = size;
	msgs[1].buf = buffer;

	msgset.msgs = msgs;
	msgset.nmsgs = 2;

	ret = osal_ioctl(impl->fd, I2C_RDWR, &msgset);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Read register failed: %s (%d)",
				  osal_strerror(err), err);
		result = err;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	return result;
}

/**
 * @brief 执行I2C传输
 */
int32_t hal_i2c_transfer(hal_i2c_handle_t handle, hal_i2c_msg_t *msgs,
						 uint32_t num)
{
	hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
	struct i2c_msg *kernel_msgs;
	struct i2c_rdwr_ioctl_data msgset;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;
	uint32_t i;

	if (NULL == impl || NULL == msgs)
		return OSAL_ERR_INVALID_POINTER;

	if (!impl->initialized || num == 0)
		return OSAL_ERR_GENERIC;

	/* 分配内核消息结构 */
	kernel_msgs =
		(struct i2c_msg *)osal_malloc(OSAL_sizeof(struct i2c_msg) * num);
	if (NULL == kernel_msgs) {
		LOG_ERROR("HAL_I2C", "Failed to allocate message buffer");
		return OSAL_ERR_NO_MEMORY;
	}

	/* 转换消息格式 */
	for (i = 0; i < num; i++) {
		kernel_msgs[i].addr = msgs[i].addr;
		kernel_msgs[i].flags = msgs[i].flags;
		kernel_msgs[i].len = msgs[i].len;
		kernel_msgs[i].buf = msgs[i].buf;
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(impl->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_I2C_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
		osal_free(kernel_msgs);
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&impl->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
		osal_flock_unlock(impl->flock);
		osal_free(kernel_msgs);
		return ret;
	}

	/* 临界区：硬件访问 */
	msgset.msgs = kernel_msgs;
	msgset.nmsgs = num;

	/* 执行传输 */
	ret = osal_ioctl(impl->fd, I2C_RDWR, &msgset);
	if (ret < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_I2C", "Transfer failed: %s (%d)", osal_strerror(err),
				  err);
		result = err;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&impl->mutex);
	osal_flock_unlock(impl->flock);

	osal_free(kernel_msgs);
	return result;
}
