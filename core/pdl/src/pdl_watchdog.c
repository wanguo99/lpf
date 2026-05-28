/************************************************************************
 * PDL层 - Watchdog服务实现
 ************************************************************************/

#include "pdl/pdl_watchdog.h"
#include "hal/hal_watchdog.h"
#include "osal/osal.h"

typedef struct
{
    char name[64];
    hal_watchdog_handle_t hal_handle;
    watchdog_mode_t mode;
    uint32_t kick_interval_ms;
    bool enabled;

    /* 自动模式相关 */
    osal_thread_t kick_thread;
    volatile bool running;
    osal_atomic_uint32_t kick_count;
    osal_atomic_uint64_t last_kick_time;  /* 上次喂狗时间戳（微秒） */
} watchdog_context_t;

/**
 * @brief 自动喂狗线程
 */
static void *watchdog_kick_thread(void *arg)
{
    watchdog_context_t *ctx = (watchdog_context_t *)arg;

    LOG_INFO("PDL_WDT", "Auto-kick thread started for %s", ctx->name);

    while (ctx->running)
    {
        /* 喂狗 */
        int32_t ret = HAL_WATCHDOG_Kick(ctx->hal_handle);
        if (ret == OSAL_SUCCESS)
        {
            OSAL_AtomicFetchAdd(&ctx->kick_count, 1);
            OSAL_AtomicStore64(&ctx->last_kick_time, OSAL_GetMonotonicTime());
            LOG_DEBUG("PDL_WDT", "[%s] Kicked (count=%u)",
                     ctx->name, OSAL_AtomicLoad(&ctx->kick_count));
        }
        else
        {
            LOG_ERROR("PDL_WDT", "[%s] Kick failed: %d", ctx->name, ret);
        }

        /* 延时 */
        OSAL_msleep(ctx->kick_interval_ms);
    }

    LOG_INFO("PDL_WDT", "Auto-kick thread stopped for %s", ctx->name);
    return NULL;
}

/**
 * @brief 初始化Watchdog服务
 */
int32_t PDL_WATCHDOG_Init(const watchdog_config_t *config, watchdog_handle_t *handle)
{
    watchdog_context_t *ctx;
    hal_watchdog_config_t hal_config;
    int32_t ret;

    if (config == NULL || handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid parameters");
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 分配上下文 */
    ctx = (watchdog_context_t *)OSAL_Malloc(sizeof(watchdog_context_t));
    if (ctx == NULL)
    {
        LOG_ERROR("PDL_WDT", "Failed to allocate context");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(watchdog_context_t));
    OSAL_Strncpy(ctx->name, config->name, sizeof(ctx->name) - 1);
    ctx->mode = config->mode;
    ctx->kick_interval_ms = config->kick_interval_ms;
    ctx->enabled = false;
    ctx->running = false;
    OSAL_AtomicInit(&ctx->kick_count, 0);
    OSAL_AtomicInit64(&ctx->last_kick_time, 0);

    /* 初始化HAL层 */
    hal_config.device = config->device;
    hal_config.timeout_sec = config->timeout_sec;
    hal_config.enable_on_init = config->enable_on_init;

    ret = HAL_WATCHDOG_Init(&hal_config, &ctx->hal_handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_WDT", "HAL_WATCHDOG_Init failed: %d", ret);
        OSAL_Free(ctx);
        return ret;
    }

    ctx->enabled = config->enable_on_init;

    LOG_INFO("PDL_WDT", "Watchdog service initialized: %s (mode=%s, interval=%ums)",
             ctx->name,
             ctx->mode == WATCHDOG_MODE_AUTO ? "auto" : "manual",
             ctx->kick_interval_ms);

    *handle = (watchdog_handle_t)ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 关闭Watchdog服务
 */
int32_t PDL_WATCHDOG_Deinit(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    /* 停止自动喂狗线程 */
    if (ctx->running)
    {
        PDL_WATCHDOG_Stop(handle);
    }

    /* 关闭HAL层 */
    HAL_WATCHDOG_Deinit(ctx->hal_handle);

    LOG_INFO("PDL_WDT", "Watchdog service deinitialized: %s (kicked %u times)",
             ctx->name, OSAL_AtomicLoad(&ctx->kick_count));

    OSAL_Free(ctx);
    return OSAL_SUCCESS;
}

/**
 * @brief 启动自动喂狗服务
 */
int32_t PDL_WATCHDOG_Start(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (ctx->mode != WATCHDOG_MODE_AUTO)
    {
        LOG_WARN("PDL_WDT", "[%s] Not in auto mode, cannot start", ctx->name);
        return OSAL_ERR_GENERIC;
    }

    if (ctx->running)
    {
        LOG_WARN("PDL_WDT", "[%s] Already running", ctx->name);
        return OSAL_SUCCESS;
    }

    /* 启动喂狗线程 */
    ctx->running = true;
    ret = OSAL_ThreadCreate(&ctx->kick_thread, watchdog_kick_thread, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_WDT", "[%s] Failed to create kick thread", ctx->name);
        ctx->running = false;
        return ret;
    }

    LOG_INFO("PDL_WDT", "[%s] Auto-kick service started", ctx->name);
    return OSAL_SUCCESS;
}

/**
 * @brief 停止自动喂狗服务
 */
int32_t PDL_WATCHDOG_Stop(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (!ctx->running)
    {
        return OSAL_SUCCESS;
    }

    /* 停止线程 */
    ctx->running = false;
    OSAL_ThreadJoin(ctx->kick_thread);

    LOG_INFO("PDL_WDT", "[%s] Auto-kick service stopped", ctx->name);
    return OSAL_SUCCESS;
}

/**
 * @brief 手动喂狗
 */
int32_t PDL_WATCHDOG_Kick(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_Kick(ctx->hal_handle);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_AtomicFetchAdd(&ctx->kick_count, 1);
        OSAL_AtomicStore64(&ctx->last_kick_time, OSAL_GetMonotonicTime());
    }

    return ret;
}

/**
 * @brief 获取Watchdog状态
 */
int32_t PDL_WATCHDOG_GetStatus(watchdog_handle_t handle, watchdog_status_t *status)
{
    watchdog_context_t *ctx;
    uint32_t timeout;

    if (handle == NULL || status == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid parameters");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    OSAL_Memset(status, 0, sizeof(watchdog_status_t));
    status->enabled = ctx->enabled;
    status->running = ctx->running;
    status->kick_interval_ms = ctx->kick_interval_ms;
    status->mode = ctx->mode;
    status->kick_count = OSAL_AtomicLoad(&ctx->kick_count);
    status->last_kick_time_us = OSAL_AtomicLoad64(&ctx->last_kick_time);

    /* 获取HAL层超时时间 */
    timeout = 0;
    if (HAL_WATCHDOG_GetTimeout(ctx->hal_handle, &timeout) == OSAL_SUCCESS)
    {
        status->timeout_sec = timeout;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 设置喂狗间隔
 */
int32_t PDL_WATCHDOG_SetInterval(watchdog_handle_t handle, uint32_t interval_ms)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (ctx->mode != WATCHDOG_MODE_AUTO)
    {
        LOG_WARN("PDL_WDT", "[%s] Not in auto mode", ctx->name);
        return OSAL_ERR_GENERIC;
    }

    ctx->kick_interval_ms = interval_ms;
    LOG_INFO("PDL_WDT", "[%s] Kick interval set to %u ms", ctx->name, interval_ms);

    return OSAL_SUCCESS;
}

/**
 * @brief 启用看门狗
 */
int32_t PDL_WATCHDOG_Enable(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_Enable(ctx->hal_handle);
    if (ret == OSAL_SUCCESS)
    {
        ctx->enabled = true;
        LOG_INFO("PDL_WDT", "[%s] Enabled", ctx->name);
    }

    return ret;
}

/**
 * @brief 禁用看门狗
 */
int32_t PDL_WATCHDOG_Disable(watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_Disable(ctx->hal_handle);
    if (ret == OSAL_SUCCESS)
    {
        ctx->enabled = false;
        LOG_INFO("PDL_WDT", "[%s] Disabled", ctx->name);
    }

    return ret;
}
