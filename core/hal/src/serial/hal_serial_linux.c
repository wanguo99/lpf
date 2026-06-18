/************************************************************************
 * HAL层 - Linux串口驱动实现
 *
 * 基于POSIX termios实现
 ************************************************************************/

#include <termios.h> /* 系统波特率常量 B9600 等 */
#include "osal.h"
#include "hal.h"
#include "hal_serial_internal.h"

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

static int32_t _hal_serial_apply_config(int32_t fd,
                                        const hal_serial_config_t *config)
{
    osal_termios_t tty;
    uint32_t speed;

    if (0 != osal_tcgetattr(fd, &tty)) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Failed to get attributes: %s (%d)",
                  osal_strerror(err), err);
        return err;
    }

    speed = _hal_serial_get_baudrate(config->baud_rate);
    osal_cfsetispeed(&tty, speed);
    osal_cfsetospeed(&tty, speed);

    tty.c_cflag &= ~OSAL_CSIZE;
    if (7 == config->data_bits) {
        tty.c_cflag |= OSAL_CS7;
    } else {
        tty.c_cflag |= OSAL_CS8;
    }

    if (2 == config->stop_bits) {
        tty.c_cflag |= OSAL_CSTOPB;
    } else {
        tty.c_cflag &= ~OSAL_CSTOPB;
    }

    if (1 == config->parity) {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag |= OSAL_PARODD;
    } else if (2 == config->parity) {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag &= ~OSAL_PARODD;
    } else {
        tty.c_cflag &= ~OSAL_PARENB;
    }

    tty.c_cflag |= (OSAL_CLOCAL | OSAL_CREAD);

#ifdef OSAL_CRTSCTS
    tty.c_cflag &= ~OSAL_CRTSCTS;
#endif

    tty.c_lflag &= ~(OSAL_ICANON | OSAL_ECHO | OSAL_ECHOE | OSAL_ISIG);
    tty.c_iflag &= ~(OSAL_IXON | OSAL_IXOFF | OSAL_IXANY);
    tty.c_iflag &= ~(OSAL_IGNBRK | OSAL_BRKINT | OSAL_PARMRK | OSAL_ISTRIP |
                     OSAL_INLCR | OSAL_IGNCR | OSAL_ICRNL);
    tty.c_oflag &= ~OSAL_OPOST;

    tty.c_cc[OSAL_VMIN] = 0;
    tty.c_cc[OSAL_VTIME] = 0;

    if (0 != osal_tcsetattr(fd, OSAL_TCSANOW, &tty)) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Failed to set attributes: %s (%d)",
                  osal_strerror(err), err);
        return err;
    }

    return OSAL_SUCCESS;
}

static void _hal_serial_build_lock_file(const char *device, char *lock_file,
                                        osal_size_t lock_file_size)
{
    char lock_name[OSAL_LOCK_PATH_MAX_LEN];
    const char *dev_name = device;
    uint32_t i;

    if (osal_strncmp(device, "/dev/", 5) == 0)
        dev_name = device + 5;

    osal_strncpy(lock_name, dev_name, OSAL_sizeof(lock_name) - 1);
    lock_name[OSAL_sizeof(lock_name) - 1] = '\0';
    for (i = 0; lock_name[i] != '\0'; i++) {
        if (lock_name[i] == '/')
            lock_name[i] = '_';
    }

    osal_snprintf(lock_file, lock_file_size, HAL_SERIAL_LOCK_PATH_FMT,
                  lock_name);
}

int32_t hal_serial_open(const char *device, const hal_serial_config_t *config,
                        hal_serial_handle_t *handle)
{
    hal_serial_context_t *ctx;
    char lock_file[OSAL_LOCK_PATH_MAX_LEN];
    int32_t ret;

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
    ctx->fd = -1;
    ctx->initialized = false;

    _hal_serial_build_lock_file(device, lock_file, OSAL_sizeof(lock_file));
    ret = _hal_linux_lock_init(&ctx->lock, lock_file, "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        goto err_free;

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        goto err_lock;

    ctx->fd =
        osal_open(device, OSAL_O_RDWR | OSAL_O_NOCTTY | OSAL_O_NONBLOCK, 0);
    if (ctx->fd < 0) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Failed to open %s: %s (%d)", device,
                  osal_strerror(err), err);
        ret = err;
        goto err_unlock;
    }

    if (osal_fcntl(ctx->fd, OSAL_F_SETFL, 0) < 0) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Failed to set blocking mode: %s (%d)",
                  osal_strerror(err), err);
        ret = err;
        goto err_close;
    }

    ret = _hal_serial_apply_config(ctx->fd, config);
    if (ret != OSAL_SUCCESS)
        goto err_close;

    if (0 != osal_tcflush(ctx->fd, OSAL_TCIOFLUSH)) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Failed to flush device: %s (%d)",
                  osal_strerror(err), err);
        ret = err;
        goto err_close;
    }

    /* 保存配置 */
    osal_memcpy(&ctx->config, config, OSAL_sizeof(hal_serial_config_t));
    osal_strncpy(ctx->device, device, OSAL_sizeof(ctx->device) - 1);
    ctx->device[OSAL_sizeof(ctx->device) - 1] = '\0';
    ctx->initialized = true;

    *handle = (hal_serial_handle_t)ctx;
    _hal_linux_lock_release(&ctx->lock);

    LOG_INFO("HAL_Serial",
             "Opened %s (baudrate=%u, databits=%u, stopbits=%u, parity=%u) "
             "with process/thread protection",
             device, config->baud_rate, config->data_bits, config->stop_bits,
             config->parity);

    return OSAL_SUCCESS;

err_close:
    _hal_linux_close_fd(&ctx->fd);
err_unlock:
    _hal_linux_lock_release(&ctx->lock);
err_lock:
    _hal_linux_lock_deinit(&ctx->lock);
err_free:
    osal_free(ctx);
    return ret;
}

/************************************************************************
 * hal_serial_close - 关闭串口
 ************************************************************************/
int32_t hal_serial_close(hal_serial_handle_t handle)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
    int32_t ret;

    if (NULL == ctx) {
        return OSAL_ERR_INVALID_ID;
    }

    if (!ctx->initialized || ctx->fd < 0)
        return OSAL_ERR_INVALID_ID;

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        return ret;
    ctx->initialized = false;
    _hal_linux_close_fd(&ctx->fd);
    _hal_linux_lock_release(&ctx->lock);
    _hal_linux_lock_deinit(&ctx->lock);
    LOG_INFO("HAL_Serial", "Closed %s", ctx->device);
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

    if (!ctx->initialized || ctx->fd < 0) {
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

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        return ret;

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

    _hal_linux_lock_release(&ctx->lock);

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

    if (!ctx->initialized || ctx->fd < 0) {
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

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        return ret;

    /* 临界区：读取数据 */
    nread = osal_read(ctx->fd, buffer, size);
    if (nread < 0) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Read error: %s (%d)", osal_strerror(err), err);
        result = err;
    } else {
        result = (int32_t)nread;
    }

    _hal_linux_lock_release(&ctx->lock);

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

    if (!ctx->initialized || ctx->fd < 0) {
        return OSAL_ERR_INVALID_ID;
    }

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        return ret;

    /* 临界区：清空输入输出缓冲区 */
    if (0 != osal_tcflush(ctx->fd, OSAL_TCIOFLUSH)) {
        int32_t err = osal_get_errno();
        LOG_ERROR("HAL_Serial", "Flush error: %s (%d)", osal_strerror(err),
                  err);
        result = err;
    }

    _hal_linux_lock_release(&ctx->lock);

    return result;
}

/************************************************************************
 * hal_serial_set_config - 动态设置串口配置
 ************************************************************************/
int32_t hal_serial_set_config(hal_serial_handle_t handle,
                              const hal_serial_config_t *config)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
    int32_t ret;

    if (NULL == ctx || NULL == config) {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (!ctx->initialized || ctx->fd < 0) {
        return OSAL_ERR_INVALID_ID;
    }

    ret = _hal_linux_lock_acquire(&ctx->lock, HAL_SERIAL_LOCK_TIMEOUT_MS,
                                  "HAL_Serial");
    if (ret != OSAL_SUCCESS)
        return ret;

    ret = _hal_serial_apply_config(ctx->fd, config);
    if (ret != OSAL_SUCCESS)
        goto unlock;

    /* 更新内部配置 */
    osal_memcpy(&ctx->config, config, OSAL_sizeof(hal_serial_config_t));

    LOG_INFO("HAL_Serial",
             "Config updated: baudrate=%u, databits=%u, stopbits=%u, parity=%u",
             config->baud_rate, config->data_bits, config->stop_bits,
             config->parity);

unlock:
    _hal_linux_lock_release(&ctx->lock);

    return ret;
}
