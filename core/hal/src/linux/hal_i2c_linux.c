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
#include "hal_i2c.h"
#include "hal_i2c_internal.h"
#include "hal/hal_error_api.h"
#include "osal.h"
#include "osal_flock.h"

/**
 * @brief 打开I2C设备
 */
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle)
{
    hal_i2c_context_t *impl;
    int32_t ret;

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

    /* 创建文件锁（进程间保护） */
    char lock_file[OSAL_LOCK_PATH_MAX_LEN];
    /* 从设备路径提取总线号，例如 /dev/i2c-0 -> 0 */
    const char *dev_name = config->device;
    const char *slash = config->device;
    while (*slash)
    {
        if (*slash == '/')
            dev_name = slash + 1;
        slash++;
    }
    /* 提取数字部分，例如 i2c-0 -> 0 */
    int bus_num = 0;
    if (OSAL_Sscanf(dev_name, "i2c-%d", &bus_num) == 1)
    {
        OSAL_Snprintf(lock_file, sizeof(lock_file), HAL_I2C_LOCK_PATH_FMT, bus_num);
    }
    else
    {
        /* 如果无法解析，使用设备名 */
        OSAL_Snprintf(lock_file, sizeof(lock_file), OSAL_LOCK_DIR "/hal_i2c_%s.lock", dev_name);
    }

    ret = OSAL_FlockCreate(lock_file, &impl->flock);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to create file lock: %s", lock_file);
        OSAL_Free(impl);
        return ret;
    }

    /* 创建互斥锁（线程间保护） */
    ret = OSAL_MutexCreate(&impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to create mutex");
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return ret;
    }

    /* 打开I2C设备（不使用 O_EXCL，由文件锁保护） */
    impl->fd = OSAL_open(config->device, OSAL_O_RDWR, 0);
    if (impl->fd < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to open device %s: %s",
                      config->device, OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Failed to open device %s: %s %d(sys_err=%d, hal_err=%d)",
                  config->device, OSAL_StrError(err), err, hal_err);
        OSAL_MutexDelete(impl->mutex);
        OSAL_FlockDestroy(impl->flock);
        OSAL_Free(impl);
        return hal_err;
    }

    impl->initialized = true;
    *handle = (hal_i2c_handle_t)impl;

    LOG_INFO("HAL_I2C", "Opened device %s (fd=%d, with dual-lock protection)", config->device, impl->fd);
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

    /* 销毁锁 */
    if (impl->mutex)
    {
        OSAL_MutexDelete(impl->mutex);
    }

    if (impl->flock)
    {
        OSAL_FlockDestroy(impl->flock);
    }

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
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_I2C_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：硬件访问 */
    /* 设置从设备地址 (参考: Linux内核文档 i2c-dev.txt) */
    ret = OSAL_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to set slave address 0x%02X: %s",
                      slave_addr, OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s %d(sys_err=%d, hal_err=%d)",
                  slave_addr, OSAL_StrError(err), err, hal_err);
        result = hal_err;
        goto unlock;
    }

    /* 写入数据 */
    ret = OSAL_write(impl->fd, buffer, size);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Write failed: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Write failed: %s %d(sys_err=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
        goto unlock;
    }

    if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_I2C", "Partial write: %d/%u bytes", ret, size);
        result = OSAL_ERR_GENERIC;
    }

unlock:
    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

/**
 * @brief 从I2C从设备读取数据
 */
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr,
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
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_I2C_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：硬件访问 */
    /* 设置从设备地址 */
    ret = OSAL_ioctl(impl->fd, I2C_SLAVE, (void *)(uintptr_t)slave_addr);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Failed to set slave address 0x%02X: %s",
                      slave_addr, OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Failed to set slave address 0x%02X: %s %d(sys_err=%d, hal_err=%d)",
                  slave_addr, OSAL_StrError(err), err, hal_err);
        result = hal_err;
        goto unlock;
    }

    /* 读取数据 */
    ret = OSAL_read(impl->fd, buffer, size);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Read failed: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Read failed: %s %d(sys_err=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
        goto unlock;
    }

    if ((uint32_t)ret != size)
    {
        LOG_ERROR("HAL_I2C", "Partial read: %d/%u bytes", ret, size);
        result = OSAL_ERR_GENERIC;
    }

unlock:
    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
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
    int32_t result = OSAL_SUCCESS;

    if (NULL == impl || NULL == buffer)
        return OSAL_ERR_INVALID_POINTER;

    if (!impl->initialized || size == 0)
        return OSAL_ERR_GENERIC;

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_I2C_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        return ret;
    }

    /* 临界区：硬件访问 */
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
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Read register failed: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Read register failed: %s %d(sys_err=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    return result;
}

/**
 * @brief 执行I2C传输
 */
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, hal_i2c_msg_t *msgs, uint32_t num)
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

    /* 第一层：文件锁（进程间保护） */
    ret = OSAL_FlockTimedLock(impl->flock, OSAL_FLOCK_EXCLUSIVE, HAL_I2C_LOCK_TIMEOUT_MS);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire file lock (timeout or error)");
        OSAL_Free(kernel_msgs);
        return ret;
    }

    /* 第二层：互斥锁（线程间保护） */
    ret = OSAL_MutexLock(impl->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("HAL_I2C", "Failed to acquire mutex");
        OSAL_FlockUnlock(impl->flock);
        OSAL_Free(kernel_msgs);
        return ret;
    }

    /* 临界区：硬件访问 */
    msgset.msgs = kernel_msgs;
    msgset.nmsgs = num;

    /* 执行传输 */
    ret = OSAL_ioctl(impl->fd, I2C_RDWR, &msgset);
    if (ret < 0)
    {
        int32_t err = OSAL_GetErrno();
        int32_t hal_err = HAL_ErrnoToError(err);
        HAL_SET_ERROR(hal_err, err, "Transfer failed: %s", OSAL_StrError(err));
        LOG_ERROR("HAL_I2C", "Transfer failed: %s %d(sys_err=%d, hal_err=%d)",
                  OSAL_StrError(err), err, hal_err);
        result = hal_err;
    }

    /* 释放锁（逆序） */
    OSAL_MutexUnlock(impl->mutex);
    OSAL_FlockUnlock(impl->flock);

    OSAL_Free(kernel_msgs);
    return result;
}
