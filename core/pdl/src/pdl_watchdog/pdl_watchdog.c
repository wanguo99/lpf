/************************************************************************
 * PDL层 - Watchdog服务实现
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pdl_watchdog.h"

typedef struct
{
    char name[64];
    hal_watchdog_handle_t hal_handle;
    pdl_watchdog_mode_t mode;
    uint32_t kick_interval_ms;
    bool enabled;

    /* 自动模式相关 */
    osal_thread_t kick_thread;
    osal_atomic_bool_t running;       /* 使用原子变量保证多线程安全 */
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

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 喂狗 */
        int32_t ret = HAL_WATCHDOG_kick(ctx->hal_handle);
        if (ret == OSAL_SUCCESS)
        {
            OSAL_atomic_fetch_add(&ctx->kick_count, 1);
            OSAL_atomic_store_u64(&ctx->last_kick_time, OSAL_get_monotonic_time());
            LOG_DEBUG("PDL_WDT", "[%s] Kicked (count=%u)",
                     ctx->name, OSAL_atomic_load(&ctx->kick_count));
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
int32_t PDL_WATCHDOG_init(const pdl_watchdog_config_t *config, pdl_watchdog_handle_t *handle)
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
    ctx = (watchdog_context_t *)OSAL_malloc(OSAL_sizeof(watchdog_context_t));
    if (ctx == NULL)
    {
        LOG_ERROR("PDL_WDT", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(watchdog_context_t));
    OSAL_strncpy(ctx->name, config->name, OSAL_sizeof(ctx->name) - 1);
    ctx->mode = config->mode;
    ctx->kick_interval_ms = config->kick_interval_ms;
    ctx->enabled = false;
    OSAL_atomic_init_bool(&ctx->running, false);
    OSAL_atomic_init(&ctx->kick_count, 0);
    OSAL_atomic_init_u64(&ctx->last_kick_time, 0);

    /* 初始化HAL层 */
    hal_config.device = config->device;
    hal_config.timeout_sec = config->timeout_sec;
    hal_config.enable_on_init = config->enable_on_init;

    ret = HAL_WATCHDOG_init(&hal_config, &ctx->hal_handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_WDT", "HAL_WATCHDOG_init failed: %d", ret);
        OSAL_free(ctx);
        return ret;
    }

    ctx->enabled = config->enable_on_init;

    LOG_INFO("PDL_WDT", "Watchdog service initialized: %s (mode=%s, interval=%ums)",
             ctx->name,
             ctx->mode == PDL_WATCHDOG_MODE_AUTO ? "auto" : "manual",
             ctx->kick_interval_ms);

    *handle = (pdl_watchdog_handle_t)ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 关闭Watchdog服务
 */
int32_t PDL_WATCHDOG_deinit(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    /* 停止自动喂狗线程 */
    if (OSAL_atomic_load_bool(&ctx->running))
    {
        PDL_WATCHDOG_stop(handle);
    }

    /* 关闭HAL层 */
    HAL_WATCHDOG_deinit(ctx->hal_handle);

    LOG_INFO("PDL_WDT", "Watchdog service deinitialized: %s (kicked %u times)",
             ctx->name, OSAL_atomic_load(&ctx->kick_count));

    OSAL_free(ctx);
    return OSAL_SUCCESS;
}

/**
 * @brief 启动自动喂狗服务
 */
int32_t PDL_WATCHDOG_start(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (ctx->mode != PDL_WATCHDOG_MODE_AUTO)
    {
        LOG_WARN("PDL_WDT", "[%s] Not in auto mode, cannot start", ctx->name);
        return OSAL_ERR_GENERIC;
    }

    if (OSAL_atomic_load_bool(&ctx->running))
    {
        LOG_WARN("PDL_WDT", "[%s] Already running", ctx->name);
        return OSAL_SUCCESS;
    }

    /* 启动喂狗线程 */
    OSAL_atomic_store_bool(&ctx->running, true);
    ret = OSAL_pthread_create(&ctx->kick_thread, NULL, watchdog_kick_thread, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_WDT", "[%s] Failed to create kick thread", ctx->name);
        OSAL_atomic_store_bool(&ctx->running, false);
        return ret;
    }

    LOG_INFO("PDL_WDT", "[%s] Auto-kick service started", ctx->name);
    return OSAL_SUCCESS;
}

/**
 * @brief 停止自动喂狗服务
 */
int32_t PDL_WATCHDOG_stop(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (!OSAL_atomic_load_bool(&ctx->running))
    {
        return OSAL_SUCCESS;
    }

    /* 停止线程 */
    OSAL_atomic_store_bool(&ctx->running, false);
    OSAL_pthread_join(ctx->kick_thread, NULL);

    LOG_INFO("PDL_WDT", "[%s] Auto-kick service stopped", ctx->name);
    return OSAL_SUCCESS;
}

/**
 * @brief 手动喂狗
 */
int32_t PDL_WATCHDOG_kick(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_kick(ctx->hal_handle);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_atomic_fetch_add(&ctx->kick_count, 1);
        OSAL_atomic_store_u64(&ctx->last_kick_time, OSAL_get_monotonic_time());
    }

    return ret;
}

/**
 * @brief 获取Watchdog状态
 */
int32_t PDL_WATCHDOG_get_status(pdl_watchdog_handle_t handle, pdl_watchdog_status_t *status)
{
    watchdog_context_t *ctx;
    uint32_t timeout;

    if (handle == NULL || status == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid parameters");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    OSAL_memset(status, 0, OSAL_sizeof(pdl_watchdog_status_t));
    status->enabled = ctx->enabled;
    status->running = OSAL_atomic_load_bool(&ctx->running);
    status->kick_interval_ms = ctx->kick_interval_ms;
    status->mode = ctx->mode;
    status->kick_count = OSAL_atomic_load(&ctx->kick_count);
    status->last_kick_time_us = OSAL_atomic_load_u64(&ctx->last_kick_time);

    /* 获取HAL层超时时间 */
    timeout = 0;
    if (HAL_WATCHDOG_get_timeout(ctx->hal_handle, &timeout) == OSAL_SUCCESS)
    {
        status->timeout_sec = timeout;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 设置喂狗间隔
 */
int32_t PDL_WATCHDOG_set_interval(pdl_watchdog_handle_t handle, uint32_t interval_ms)
{
    watchdog_context_t *ctx;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    if (ctx->mode != PDL_WATCHDOG_MODE_AUTO)
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
int32_t PDL_WATCHDOG_enable(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_enable(ctx->hal_handle);
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
int32_t PDL_WATCHDOG_disable(pdl_watchdog_handle_t handle)
{
    watchdog_context_t *ctx;
    int32_t ret;

    if (handle == NULL)
    {
        LOG_ERROR("PDL_WDT", "Invalid handle");
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (watchdog_context_t *)handle;

    ret = HAL_WATCHDOG_disable(ctx->hal_handle);
    if (ret == OSAL_SUCCESS)
    {
        ctx->enabled = false;
        LOG_INFO("PDL_WDT", "[%s] Disabled", ctx->name);
    }

    return ret;
}
