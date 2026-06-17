/************************************************************************
 * 卫星平台服务实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理心跳和命令处理任务
 * - 调度内部通信模块（支持多接口：CAN/Ethernet）
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"
#include "pdl.h"
#include "pdl_satellite_internal.h"

/*
 * 卫星平台服务上下文
 */
typedef struct
{
    pconfig_satellite_config_t config;
    pconfig_satellite_interface_t interface;
    void *comm_handle;                    /* 通信句柄（CAN/Ethernet） */
    const pdl_satellite_ops_t *ops;       /* 操作函数表（初始化时注册） */
    pdl_satellite_cmd_callback_t callback;
    void *user_data;

    /* 统计信息 */
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t error_count;

    /* 线程控制 */
    osal_thread_t rx_thread;
    osal_thread_t heartbeat_thread;
    osal_atomic_bool_t running;       /* 使用原子变量保证多线程安全 */

    /* 互斥锁保护 */
    osal_mutex_t mutex;
} satellite_service_context_t;

/*
 * 心跳任务
 */
static void *heartbeat_task(void *arg)
{
    satellite_service_context_t *ctx = (satellite_service_context_t *)arg;

    LOG_INFO("SAT", "Heartbeat task started");

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 通过 ops 发送心跳（无需 switch 判断）*/
        if (ctx->ops && ctx->ops->send_heartbeat)
        {
            if (OSAL_SUCCESS == ctx->ops->send_heartbeat(ctx->comm_handle, PDL_SATELLITE_STATUS_OK))
            {
                OSAL_pthread_mutex_lock(&ctx->mutex);
                ctx->tx_count++;
                OSAL_pthread_mutex_unlock(&ctx->mutex);
            }
            else
            {
                OSAL_pthread_mutex_lock(&ctx->mutex);
                ctx->error_count++;
                OSAL_pthread_mutex_unlock(&ctx->mutex);
            }
        }

        /* 延迟 */
        OSAL_msleep(ctx->config.heartbeat_interval_ms);
    }

    LOG_INFO("SAT", "Heartbeat task stopped");
    return NULL;
}

/*
 * 接收任务（适用于所有接口）
 */
static void *rx_task(void *arg)
{
    satellite_service_context_t *ctx = (satellite_service_context_t *)arg;
    satellite_can_msg_t msg;
    int32_t ret;

    LOG_INFO("SAT", "RX task started");

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 通过 ops 接收消息（无需 switch 判断）*/
        if (!ctx->ops || !ctx->ops->recv)
        {
            break;
        }

        ret = ctx->ops->recv(ctx->comm_handle, &msg, ctx->config.cmd_timeout_ms);

        if (OSAL_SUCCESS == ret)
        {
            OSAL_pthread_mutex_lock(&ctx->mutex);
            ctx->rx_count++;
            OSAL_pthread_mutex_unlock(&ctx->mutex);

            /* 处理命令请求 */
            if (msg.msg_type == CAN_MSG_TYPE_CMD_REQ)
            {
                pdl_satellite_cmd_callback_t callback;
                void *user_data;

                OSAL_pthread_mutex_lock(&ctx->mutex);
                callback = ctx->callback;
                user_data = ctx->user_data;
                OSAL_pthread_mutex_unlock(&ctx->mutex);

                if (NULL != callback)
                {
                    callback(msg.cmd_type, msg.data, user_data);
                }
            }
        }
        else if (OSAL_ERR_TIMEOUT != ret)
        {
            OSAL_pthread_mutex_lock(&ctx->mutex);
            ctx->error_count++;
            OSAL_pthread_mutex_unlock(&ctx->mutex);
            LOG_ERROR("SAT", "Receive error: %d", ret);
        }
    }

    LOG_INFO("SAT", "RX task stopped");
    return NULL;
}

/**
 * @brief 初始化卫星平台服务（从 PCONFIG 获取配置）
 */
int32_t PDL_SATELLITE_init_from_pconfig(uint32_t index, pdl_satellite_handle_t *handle)
{
    satellite_service_context_t *ctx;
    const pconfig_platform_config_t *platform;
    const pconfig_satellite_entry_t *sat_entry;
    const pconfig_satellite_config_t *config;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 从 PCONFIG 获取平台配置 */
    platform = PCONFIG_GetBoard();
    if (NULL == platform)
    {
        LOG_ERROR("SAT", "No platform config registered");
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 从 PCONFIG 获取 Satellite 配置 */
    sat_entry = PCONFIG_HW_GetSatellite(platform, index);
    if (NULL == sat_entry)
    {
        LOG_ERROR("SAT", "Satellite config not found for index %u", index);
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 检查是否启用 */
    if (!sat_entry->enabled)
    {
        LOG_WARN("SAT", "Satellite index %u is disabled in config", index);
        return OSAL_ERR_GENERIC;
    }

    config = &sat_entry->config;

    LOG_INFO("SAT", "Initializing Satellite index %u: %s", index, sat_entry->description);

    /* 分配上下文 */
    ctx = (satellite_service_context_t *)OSAL_malloc(OSAL_sizeof(satellite_service_context_t));
    if (NULL == ctx)
    {
        LOG_ERROR("SAT", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(satellite_service_context_t));
    OSAL_memcpy(&ctx->config, config, OSAL_sizeof(pconfig_satellite_config_t));
    ctx->interface = config->interface;
    OSAL_atomic_init_bool(&ctx->running, true);

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
    {
        LOG_ERROR("SAT", "Failed to create mutex");
        OSAL_free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 根据接口类型注册 ops（switch 只执行一次）*/
    ret = OSAL_ERR_GENERIC;
    switch (config->interface)
    {
        case PCONFIG_SATELLITE_INTERFACE_CAN:
            ctx->ops = &satellite_can_ops;
            ret = ctx->ops->init(&config->hw.can, &ctx->comm_handle);
            break;

        case PCONFIG_SATELLITE_INTERFACE_ETHERNET:
            ctx->ops = &satellite_ethernet_ops;
            ret = ctx->ops->init(&config->hw.ethernet, &ctx->comm_handle);
            break;

        default:
            LOG_ERROR("SAT", "Unknown interface type: %d", config->interface);
            ret = OSAL_ERR_GENERIC;
            break;
    }

    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to initialize interface");
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动运行标志 */
    OSAL_atomic_store_bool(&ctx->running, true);

    /* 创建接收线程 */
    ret = OSAL_pthread_create(&ctx->rx_thread, NULL, rx_task, ctx);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to create RX thread");
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 创建心跳线程 */
    ret = OSAL_pthread_create(&ctx->heartbeat_thread, NULL, heartbeat_task, ctx);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to create heartbeat thread");
        OSAL_atomic_store_bool(&ctx->running, false);
        OSAL_pthread_join(ctx->rx_thread, NULL);
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    *handle = (pdl_satellite_handle_t)ctx;
    LOG_INFO("SAT", "Satellite index %u initialized successfully", index);

    return OSAL_SUCCESS;
}

/**
 * @brief 初始化卫星平台服务（旧接口，保持向后兼容）
 */
int32_t PDL_SATELLITE_init(const pdl_satellite_config_t *config,
                        pdl_satellite_handle_t *handle)
{
    satellite_service_context_t *ctx;
    int32_t ret;

    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 分配上下文 */
    ctx = (satellite_service_context_t *)OSAL_malloc(OSAL_sizeof(satellite_service_context_t));
    if (NULL == ctx)
    {
        LOG_ERROR("SAT", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(satellite_service_context_t));
    OSAL_atomic_init_bool(&ctx->running, true);

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
    {
        LOG_ERROR("SAT", "Failed to create mutex");
        OSAL_free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 使用旧的 CAN 初始化接口 */
    ctx->ops = &satellite_can_ops;
    ret = satellite_can_init(config->can_device, config->can_bitrate, &ctx->comm_handle);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to initialize CAN");
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动运行标志 */
    OSAL_atomic_store_bool(&ctx->running, true);

    /* 创建接收线程 */
    ret = OSAL_pthread_create(&ctx->rx_thread, NULL, rx_task, ctx);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to create RX thread");
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 创建心跳线程 */
    ret = OSAL_pthread_create(&ctx->heartbeat_thread, NULL, heartbeat_task, ctx);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to create heartbeat thread");
        OSAL_atomic_store_bool(&ctx->running, false);
        OSAL_pthread_join(ctx->rx_thread, NULL);
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    *handle = (pdl_satellite_handle_t)ctx;
    LOG_INFO("SAT", "Satellite service initialized");

    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化卫星平台服务
 */
int32_t PDL_SATELLITE_deinit(pdl_satellite_handle_t handle)
{
    satellite_service_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (satellite_service_context_t *)handle;

    /* 停止线程 */
    OSAL_atomic_store_bool(&ctx->running, false);
    OSAL_pthread_join(ctx->rx_thread, NULL);
    OSAL_pthread_join(ctx->heartbeat_thread, NULL);

    /* 通过 ops 关闭通信接口（无需 switch 判断）*/
    if (ctx->ops && ctx->ops->deinit) {
        ctx->ops->deinit(ctx->comm_handle);
    }

    /* 删除互斥锁 */
    OSAL_pthread_mutex_destroy(&ctx->mutex);

    OSAL_free(ctx);
    LOG_INFO("SAT", "Satellite service deinitialized");

    return OSAL_SUCCESS;
}

/**
 * @brief 注册命令回调函数
 */
int32_t PDL_SATELLITE_register_callback(pdl_satellite_handle_t handle,
                                    pdl_satellite_cmd_callback_t callback,
                                    void *user_data)
{
    satellite_service_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (satellite_service_context_t *)handle;

    OSAL_pthread_mutex_lock(&ctx->mutex);
    ctx->callback = callback;
    ctx->user_data = user_data;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 发送响应到卫星平台
 */
int32_t PDL_SATELLITE_send_response(pdl_satellite_handle_t handle,
                                 uint32_t seq_num,
                                 pdl_satellite_status_t status,
                                 uint32_t result)
{
    satellite_service_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (satellite_service_context_t *)handle;

    /* 通过 ops 发送响应（无需 switch 判断）*/
    if (!ctx->ops || !ctx->ops->send_response)
    {
        return OSAL_ERR_GENERIC;
    }

    ret = ctx->ops->send_response(ctx->comm_handle, (uint8_t)seq_num, (uint8_t)status, result);

    if (OSAL_SUCCESS == ret)
    {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        ctx->tx_count++;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }
    else
    {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        ctx->error_count++;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }

    return ret;
}

/**
 * @brief 发送心跳到卫星平台
 */
int32_t PDL_SATELLITE_send_heartbeat(pdl_satellite_handle_t handle,
                                  pdl_satellite_status_t status)
{
    satellite_service_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (satellite_service_context_t *)handle;

    /* 通过 ops 发送心跳（无需 switch 判断）*/
    if (!ctx->ops || !ctx->ops->send_heartbeat)
    {
        return OSAL_ERR_GENERIC;
    }

    ret = ctx->ops->send_heartbeat(ctx->comm_handle, (uint8_t)status);

    if (OSAL_SUCCESS == ret)
    {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        ctx->tx_count++;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }
    else
    {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        ctx->error_count++;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }

    return ret;
}

/**
 * @brief 获取服务统计信息
 */
int32_t PDL_SATELLITE_get_stats(pdl_satellite_handle_t handle,
                             uint32_t *rx_count,
                             uint32_t *tx_count,
                             uint32_t *error_count)
{
    satellite_service_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (satellite_service_context_t *)handle;

    OSAL_pthread_mutex_lock(&ctx->mutex);

    if (NULL != rx_count) *rx_count = ctx->rx_count;
    if (NULL != tx_count) *tx_count = ctx->tx_count;
    if (NULL != error_count) *error_count = ctx->error_count;

    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}
