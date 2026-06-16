/************************************************************************
 * 卫星平台服务实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理心跳和命令处理任务
 * - 调度内部CAN通信模块
 ************************************************************************/

#include "osal.h"
#include "pdl.h"
#include "pdl_satellite_internal.h"

/*
 * 卫星平台服务上下文
 */
typedef struct
{
    pdl_satellite_config_t config;
    void *can_handle;                 /* CAN通信句柄 */
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
        /* 发送心跳 */
        if (OSAL_SUCCESS == satellite_can_send_heartbeat(ctx->can_handle, PDL_SATELLITE_STATUS_OK))
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

        /* 延迟 */
        OSAL_msleep(ctx->config.heartbeat_interval_ms);
    }

    LOG_INFO("SAT", "Heartbeat task stopped");
    return NULL;
}

/*
 * CAN接收任务
 */
static void *can_rx_task(void *arg)
{
    satellite_service_context_t *ctx = (satellite_service_context_t *)arg;
    satellite_can_msg_t msg;
    int32_t ret;

    LOG_INFO("SAT", "CAN RX task started");

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 接收CAN消息 */
        ret = satellite_can_recv(ctx->can_handle, &msg, ctx->config.cmd_timeout_ms);

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
            LOG_ERROR("SAT", "CAN receive error: %d", ret);
        }
    }

    LOG_INFO("SAT", "CAN RX task stopped");
    return NULL;
}

/**
 * @brief 初始化卫星平台服务
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
    OSAL_memcpy(&ctx->config, config, OSAL_sizeof(pdl_satellite_config_t));
    OSAL_atomic_init_bool(&ctx->running, true);

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
    {
        LOG_ERROR("SAT", "Failed to create mutex");
        OSAL_free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化CAN通信 */
    ret = satellite_can_init(config->can_device, config->can_bitrate, &ctx->can_handle);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to initialize CAN");
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动运行标志 */
    OSAL_atomic_store_bool(&ctx->running, true);

    /* 创建CAN接收线程 */
    ret = OSAL_pthread_create(&ctx->rx_thread, NULL, can_rx_task, ctx);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("SAT", "Failed to create RX thread");
        satellite_can_deinit(ctx->can_handle);
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
        satellite_can_deinit(ctx->can_handle);
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

    /* 关闭CAN */
    satellite_can_deinit(ctx->can_handle);

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

    ret = satellite_can_send_response(ctx->can_handle, seq_num, status, result);
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
        LOG_ERROR("SAT", "Failed to send response");
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

    ret = satellite_can_send_heartbeat(ctx->can_handle, status);

    /* 加锁保护统计计数器，与其他函数保持一致 */
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

    /* 加锁保护统计计数器读取，与写入操作保持一致 */
    OSAL_pthread_mutex_lock(&ctx->mutex);
    if (NULL != rx_count) *rx_count = ctx->rx_count;
    if (NULL != tx_count) *tx_count = ctx->tx_count;
    if (NULL != error_count) *error_count = ctx->error_count;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}
