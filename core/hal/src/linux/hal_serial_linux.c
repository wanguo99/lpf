/************************************************************************
 * HAL层 - Linux串口驱动实现
 *
 * 基于POSIX termios实现
 ************************************************************************/

#include "hal_serial.h"
#include "osal.h"
#include <termios.h>  /* 系统波特率常量 B9600 等 */
#include <pthread.h>

typedef struct
{
    int32_t fd;
    hal_serial_config_t config;
    char device[64];
    pthread_mutex_t lock;
} hal_serial_context_t;

static uint32_t hal_serial_get_baudrate(uint32_t baudrate)
{
    switch (baudrate)
    {
        case 9600:    return B9600;
        case 19200:   return B19200;
        case 38400:   return B38400;
        case 57600:   return B57600;
        case 115200:  return B115200;
        case 230400:  return B230400;
        case 460800:  return B460800;
        case 500000:  return B500000;
        case 576000:  return B576000;
        case 921600:  return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 1500000: return B1500000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 3500000: return B3500000;
        case 4000000: return B4000000;
        default:      return B115200;
    }
}

int32_t HAL_Serial_Open(const char *device, const hal_serial_config_t *config, hal_serial_handle_t *handle)
{
    hal_serial_context_t *ctx;
    osal_termios_t tty;
    uint32_t speed;

    if (NULL == device || NULL == handle)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (NULL == config)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (hal_serial_context_t *)OSAL_Malloc(sizeof(hal_serial_context_t));
    if (NULL == ctx)
    {
        LOG_ERROR("HAL_Serial", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_Memset(ctx, 0, sizeof(hal_serial_context_t));

    /* 打开串口设备（O_EXCL 保证独占访问，防止多进程竞争） */
    ctx->fd = OSAL_open(device, OSAL_O_RDWR | OSAL_O_NOCTTY | OSAL_O_NONBLOCK | OSAL_O_EXCL, 0);
    if (ctx->fd < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (OSAL_EBUSY == err)
        {
            LOG_ERROR("HAL_Serial", "Device %s is busy (already opened by another process)", device);
            OSAL_Free(ctx);
            return OSAL_ERR_BUSY;
        }
        LOG_ERROR("HAL_Serial", "Failed to open %s: %s", device, OSAL_StrError(err));
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 设置为阻塞模式 */
    OSAL_fcntl(ctx->fd, OSAL_F_SETFL, 0);

    /* 获取当前配置 */
    if (0 != OSAL_tcgetattr(ctx->fd, &tty))
    {
        LOG_ERROR("HAL_Serial", "Failed to get attributes: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_close(ctx->fd);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 配置波特率 */
    speed = hal_serial_get_baudrate(config->baud_rate);
    OSAL_cfsetispeed(&tty, speed);
    OSAL_cfsetospeed(&tty, speed);

    /* 配置数据位 */
    tty.c_cflag &= ~OSAL_CSIZE;
    if (7 == config->data_bits)
    {
        tty.c_cflag |= OSAL_CS7;
    }
    else
    {
        tty.c_cflag |= OSAL_CS8;  /* 默认8位 */
    }

    /* 配置停止位 */
    if (2 == config->stop_bits)
    {
        tty.c_cflag |= OSAL_CSTOPB;
    }
    else
    {
        tty.c_cflag &= ~OSAL_CSTOPB;  /* 默认1位 */
    }

    /* 配置校验位 */
    if (1 == config->parity)  /* 奇校验 */
    {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag |= OSAL_PARODD;
    }
    else if (2 == config->parity)  /* 偶校验 */
    {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag &= ~OSAL_PARODD;
    }
    else  /* 无校验 */
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
    tty.c_iflag &= ~(OSAL_IGNBRK | OSAL_BRKINT | OSAL_PARMRK | OSAL_ISTRIP | OSAL_INLCR | OSAL_IGNCR | OSAL_ICRNL);
    tty.c_oflag &= ~OSAL_OPOST;

    /* 超时配置 */
    tty.c_cc[OSAL_VMIN]  = 0;  /* 非阻塞读取 */
    tty.c_cc[OSAL_VTIME] = 0;  /* 不等待 */

    /* 应用配置 */
    if (0 != OSAL_tcsetattr(ctx->fd, OSAL_TCSANOW, &tty))
    {
        LOG_ERROR("HAL_Serial", "Failed to set attributes: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_close(ctx->fd);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 清空缓冲区 */
    OSAL_tcflush(ctx->fd, OSAL_TCIOFLUSH);

    /* 初始化互斥锁 */
    if (0 != pthread_mutex_init(&ctx->lock, NULL))
    {
        LOG_ERROR("HAL_Serial", "Failed to initialize mutex");
        OSAL_close(ctx->fd);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 保存配置 */
    OSAL_Memcpy(&ctx->config, config, sizeof(hal_serial_config_t));
    OSAL_Strncpy(ctx->device, device, sizeof(ctx->device) - 1);
    ctx->device[sizeof(ctx->device) - 1] = '\0';

    *handle = (hal_serial_handle_t)ctx;

    LOG_INFO("HAL_Serial", "Opened %s (baudrate=%u, databits=%u, stopbits=%u, parity=%u)",
              device, config->baud_rate, config->data_bits, config->stop_bits, config->parity);

    return OSAL_SUCCESS;
}

/************************************************************************
 * HAL_Serial_Close - 关闭串口
 ************************************************************************/
int32_t HAL_Serial_Close(hal_serial_handle_t handle)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;

    if (NULL == ctx)
    {
        return OSAL_ERR_INVALID_ID;
    }

    if (ctx->fd >= 0)
    {
        OSAL_close(ctx->fd);
        LOG_INFO("HAL_Serial", "Closed %s", ctx->device);
    }

    pthread_mutex_destroy(&ctx->lock);

    OSAL_Free(ctx);
    return OSAL_SUCCESS;
}

/************************************************************************
 * HAL_Serial_Write - 发送数据
 ************************************************************************/
int32_t HAL_Serial_Write(hal_serial_handle_t handle, const void *buffer, uint32_t size, int32_t timeout)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
    osal_fd_set_t writefds;
    osal_timeval_t tv;
    int32_t ret;
    int32_t written;

    if (NULL == ctx || NULL == buffer)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (ctx->fd < 0)
    {
        return OSAL_ERR_INVALID_ID;
    }

    pthread_mutex_lock(&ctx->lock);

    /* 设置超时 */
    if (timeout > 0)
    {
        OSAL_FD_ZERO(&writefds);
        OSAL_FD_SET(ctx->fd, &writefds);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        ret = OSAL_select(ctx->fd + 1, NULL, &writefds, NULL, &tv);
        if (0 == ret)
        {
            pthread_mutex_unlock(&ctx->lock);
            return OSAL_ERR_TIMEOUT;
        }
        else if (ret < 0)
        {
            LOG_ERROR("HAL_Serial", "Select error: %s", OSAL_StrError(OSAL_GetErrno()));
            pthread_mutex_unlock(&ctx->lock);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 发送数据 */
    written = OSAL_write(ctx->fd, buffer, size);
    if (written < 0)
    {
        LOG_ERROR("HAL_Serial", "Write error: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&ctx->lock);
        return OSAL_ERR_GENERIC;
    }

    pthread_mutex_unlock(&ctx->lock);
    return (int32_t)written;
}

/************************************************************************
 * HAL_Serial_Read - 接收数据
 ************************************************************************/
int32_t HAL_Serial_Read(hal_serial_handle_t handle, void *buffer, uint32_t size, int32_t timeout)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
    osal_fd_set_t readfds;
    osal_timeval_t tv;
    int32_t ret;
    int32_t nread;

    if (NULL == ctx || NULL == buffer)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (ctx->fd < 0)
    {
        return OSAL_ERR_INVALID_ID;
    }

    pthread_mutex_lock(&ctx->lock);

    /* 设置超时 */
    if (timeout > 0)
    {
        OSAL_FD_ZERO(&readfds);
        OSAL_FD_SET(ctx->fd, &readfds);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        ret = OSAL_select(ctx->fd + 1, &readfds, NULL, NULL, &tv);
        if (0 == ret)
        {
            pthread_mutex_unlock(&ctx->lock);
            return OSAL_ERR_TIMEOUT;
        }
        else if (ret < 0)
        {
            LOG_ERROR("HAL_Serial", "Select error: %s", OSAL_StrError(OSAL_GetErrno()));
            pthread_mutex_unlock(&ctx->lock);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 接收数据 */
    nread = OSAL_read(ctx->fd, buffer, size);
    if (nread < 0)
    {
        LOG_ERROR("HAL_Serial", "Read error: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&ctx->lock);
        return OSAL_ERR_GENERIC;
    }

    pthread_mutex_unlock(&ctx->lock);
    return (int32_t)nread;
}

/************************************************************************
 * HAL_Serial_Flush - 清空接收缓冲区
 ************************************************************************/
int32_t HAL_Serial_Flush(hal_serial_handle_t handle)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;

    if (NULL == ctx)
    {
        return OSAL_ERR_INVALID_ID;
    }

    if (ctx->fd < 0)
    {
        return OSAL_ERR_INVALID_ID;
    }

    pthread_mutex_lock(&ctx->lock);

    /* 清空输入输出缓冲区 */
    if (0 != OSAL_tcflush(ctx->fd, OSAL_TCIOFLUSH))
    {
        LOG_ERROR("HAL_Serial", "Flush error: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&ctx->lock);
        return OSAL_ERR_GENERIC;
    }

    pthread_mutex_unlock(&ctx->lock);
    return OSAL_SUCCESS;
}

/************************************************************************
 * HAL_Serial_SetConfig - 动态设置串口配置
 ************************************************************************/
int32_t HAL_Serial_SetConfig(hal_serial_handle_t handle,
                            const hal_serial_config_t *config)
{
    hal_serial_context_t *ctx = (hal_serial_context_t *)handle;
    osal_termios_t tty;
    uint32_t speed;

    if (NULL == ctx || NULL == config)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (ctx->fd < 0)
    {
        return OSAL_ERR_INVALID_ID;
    }

    pthread_mutex_lock(&ctx->lock);

    /* 获取当前配置 */
    if (0 != OSAL_tcgetattr(ctx->fd, &tty))
    {
        LOG_ERROR("HAL_Serial", "Failed to get attributes: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&ctx->lock);
        return OSAL_ERR_GENERIC;
    }

    /* 配置波特率 */
    speed = hal_serial_get_baudrate(config->baud_rate);
    OSAL_cfsetispeed(&tty, speed);
    OSAL_cfsetospeed(&tty, speed);

    /* 配置数据位 */
    tty.c_cflag &= ~OSAL_CSIZE;
    if (7 == config->data_bits)
    {
        tty.c_cflag |= OSAL_CS7;
    }
    else
    {
        tty.c_cflag |= OSAL_CS8;  /* 默认8位 */
    }

    /* 配置停止位 */
    if (2 == config->stop_bits)
    {
        tty.c_cflag |= OSAL_CSTOPB;
    }
    else
    {
        tty.c_cflag &= ~OSAL_CSTOPB;  /* 默认1位 */
    }

    /* 配置校验位 */
    if (1 == config->parity)  /* 奇校验 */
    {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag |= OSAL_PARODD;
    }
    else if (2 == config->parity)  /* 偶校验 */
    {
        tty.c_cflag |= OSAL_PARENB;
        tty.c_cflag &= ~OSAL_PARODD;
    }
    else  /* 无校验 */
    {
        tty.c_cflag &= ~OSAL_PARENB;
    }

    /* 应用配置 */
    if (0 != OSAL_tcsetattr(ctx->fd, OSAL_TCSANOW, &tty))
    {
        LOG_ERROR("HAL_Serial", "Failed to set attributes: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&ctx->lock);
        return OSAL_ERR_GENERIC;
    }

    /* 更新内部配置 */
    OSAL_Memcpy(&ctx->config, config, sizeof(hal_serial_config_t));

    pthread_mutex_unlock(&ctx->lock);

    LOG_INFO("HAL_Serial", "Config updated: baudrate=%u, databits=%u, stopbits=%u, parity=%u",
             config->baud_rate, config->data_bits, config->stop_bits, config->parity);

    return OSAL_SUCCESS;
}
