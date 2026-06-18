/************************************************************************
 * HAL层 - Linux串口驱动实现
 *
 * 基于POSIX termios实现
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "hal_serial_internal.h"
#include <termios.h> /* 系统波特率常量 B9600 等 */

static uint32_t _hal_serial_get_baudrate(uint32_t baudrate)
{
	switch (baudrate) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default:
		return B115200;
	}
}

int32_t hal_serial_open(const char *device, const hal_serial_config_t *config,
						hal_serial_handle_t *handle)
{
	hal_serial_context_t *ctx;
	osal_termios_t tty;
	uint32_t speed;
	int32_t ret; /* 添加缺失的变量声明 */

	if (NULL == device || NULL == handle) {
		return OSAL_ERR_INVALID_POINTER;
	}

	if (NULL == config) {
		return OSAL_ERR_INVALID_POINTER;
	}

	ctx =
		(hal_serial_context_t *)osal_malloc(OSAL_sizeof(hal_serial_context_t));
	if (NULL == ctx) {
		LOG_ERROR("HAL_Serial", "Failed to allocate context");
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(ctx, 0, OSAL_sizeof(hal_serial_context_t));

	/* 打开串口设备（O_EXCL 保证独占访问，防止多进程竞争） */
	ctx->fd = osal_open(
		device, OSAL_O_RDWR | OSAL_O_NOCTTY | OSAL_O_NONBLOCK | OSAL_O_EXCL, 0);
	if (ctx->fd < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Failed to open %s: %s (%d)", device,
				  osal_strerror(err), err);
		osal_free(ctx);
		return err;
	}

	/* 设置为阻塞模式 */
	osal_fcntl(ctx->fd, OSAL_F_SETFL, 0);

	/* 获取当前配置 */
	if (0 != osal_tcgetattr(ctx->fd, &tty)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Failed to get attributes: %s (%d)",
				  osal_strerror(err), err);
		osal_close(ctx->fd);
		osal_free(ctx);
		return err;
	}

	/* 配置波特率 */
	speed = _hal_serial_get_baudrate(config->baud_rate);
	osal_cfsetispeed(&tty, speed);
	osal_cfsetospeed(&tty, speed);

	/* 配置数据位 */
	tty.c_cflag &= ~OSAL_CSIZE;
	if (7 == config->data_bits) {
		tty.c_cflag |= OSAL_CS7;
	} else {
		tty.c_cflag |= OSAL_CS8; /* 默认8位 */
	}

	/* 配置停止位 */
	if (2 == config->stop_bits) {
		tty.c_cflag |= OSAL_CSTOPB;
	} else {
		tty.c_cflag &= ~OSAL_CSTOPB; /* 默认1位 */
	}

	/* 配置校验位 */
	if (1 == config->parity) /* 奇校验 */
	{
		tty.c_cflag |= OSAL_PARENB;
		tty.c_cflag |= OSAL_PARODD;
	} else if (2 == config->parity) /* 偶校验 */
	{
		tty.c_cflag |= OSAL_PARENB;
		tty.c_cflag &= ~OSAL_PARODD;
	} else /* 无校验 */
	{
		tty.c_cflag &= ~OSAL_PARENB;
	}

	/* 启用接收，本地模式 */
	tty.c_cflag |= (OSAL_CLOCAL | OSAL_CREAD);

	/* 禁用硬件流控 */
#ifdef OSAL_CRTSCTS
	tty.c_cflag &= ~OSAL_CRTSCTS;
#endif

	/* 原始模式 */
	tty.c_lflag &= ~(OSAL_ICANON | OSAL_ECHO | OSAL_ECHOE | OSAL_ISIG);
	tty.c_iflag &= ~(OSAL_IXON | OSAL_IXOFF | OSAL_IXANY);
	tty.c_iflag &= ~(OSAL_IGNBRK | OSAL_BRKINT | OSAL_PARMRK | OSAL_ISTRIP |
					 OSAL_INLCR | OSAL_IGNCR | OSAL_ICRNL);
	tty.c_oflag &= ~OSAL_OPOST;

	/* 超时配置 */
	tty.c_cc[OSAL_VMIN] = 0; /* 非阻塞读取 */
	tty.c_cc[OSAL_VTIME] = 0; /* 不等待 */

	/* 应用配置 */
	if (0 != osal_tcsetattr(ctx->fd, OSAL_TCSANOW, &tty)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Failed to set attributes: %s (%d)",
				  osal_strerror(err), err);
		osal_close(ctx->fd);
		osal_free(ctx);
		return err;
	}

	/* 清空缓冲区 */
	osal_tcflush(ctx->fd, OSAL_TCIOFLUSH);

	/* 创建文件锁（进程间保护） */
	char lock_file[OSAL_LOCK_PATH_MAX_LEN];
	char lock_name[OSAL_LOCK_PATH_MAX_LEN];
	const char *dev_name = device;
	uint32_t i;

	if (osal_strncmp(device, "/dev/", 5) == 0) {
		dev_name = device + 5; /* 跳过 /dev/ */
	}

	osal_strncpy(lock_name, dev_name, OSAL_sizeof(lock_name) - 1);
	lock_name[OSAL_sizeof(lock_name) - 1] = '\0';
	for (i = 0; lock_name[i] != '\0'; i++) {
		if (lock_name[i] == '/') {
			lock_name[i] = '_';
		}
	}

	osal_snprintf(lock_file, OSAL_sizeof(lock_file), HAL_SERIAL_LOCK_PATH_FMT,
				  lock_name);
	ret = osal_flock_create(lock_file, &ctx->flock);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to create file lock: %s", lock_file);
		osal_close(ctx->fd);
		osal_free(ctx);
		return ret;
	}

	/* 创建互斥锁（线程间保护） */
	ret = osal_mutex_init(&ctx->mutex, NULL);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to create mutex");
		osal_flock_destroy(ctx->flock);
		osal_close(ctx->fd);
		osal_free(ctx);
		return ret;
	}

	/* 保存配置 */
	osal_memcpy(&ctx->config, config, OSAL_sizeof(hal_serial_config_t));
	osal_strncpy(ctx->device, device, OSAL_sizeof(ctx->device) - 1);
	ctx->device[OSAL_sizeof(ctx->device) - 1] = '\0';

	*handle = (hal_serial_handle_t)ctx;

	LOG_INFO("HAL_Serial",
			 "Opened %s (baudrate=%u, databits=%u, stopbits=%u, parity=%u) "
			 "with dual-lock protection",
			 device, config->baud_rate, config->data_bits, config->stop_bits,
			 config->parity);

	return OSAL_SUCCESS;
}

/************************************************************************
 * hal_serial_close - 关闭串口
 ************************************************************************/
int32_t hal_serial_close(hal_serial_handle_t handle)
{
	hal_serial_context_t *ctx = (hal_serial_context_t *)handle;

	if (NULL == ctx) {
		return OSAL_ERR_INVALID_ID;
	}

	if (ctx->fd >= 0) {
		osal_close(ctx->fd);
		LOG_INFO("HAL_Serial", "Closed %s", ctx->device);
	}

	/* 销毁锁 */
	osal_mutex_destroy(&ctx->mutex);

	if (ctx->flock) {
		osal_flock_destroy(ctx->flock);
	}

	osal_free(ctx);
	return OSAL_SUCCESS;
}

/************************************************************************
 * hal_serial_write - 发送数据
 ************************************************************************/
int32_t hal_serial_write(hal_serial_handle_t handle, const void *buffer,
						 uint32_t size, int32_t timeout)
{
	hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
	osal_fd_set_t writefds;
	osal_timeval_t tv;
	int32_t ret;
	int32_t written;
	int32_t result = OSAL_SUCCESS;

	if (NULL == ctx || NULL == buffer) {
		return OSAL_ERR_INVALID_POINTER;
	}

	if (ctx->fd < 0) {
		return OSAL_ERR_INVALID_ID;
	}

	/*
     * 注意：select 等待期间不应持有锁，否则会阻塞其他进程/线程
     * 策略：先 select，可写后再加锁发送
     */
	if (timeout > 0) {
		osal_fd_zero(&writefds);
		osal_fd_set(ctx->fd, &writefds);

		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		ret = osal_select(ctx->fd + 1, NULL, &writefds, NULL, &tv);
		if (0 == ret) {
			return OSAL_ERR_TIMEOUT;
		} else if (ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_Serial", "Select error: %s (%d)", osal_strerror(err),
					  err);
			return err;
		}
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(ctx->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_SERIAL_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial",
				  "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&ctx->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to acquire mutex");
		osal_flock_unlock(ctx->flock);
		return ret;
	}

	/* 临界区：发送数据 */
	written = osal_write(ctx->fd, buffer, size);
	if (written < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Write error: %s (%d)", osal_strerror(err),
				  err);
		result = err;
	} else {
		result = (int32_t)written;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&ctx->mutex);
	osal_flock_unlock(ctx->flock);

	return result;
}

/************************************************************************
 * hal_serial_read - 接收数据
 ************************************************************************/
int32_t hal_serial_read(hal_serial_handle_t handle, void *buffer, uint32_t size,
						int32_t timeout)
{
	hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
	osal_fd_set_t readfds;
	osal_timeval_t tv;
	int32_t ret;
	int32_t nread;
	int32_t result = OSAL_SUCCESS;

	if (NULL == ctx || NULL == buffer) {
		return OSAL_ERR_INVALID_POINTER;
	}

	if (ctx->fd < 0) {
		return OSAL_ERR_INVALID_ID;
	}

	/*
     * 注意：select 等待期间不应持有锁，否则会阻塞其他进程/线程
     * 策略：先 select，有数据后再加锁读取
     */
	if (timeout > 0) {
		osal_fd_zero(&readfds);
		osal_fd_set(ctx->fd, &readfds);

		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		ret = osal_select(ctx->fd + 1, &readfds, NULL, NULL, &tv);
		if (0 == ret) {
			return OSAL_ERR_TIMEOUT;
		} else if (ret < 0) {
			int32_t err = osal_get_errno();
			LOG_ERROR("HAL_Serial", "Select error: %s (%d)", osal_strerror(err),
					  err);
			return err;
		}
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(ctx->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_SERIAL_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial",
				  "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&ctx->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to acquire mutex");
		osal_flock_unlock(ctx->flock);
		return ret;
	}

	/* 临界区：读取数据 */
	nread = osal_read(ctx->fd, buffer, size);
	if (nread < 0) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Read error: %s (%d)", osal_strerror(err), err);
		result = err;
	} else {
		result = (int32_t)nread;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&ctx->mutex);
	osal_flock_unlock(ctx->flock);

	return result;
}

/************************************************************************
 * hal_serial_flush - 清空接收缓冲区
 ************************************************************************/
int32_t hal_serial_flush(hal_serial_handle_t handle)
{
	hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == ctx) {
		return OSAL_ERR_INVALID_ID;
	}

	if (ctx->fd < 0) {
		return OSAL_ERR_INVALID_ID;
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(ctx->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_SERIAL_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial",
				  "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&ctx->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to acquire mutex");
		osal_flock_unlock(ctx->flock);
		return ret;
	}

	/* 临界区：清空输入输出缓冲区 */
	if (0 != osal_tcflush(ctx->fd, OSAL_TCIOFLUSH)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Flush error: %s (%d)", osal_strerror(err),
				  err);
		result = err;
	}

	/* 释放锁（逆序） */
	osal_mutex_unlock(&ctx->mutex);
	osal_flock_unlock(ctx->flock);

	return result;
}

/************************************************************************
 * hal_serial_set_config - 动态设置串口配置
 ************************************************************************/
int32_t hal_serial_set_config(hal_serial_handle_t handle,
							  const hal_serial_config_t *config)
{
	hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
	osal_termios_t tty;
	uint32_t speed;
	int32_t ret;
	int32_t result = OSAL_SUCCESS;

	if (NULL == ctx || NULL == config) {
		return OSAL_ERR_INVALID_POINTER;
	}

	if (ctx->fd < 0) {
		return OSAL_ERR_INVALID_ID;
	}

	/* 第一层：文件锁（进程间保护） */
	ret = osal_flock_timed_lock(ctx->flock, OSAL_FLOCK_EXCLUSIVE,
								HAL_SERIAL_LOCK_TIMEOUT_MS);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial",
				  "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	/* 第二层：互斥锁（线程间保护） */
	ret = osal_mutex_lock(&ctx->mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("HAL_Serial", "Failed to acquire mutex");
		osal_flock_unlock(ctx->flock);
		return ret;
	}

	/* 临界区：配置串口参数 */
	/* 获取当前配置 */
	if (0 != osal_tcgetattr(ctx->fd, &tty)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Failed to get attributes: %s (%d)",
				  osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	/* 配置波特率 */
	speed = _hal_serial_get_baudrate(config->baud_rate);
	osal_cfsetispeed(&tty, speed);
	osal_cfsetospeed(&tty, speed);

	/* 配置数据位 */
	tty.c_cflag &= ~OSAL_CSIZE;
	if (7 == config->data_bits) {
		tty.c_cflag |= OSAL_CS7;
	} else {
		tty.c_cflag |= OSAL_CS8; /* 默认8位 */
	}

	/* 配置停止位 */
	if (2 == config->stop_bits) {
		tty.c_cflag |= OSAL_CSTOPB;
	} else {
		tty.c_cflag &= ~OSAL_CSTOPB; /* 默认1位 */
	}

	/* 配置校验位 */
	if (1 == config->parity) /* 奇校验 */
	{
		tty.c_cflag |= OSAL_PARENB;
		tty.c_cflag |= OSAL_PARODD;
	} else if (2 == config->parity) /* 偶校验 */
	{
		tty.c_cflag |= OSAL_PARENB;
		tty.c_cflag &= ~OSAL_PARODD;
	} else /* 无校验 */
	{
		tty.c_cflag &= ~OSAL_PARENB;
	}

	/* 应用配置 */
	if (0 != osal_tcsetattr(ctx->fd, OSAL_TCSANOW, &tty)) {
		int32_t err = osal_get_errno();
		LOG_ERROR("HAL_Serial", "Failed to set attributes: %s (%d)",
				  osal_strerror(err), err);
		result = err;
		goto unlock;
	}

	/* 更新内部配置 */
	osal_memcpy(&ctx->config, config, OSAL_sizeof(hal_serial_config_t));

	LOG_INFO("HAL_Serial",
			 "Config updated: baudrate=%u, databits=%u, stopbits=%u, parity=%u",
			 config->baud_rate, config->data_bits, config->stop_bits,
			 config->parity);

unlock:
	/* 释放锁（逆序） */
	osal_mutex_unlock(&ctx->mutex);
	osal_flock_unlock(ctx->flock);

	return result;
}
