/************************************************************************
 * HAL层 - SPI驱动Linux实现
 ************************************************************************/

/* 必须在所有头文件之前定义，以启用完整的POSIX功能 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "hal/hal_spi_api.h"
#include "hal_spi_internal.h"
#include "osal/osal.h"

/**
 * @brief 打开SPI设备
 */
int32_t HAL_SPI_Open(const hal_spi_config_t *config, hal_spi_handle_t *handle)
{
    hal_spi_context_t *impl;
    int32_t ret;

    /* 参数检查 */
    if (NULL == config || NULL == handle)
        return OSAL_ERR_INVALID_POINTER;

    if (NULL == config->device || 0 == OSAL_Strlen(config->device))
        return OSAL_ERR_GENERIC;

    /* 分配句柄 */
    impl = (hal_spi_context_t *)OSAL_Malloc(sizeof(hal_spi_context_t));
    if (NULL == impl)
    {
        LOG_ERROR("HAL_SPI", "Failed to allocate memory");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(impl, 0, sizeof(hal_spi_context_t));
    OSAL_Strncpy(impl->device, config->device, sizeof(impl->device) - 1);
    impl->device[sizeof(impl->device) - 1] = '\0';
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
    while (*slash)
    {
        if (*slash == '/')
            dev_name = slash + 1;
        slash++;
    }

    OSAL_Snprintf(lock_file, sizeof(lock_file), HAL_SPI_LOCK_PATH_FMT, dev_name);
    ret = OSAL_FlockCreate(lock_file, &impl->flock);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to create file lock: %s", lock_file);
        OSAL_Free(impl);
        return ret;
    }

    /* 创建互斥锁（线程间保护） */
    ret = OSAL_MutexCreate(&impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to create mutex");
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return ret;
    }

    /* 打开SPI设备 */
    impl->fd = OSAL_open(config->device, OSAL_O_RDWR, 0);
    if (impl->fd < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Failed to open device %s: %s %d (%d)",
                  config->device, OSAL_StrError(err), err);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return err;
    }

    /* 设置SPI模式 (参考: Linux内核文档 spidev.txt) */
    ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_MODE, &impl->mode);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Failed to set SPI mode: %s %d (%d)",
                  OSAL_StrError(err), err);
        OSAL_close(impl->fd);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return err;
    }

    /* 设置每字位数 */
    ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_BITS_PER_WORD, &impl->bits_per_word);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Failed to set bits per word: %s %d (%d)",
                  OSAL_StrError(err), err);
        OSAL_close(impl->fd);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return err;
    }

    /* 设置最大速率 */
    ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_MAX_SPEED_HZ, &impl->max_speed_hz);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Failed to set max speed: %s %d (%d)",
                  OSAL_StrError(err), err);
        OSAL_close(impl->fd);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return err;
    }

    impl->initialized = true;
    *handle = (hal_spi_handle_t)impl;

    LOG_INFO("HAL_SPI", "Opened device %s (fd=%d, mode=%d, bits=%d, speed=%u Hz, with dual-lock protection)",
             config->device, impl->fd, impl->mode, impl->bits_per_word, impl->max_speed_hz);
    return OSAL_SUCCESS;
}

/**
 * @brief 关闭SPI设备
 */
int32_t HAL_SPI_Close(hal_spi_handle_t handle)
{
    hal_spi_context_t *impl = (hal_spi_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized)
        return OSAL_ERR_GENERIC;

    if (impl->fd >= 0)
    {
        OSAL_close(impl->fd);
        impl->fd = -1;
    }

    /* 销毁锁 */
    if (impl->mutex)
    {
        OSAL_MutexDelete(impl->mutex);
    }

    if (impl->flock)
    {
        OSAL_FlockDestroy(impl->flock);
    }

    impl->initialized = false;
    OSAL_Free(impl);

    LOG_INFO("HAL_SPI", "Device closed");
    return OSAL_SUCCESS;
}

/**
 * @brief SPI写操作
 */
int32_t HAL_SPI_Write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size)
{
    hal_spi_context_t *impl = (hal_spi_context_t *)handle;
    int32_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_SPI_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：写入数据 */
    ret = OSAL_write(impl->fd, buffer, size);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Write failed: %s %d (%d)",
                  OSAL_StrError(err), err);
        result = err;
    }
    else if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_SPI", "Partial write: %d/%u bytes", ret, size);
        result = OSAL_ERR_GENERIC;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

/**
 * @brief SPI读操作
 */
int32_t HAL_SPI_Read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size)
{
    hal_spi_context_t *impl = (hal_spi_context_t *)handle;
    int32_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_SPI_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：读取数据 */
    ret = OSAL_read(impl->fd, buffer, size);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Read failed: %s %d (%d)",
                  OSAL_StrError(err), err);
        result = err;
    }
    else if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_SPI", "Partial read: %d/%u bytes", ret, size);
        result = OSAL_ERR_GENERIC;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

/**
 * @brief SPI全双工传输
 */
int32_t HAL_SPI_Transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
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

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_SPI_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：构造传输结构 (参考: Linux内核 spidev.h) */
    OSAL_Memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf = (uintptr_t)tx_buffer;
    xfer.rx_buf = (uintptr_t)rx_buffer;
    xfer.len = size;
    xfer.speed_hz = impl->max_speed_hz;
    xfer.bits_per_word = impl->bits_per_word;
    xfer.delay_usecs = 0;
    xfer.cs_change = 0;

    /* 执行传输 */
    ret = OSAL_ioctl(impl->fd, SPI_IOC_MESSAGE(1), &xfer);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Transfer failed: %s %d (%d)",
                  OSAL_StrError(err), err);
        result = err;
    }
    else if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_SPI", "Partial transfer: %d/%u bytes", ret, size);
        result = OSAL_ERR_GENERIC;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

/**
 * @brief SPI批量传输
 */
int32_t HAL_SPI_TransferMulti(hal_spi_handle_t handle, hal_spi_transfer_t *transfers, uint32_t num)
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
    xfers = (struct spi_ioc_transfer *)OSAL_Malloc(sizeof(struct spi_ioc_transfer) * num);
    if (NULL == xfers)
    {
        LOG_ERROR("HAL_SPI", "Failed to allocate transfer buffer");
        return OSAL_ERR_GENERIC;
    }

    /* 转换传输格式 */
    OSAL_Memset(xfers, 0, sizeof(struct spi_ioc_transfer) * num);
    for (i = 0; i < num; i++)
    {
        xfers[i].tx_buf = (uintptr_t)transfers[i].tx_buf;
        xfers[i].rx_buf = (uintptr_t)transfers[i].rx_buf;
        xfers[i].len = transfers[i].len;
        xfers[i].speed_hz = (transfers[i].speed_hz != 0) ? transfers[i].speed_hz : impl->max_speed_hz;
        xfers[i].bits_per_word = (transfers[i].bits_per_word != 0) ? transfers[i].bits_per_word : impl->bits_per_word;
        xfers[i].delay_usecs = transfers[i].delay_usecs;
        xfers[i].cs_change = transfers[i].cs_change;
    }

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_SPI_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire file lock (timeout or error)");
        OSAL_Free(xfers);
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        OSAL_Free(xfers);
        return ret;
    }

    /* 临界区：执行批量传输 */
    ret = OSAL_ioctl(impl->fd, SPI_IOC_MESSAGE(num), xfers);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_SPI", "Multi-transfer failed: %s %d (%d)",
                  OSAL_StrError(err), err);
        result = err;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    OSAL_Free(xfers);
    return result;
}

/**
 * @brief 设置SPI配置
 */
int32_t HAL_SPI_SetConfig(hal_spi_handle_t handle, const hal_spi_config_t *config)
{
    hal_spi_context_t *impl = (hal_spi_context_t *)handle;
    int32_t ret;
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == config)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized)
        return OSAL_ERR_GENERIC;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_SPI_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_SPI", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：更新配置 */
    /* 更新模式 */
    if (config->mode != impl->mode)
    {
        uint8_t mode = config->mode;
        ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_MODE, &mode);
        if (ret < 0)
        {
            int32_t err = OSAL_GetErrno();
            LOG_ERROR("HAL_SPI", "Failed to update mode: %s %d (%d)",
                      OSAL_StrError(err), err);
            result = err;
            goto unlock;
        }
        impl->mode = config->mode;
    }

    /* 更新每字位数 */
    if (config->bits_per_word != impl->bits_per_word)
    {
        uint8_t bits = config->bits_per_word;
        ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
        if (ret < 0)
        {
            int32_t err = OSAL_GetErrno();
            LOG_ERROR("HAL_SPI", "Failed to update bits per word: %s %d (%d)",
                      OSAL_StrError(err), err);
            result = err;
            goto unlock;
        }
        impl->bits_per_word = config->bits_per_word;
    }

    /* 更新速率 */
    if (config->max_speed_hz != impl->max_speed_hz)
    {
        uint32_t speed = config->max_speed_hz;
        ret = OSAL_ioctl(impl->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (ret < 0)
        {
            int32_t err = OSAL_GetErrno();
            LOG_ERROR("HAL_SPI", "Failed to update max speed: %s %d (%d)",
                      OSAL_StrError(err), err);
            result = err;
            goto unlock;
        }
        impl->max_speed_hz = config->max_speed_hz;
    }

    impl->timeout = config->timeout;

    if (result == OSAL_SUCCESS)
    {
        LOG_INFO("HAL_SPI", "Configuration updated");
    }

unlock:
    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}
