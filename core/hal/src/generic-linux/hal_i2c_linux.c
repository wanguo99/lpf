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
#include <pthread.h>
#include "hal_i2c.h"
#include "osal.h"

typedef struct
{
    int32_t fd;
    char device[64];
    uint32_t timeout;
    bool initialized;
    pthread_mutex_t lock;  /* 操作级别互斥锁 */
} hal_i2c_context_t;

/**
 * @brief 打开I2C设备
 */
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle)
{
    hal_i2c_context_t *impl;

    /* 参数检查 */
    if (NULL == config || NULL == handle)
        return OSAL_ERR_INVALID_POINTER;

    if (NULL == config->device || 0 == OSAL_Strlen(config->device))
        return OSAL_ERR_GENERIC;

    /* 分配句柄 */
    impl = (hal_i2c_context_t *)OSAL_Malloc(sizeof(hal_i2c_context_t));
    if (NULL == impl)
    {
        LOG_ERROR("HAL_I2C", "Failed to allocate memory");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(impl, 0, sizeof(hal_i2c_context_t));
    OSAL_Strncpy(impl->device, config->device, sizeof(impl->device) - 1);
    impl->device[sizeof(impl->device) - 1] = '\0';
    impl->timeout = config->timeout;
    impl->initialized = false;

    /* 初始化互斥锁，检查返回值 */
    if (0 != pthread_mutex_init(&impl->lock, NULL))
    {
        LOG_ERROR("HAL_I2C", "Failed to initialize mutex");
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    /* 打开I2C设备（O_EXCL 保证独占访问，防止多进程竞争） */
    impl->fd = OSAL_open(config->device, OSAL_O_RDWR | OSAL_O_EXCL, 0);
    if (impl->fd < 0)
    {
        int32_t err = OSAL_GetErrno();
        if (OSAL_EBUSY == err)
        {
            LOG_ERROR("HAL_I2C", "Device %s is busy (already opened by another process)", config->device);
            OSAL_Free(impl);
            return OSAL_ERR_BUSY;
        }
        LOG_ERROR("HAL_I2C", "Failed to open device %s: %s",
                  config->device, OSAL_StrError(err));
        OSAL_Free(impl);
        return OSAL_ERR_GENERIC;
    }

    impl->initialized = true;
    *handle = (hal_i2c_handle_t)impl;

    LOG_INFO("HAL_I2C", "Opened device %s (fd=%d)", config->device, impl->fd);
    return OSAL_SUCCESS;
}

/**
 * @brief 关闭I2C设备
 */
int32_t HAL_I2C_Close(hal_i2c_handle_t handle)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;

    if (NULL == impl)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized)
        return OSAL_ERR_GENERIC;

    if (impl->fd >= 0)
    {
        OSAL_close(impl->fd);
        impl->fd = -1;
    }

    impl->initialized = false;
    pthread_mutex_destroy(&impl->lock);
    OSAL_Free(impl);

    LOG_INFO("HAL_I2C", "Device closed");
    return OSAL_SUCCESS;
}

/**
 * @brief 写入数据到I2C从设备
 */
int32_t HAL_I2C_Write(hal_i2c_handle_t handle, uint16_t slave_addr,
                      const uint8_t *buffer, uint32_t size)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
    int32_t ret;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    pthread_mutex_lock(&impl->lock);

    /* 设置从设备地址 (参考: Linux内核文档 i2c-dev.txt) */
    ret = OSAL_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s",
                  slave_addr, OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    /* 写入数据 */
    ret = OSAL_write(impl->fd, buffer, size);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Write failed: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_I2C", "Partial write: %d/%u bytes", ret, size);
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    pthread_mutex_unlock(&impl->lock);
    return OSAL_SUCCESS;
}

/**
 * @brief 从I2C从设备读取数据
 */
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr,
                     uint8_t *buffer, uint32_t size)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
    int32_t ret;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    pthread_mutex_lock(&impl->lock);

    /* 设置从设备地址 */
    ret = OSAL_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s",
                  slave_addr, OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    /* 读取数据 */
    ret = OSAL_read(impl->fd, buffer, size);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Read failed: %s", OSAL_StrError(OSAL_GetErrno()));
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_I2C", "Partial read: %d/%u bytes", ret, size);
        pthread_mutex_unlock(&impl->lock);
        return OSAL_ERR_GENERIC;
    }

    pthread_mutex_unlock(&impl->lock);
    return OSAL_SUCCESS;
}

/**
 * @brief 写寄存器操作
 */
int32_t HAL_I2C_WriteReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                         uint8_t reg_addr, const uint8_t *buffer, uint32_t size)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
    uint8_t *write_buf;
    int32_t ret;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 分配临时缓冲区: [寄存器地址][数据] */
    write_buf = (uint8_t *)OSAL_Malloc(size + 1);
    if (NULL == write_buf)
    {
        LOG_ERROR("HAL_I2C", "Failed to allocate write buffer");
        return OSAL_ERR_GENERIC;
    }

    write_buf[0] = reg_addr;
    OSAL_Memcpy(&write_buf[1], buffer, size);

    /* 执行写操作 */
    ret = HAL_I2C_Write(handle, slave_addr, write_buf, size + 1);

    OSAL_Free(write_buf);
    return ret;
}

/**
 * @brief 读寄存器操作
 */
int32_t HAL_I2C_ReadReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                        uint8_t reg_addr, uint8_t *buffer, uint32_t size)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset;
    int32_t ret;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 组合传输: 先写寄存器地址，再读数据 (参考: Linux内核 i2c.h) */
    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;              /* 写操作 */
    msgs[0].len = 1;
    msgs[0].buf = &reg_addr;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD;       /* 读操作 */
    msgs[1].len = size;
    msgs[1].buf = buffer;

    msgset.msgs = msgs;
    msgset.nmsgs = 2;

    ret = OSAL_ioctl(impl->fd, I2C_RDWR, &msgset);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Read register failed: %s", OSAL_StrError(OSAL_GetErrno()));
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 执行I2C传输
 */
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, i2c_msg_t *msgs, uint32_t num)
{
    hal_i2c_context_t *impl = (hal_i2c_context_t *)handle;
    struct i2c_msg *kernel_msgs;
    struct i2c_rdwr_ioctl_data msgset;
    int32_t ret;
    uint32_t i;

    if (NULL == impl || NULL == msgs)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || num == 0)
        return OSAL_ERR_GENERIC;

    /* 分配内核消息结构 */
    kernel_msgs = (struct i2c_msg *)OSAL_Malloc(sizeof(struct i2c_msg) * num);
    if (NULL == kernel_msgs)
    {
        LOG_ERROR("HAL_I2C", "Failed to allocate message buffer");
        return OSAL_ERR_GENERIC;
    }

    /* 转换消息格式 */
    for (i = 0; i < num; i++)
    {
        kernel_msgs[i].addr = msgs[i].addr;
        kernel_msgs[i].flags = msgs[i].flags;
        kernel_msgs[i].len = msgs[i].len;
        kernel_msgs[i].buf = msgs[i].buf;
    }

    msgset.msgs = kernel_msgs;
    msgset.nmsgs = num;

    /* 执行传输 */
    ret = OSAL_ioctl(impl->fd, I2C_RDWR, &msgset);
    if (ret < 0)
    {
        LOG_ERROR("HAL_I2C", "Transfer failed: %s", OSAL_StrError(OSAL_GetErrno()));
        OSAL_Free(kernel_msgs);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Free(kernel_msgs);
    return OSAL_SUCCESS;
}
