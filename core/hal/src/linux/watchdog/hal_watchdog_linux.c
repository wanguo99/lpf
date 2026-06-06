/************************************************************************
 * HAL层 - Watchdog驱动Linux实现
 ************************************************************************/

/* 必须在所有头文件之前定义，以启用完整的POSIX功能 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include "hal.h"
#include "hal_watchdog_internal.h"
#include "osal.h"

/**
 * @brief 初始化Watchdog驱动
 */
int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle)
{
    hal_watchdog_context_t *ctx;
    int32_t timeout;

    if (config == NULL || handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid parameters");
        return OSAL_ERR_INVALID_PARAM;
    }

    if (config->device == NULL)
    {
        LOG_ERROR("HAL_WDT", "Device path is NULL");
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 分配上下文 */
    ctx = (hal_watchdog_context_t *)OSAL_Malloc(sizeof(hal_watchdog_context_t));
    if (ctx == NULL)
    {
        LOG_ERROR("HAL_WDT", "Failed to allocate context");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(hal_watchdog_context_t));
    OSAL_Strncpy(ctx->device, config->device, sizeof(ctx->device) - 1);
    ctx->timeout_sec = config->timeout_sec;
    atomic_init(&ctx->kick_count, 0);

    /* 打开看门狗设备 */
    ctx->fd = OSAL_open(config->device, OSAL_O_WRONLY, 0);
    if (ctx->fd < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_WDT", "Failed to open %s: %s (%d)",
                  config->device, OSAL_StrError(err), err);
        OSAL_Free(ctx);
        return err;
    }

    LOG_INFO("HAL_WDT", "Opened watchdog device: %s (fd=%d)", config->device, ctx->fd);

    /* 设置超时时间 */
    if (config->timeout_sec > 0)
    {
        timeout = (int32_t)config->timeout_sec;
        if (OSAL_ioctl(ctx->fd, WDIOC_SETTIMEOUT, &timeout) < 0)
        {
            int32_t err = OSAL_GetErrno();
            LOG_ERROR("HAL_WDT", "Failed to set timeout: %s (%d)",
                      OSAL_StrError(err), err);
            OSAL_close(ctx->fd);
            OSAL_Free(ctx);
            return err;
        }
        ctx->timeout_sec = (uint32_t)timeout;
        LOG_INFO("HAL_WDT", "Set timeout to %u seconds", ctx->timeout_sec);
    }
    else
    {
        /* 获取默认超时时间 */
        timeout = 0;
        if (OSAL_ioctl(ctx->fd, WDIOC_GETTIMEOUT, &timeout) == 0)
        {
            ctx->timeout_sec = (uint32_t)timeout;
            LOG_INFO("HAL_WDT", "Using default timeout: %u seconds", ctx->timeout_sec);
        }
    }

    ctx->enabled = config->enable_on_init;

    *handle = (hal_watchdog_handle_t)ctx;
    LOG_INFO("HAL_WDT", "Watchdog initialized successfully");

    return OSAL_SUCCESS;
}

/**
 * @brief 关闭Watchdog驱动
 */
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle)
{
    hal_watchdog_context_t *ctx;
    const char magic = 'V';
    osal_ssize_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_PARAM;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd >= 0)
    {
        /* 写入魔术字符'V'来禁用看门狗（如果硬件支持） */
        ret = OSAL_write(ctx->fd, &magic, 1);
        if (ret < 0)
        {
            LOG_WARN("HAL_WDT", "Failed to disable watchdog (may not be supported)");
        }
        else
        {
            LOG_INFO("HAL_WDT", "Watchdog disabled");
        }

        OSAL_close(ctx->fd);
        LOG_INFO("HAL_WDT", "Closed watchdog device (kicked %u times)",
                 atomic_load(&ctx->kick_count));
    }

    OSAL_Free(ctx);
    return OSAL_SUCCESS;
}

/**
 * @brief 喂狗（重置看门狗定时器）
 */
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle)
{
    hal_watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_PARAM;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    /* 使用ioctl喂狗 */
    if (OSAL_ioctl(ctx->fd, WDIOC_KEEPALIVE, 0) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_WDT", "Failed to kick watchdog: %s (%d)",
                  OSAL_StrError(err), err);
        return err;
    }

    atomic_fetch_add(&ctx->kick_count, 1);
    return OSAL_SUCCESS;
}

/**
 * @brief 启用看门狗
 */
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle)
{
    hal_watchdog_context_t *ctx;
    int32_t options;

    if (handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_PARAM;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    options = WDIOS_ENABLECARD;
    if (OSAL_ioctl(ctx->fd, WDIOC_SETOPTIONS, &options) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_WDT", "Failed to enable watchdog: %s (%d)",
                  OSAL_StrError(err), err);
        return err;
    }

    ctx->enabled = true;
    LOG_INFO("HAL_WDT", "Watchdog enabled");
    return OSAL_SUCCESS;
}

/**
 * @brief 禁用看门狗
 */
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle)
{
    hal_watchdog_context_t *ctx;
    int32_t options;

    if (handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_PARAM;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    options = WDIOS_DISABLECARD;
    if (OSAL_ioctl(ctx->fd, WDIOC_SETOPTIONS, &options) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_WARN("HAL_WDT", "Failed to disable watchdog (may not be supported): %s (%d)",
                 OSAL_StrError(err), err);
        return err;
    }

    ctx->enabled = false;
    LOG_INFO("HAL_WDT", "Watchdog disabled");
    return OSAL_SUCCESS;
}

/**
 * @brief 设置看门狗超时时间
 */
int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec)
{
    hal_watchdog_context_t *ctx;
    int32_t timeout;

    if (handle == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_PARAM;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    timeout = (int32_t)timeout_sec;
    if (OSAL_ioctl(ctx->fd, WDIOC_SETTIMEOUT, &timeout) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_WDT", "Failed to set timeout: %s (%d)",
                  OSAL_StrError(err), err);
        return err;
    }

    ctx->timeout_sec = (uint32_t)timeout;
    LOG_INFO("HAL_WDT", "Timeout set to %u seconds", ctx->timeout_sec);
    return OSAL_SUCCESS;
}

/**
 * @brief 获取看门狗超时时间
 */
int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec)
{
    hal_watchdog_context_t *ctx;
    int32_t timeout;

    if (handle == NULL || timeout_sec == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid parameters");
        return OSAL_EINVAL;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    timeout = 0;
    if (OSAL_ioctl(ctx->fd, WDIOC_GETTIMEOUT, &timeout) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_ERROR("HAL_WDT", "Failed to get timeout: %s (%d)",
                  OSAL_StrError(err), err);
        return err;
    }

    *timeout_sec = (uint32_t)timeout;
    return OSAL_SUCCESS;
}

/**
 * @brief 获取看门狗剩余时间
 */
int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec)
{
    hal_watchdog_context_t *ctx;
    int32_t timeleft;

    if (handle == NULL || timeleft_sec == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid parameters");
        return OSAL_EINVAL;
    }

    ctx = (hal_watchdog_context_t *)handle;

    if (ctx->fd < 0)
    {
        LOG_ERROR("HAL_WDT", "Watchdog not initialized");
        return OSAL_ERR_GENERIC;
    }

    timeleft = 0;
    if (OSAL_ioctl(ctx->fd, WDIOC_GETTIMELEFT, &timeleft) < 0)
    {
        int32_t err = OSAL_GetErrno();
        LOG_WARN("HAL_WDT", "Failed to get timeleft (may not be supported): %s (%d)",
                 OSAL_StrError(err), err);
        return err;
    }

    *timeleft_sec = (uint32_t)timeleft;
    return OSAL_SUCCESS;
}

/**
 * @brief 获取看门狗统计信息
 */
int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count)
{
    hal_watchdog_context_t *ctx;

    if (handle == NULL || kick_count == NULL)
    {
        LOG_ERROR("HAL_WDT", "Invalid parameters");
        return OSAL_EINVAL;
    }

    ctx = (hal_watchdog_context_t *)handle;
    *kick_count = atomic_load(&ctx->kick_count);
    return OSAL_SUCCESS;
}
