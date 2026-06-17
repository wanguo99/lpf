/************************************************************************
 * PMC 系统驱动实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理心跳和消息处理任务
 * - 调度内部通信模块（支持多接口：Ethernet/CAN）
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"
#include "prl.h"
#include "prl_pmc.h"  /* PMC protocol definitions */
#include "pdl.h"
#include "pdl_pmc_internal.h"

/*
 * PMC 系统驱动上下文
 */
typedef struct
{
    pconfig_pmc_config_t config;
    pconfig_pmc_interface_t interface;
    void *comm_handle;                    /* 通信句柄（Ethernet/CAN） */
    const pdl_pmc_ops_t *ops;             /* 操作函数表（初始化时注册） */

    /* 回调函数 */
    pdl_pmc_telemetry_callback_t tm_callback;
    void *tm_user_data;
    pdl_pmc_command_callback_t tc_callback;
    void *tc_user_data;

    /* 统计信息 */
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t error_count;
    uint32_t link_quality;

    /* 线程控制 */
    osal_thread_t rx_thread;
    osal_thread_t heartbeat_thread;
    osal_atomic_bool_t running;       /* 使用原子变量保证多线程安全 */

    /* 互斥锁保护 */
    osal_mutex_t mutex;
} pmc_driver_context_t;

/*
 * 心跳任务
 */
static void *heartbeat_task(void *arg)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)arg;
    uint8_t buf[256];
    size_t len;
    int32_t ret;

    LOG_INFO("PDL_PMC", "Heartbeat task started");

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 通过 ops 检查连接状态（无需 switch 判断）*/
        if (ctx->ops && ctx->ops->is_connected && !ctx->ops->is_connected(ctx->comm_handle))
        {
            LOG_WARN("PDL_PMC", "Connection lost, skip heartbeat");
            OSAL_msleep(ctx->config.heartbeat_interval_ms);
            continue;
        }

        /* 编码心跳消息 - 使用新的 PRL API */
        prl_pmc_heartbeat_t hb = {
            .sender_status = PDL_PMC_STATUS_OK,
            .link_quality = ctx->link_quality,
            .packet_loss = 0,
            .rtt_ms = 0
        };
        prl_encode_ctx_t encode_ctx = {
            .dev_type = PRL_DEV_TYPE_PMC,
            .msg_type = PRL_PMC_MSG_HEARTBEAT,
            .flags = 0,
            .payload = &hb,
            .payload_len = OSAL_sizeof(hb),
            .buffer = buf,
            .buffer_size = OSAL_sizeof(buf)
        };
        ret = prl_device_encode(&encode_ctx);
        if (ret < 0)
        {
            LOG_ERROR("PDL_PMC", "Failed to encode heartbeat");
            OSAL_pthread_mutex_lock(&ctx->mutex);
            ctx->error_count++;
            OSAL_pthread_mutex_unlock(&ctx->mutex);
            OSAL_msleep(ctx->config.heartbeat_interval_ms);
            continue;
        }
        len = ret;  /* prl_device_encode 返回编码后的长度 */

        /* 通过 ops 发送心跳（无需 switch 判断）*/
        pmc_msg_t msg = {
            .msg_type = 0x01,  /* 心跳消息类型 */
            .seq_num = 0,
            .payload_len = len,
        };
        OSAL_memcpy(msg.payload, buf, len);

        if (ctx->ops && ctx->ops->send)
        {
            ret = ctx->ops->send(ctx->comm_handle, &msg, ctx->config.send_timeout_ms);
            if (ret == OSAL_SUCCESS)
            {
                OSAL_pthread_mutex_lock(&ctx->mutex);
                ctx->tx_count++;
                OSAL_pthread_mutex_unlock(&ctx->mutex);
            }
            else
            {
                LOG_ERROR("PDL_PMC", "Failed to send heartbeat");
                OSAL_pthread_mutex_lock(&ctx->mutex);
                ctx->error_count++;
                OSAL_pthread_mutex_unlock(&ctx->mutex);
            }
        }

        /* 延迟 */
        OSAL_msleep(ctx->config.heartbeat_interval_ms);
    }

    LOG_INFO("PDL_PMC", "Heartbeat task stopped");
    return NULL;
}

/*
 * 接收任务（适用于所有接口）
 */
static void *rx_task(void *arg)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)arg;
    pmc_msg_t msg;
    int32_t ret;

    LOG_INFO("PDL_PMC", "RX task started");

    while (OSAL_atomic_load_bool(&ctx->running))
    {
        /* 通过 ops 接收消息（无需 switch 判断）*/
        if (!ctx->ops || !ctx->ops->recv)
        {
            break;
        }

        ret = ctx->ops->recv(ctx->comm_handle, &msg, ctx->config.recv_timeout_ms);

        if (ret == OSAL_SUCCESS)
        {
            OSAL_pthread_mutex_lock(&ctx->mutex);
            ctx->rx_count++;
            OSAL_pthread_mutex_unlock(&ctx->mutex);

            /* 处理遥测数据 */
            if (msg.msg_type == 0x02)  /* 遥测消息类型 */
            {
                uint8_t payload_buf[512];
                prl_decode_ctx_t decode_ctx = {
                    .buffer = msg.payload,
                    .buffer_len = msg.payload_len,
                    .payload = payload_buf,
                    .payload_size = sizeof(payload_buf)
                };

                ret = prl_device_decode(&decode_ctx);
                if (ret == OSAL_SUCCESS && decode_ctx.payload_len >= OSAL_sizeof(prl_pmc_telemetry_t))
                {
                    const prl_pmc_telemetry_t *tm = (const prl_pmc_telemetry_t *)payload_buf;
                    const uint8_t *data = payload_buf + OSAL_sizeof(prl_pmc_telemetry_t);
                    size_t data_len = decode_ctx.payload_len - OSAL_sizeof(prl_pmc_telemetry_t);

                    if (ctx->tm_callback)
                    {
                        ctx->tm_callback(tm->tm_id, tm->tm_source, data, data_len, ctx->tm_user_data);
                    }
                }
            }
            /* 处理遥控指令 */
            else if (msg.msg_type == 0x03)  /* 遥控消息类型 */
            {
                uint8_t payload_buf[512];
                prl_decode_ctx_t decode_ctx = {
                    .buffer = msg.payload,
                    .buffer_len = msg.payload_len,
                    .payload = payload_buf,
                    .payload_size = sizeof(payload_buf)
                };

                ret = prl_device_decode(&decode_ctx);
                if (ret == OSAL_SUCCESS && decode_ctx.payload_len >= OSAL_sizeof(prl_pmc_command_t))
                {
                    const prl_pmc_command_t *tc = (const prl_pmc_command_t *)payload_buf;
                    const uint8_t *params = payload_buf + OSAL_sizeof(prl_pmc_command_t);
                    size_t params_len = decode_ctx.payload_len - OSAL_sizeof(prl_pmc_command_t);

                    if (ctx->tc_callback)
                    {
                        ctx->tc_callback(tc->cmd_id, tc->target_node, tc->cmd_type, params, params_len, ctx->tc_user_data);
                    }
                }
            }
            /* 处理应答消息 */
            else if (msg.msg_type == 0xFF)  /* 应答消息类型 */
            {
                uint8_t payload_buf[512];
                prl_decode_ctx_t decode_ctx = {
                    .buffer = msg.payload,
                    .buffer_len = msg.payload_len,
                    .payload = payload_buf,
                    .payload_size = sizeof(payload_buf)
                };

                ret = prl_device_decode(&decode_ctx);
                if (ret == OSAL_SUCCESS && decode_ctx.payload_len >= OSAL_sizeof(prl_pmc_ack_t))
                {
                    const prl_pmc_ack_t *ack = (const prl_pmc_ack_t *)payload_buf;
                    LOG_DEBUG("PDL_PMC", "Received ACK seq=%u result=%u",
                                   ack->ack_seq, ack->ack_result);
                }
            }
        }
        else if (ret == OSAL_ERR_TIMEOUT)
        {
            /* 超时是正常的，继续接收 */
            continue;
        }
        else
        {
            LOG_ERROR("PDL_PMC", "Receive error");
            OSAL_pthread_mutex_lock(&ctx->mutex);
            ctx->error_count++;
            OSAL_pthread_mutex_unlock(&ctx->mutex);
        }
    }

    LOG_INFO("PDL_PMC", "RX task stopped");
    return NULL;
}

/**
 * @brief 初始化 PMC 系统驱动（从 PCONFIG 获取配置）
 */
int32_t PDL_PMC_init_from_pconfig(uint32_t index, pdl_pmc_handle_t *handle)
{
    pmc_driver_context_t *ctx;
    const pconfig_platform_config_t *platform;
    const pconfig_pmc_entry_t *pmc_entry;
    const pconfig_pmc_config_t *config;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 从 PCONFIG 获取平台配置 */
    platform = PCONFIG_GetBoard();
    if (NULL == platform)
    {
        LOG_ERROR("PDL_PMC", "No platform config registered");
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 从 PCONFIG 获取 PMC 配置 */
    pmc_entry = PCONFIG_HW_GetPMC(platform, index);
    if (NULL == pmc_entry)
    {
        LOG_ERROR("PDL_PMC", "PMC config not found for index %u", index);
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 检查是否启用 */
    if (!pmc_entry->enabled)
    {
        LOG_WARN("PDL_PMC", "PMC index %u is disabled in config", index);
        return OSAL_ERR_GENERIC;
    }

    config = &pmc_entry->config;

    LOG_INFO("PDL_PMC", "Initializing PMC index %u: %s", index, pmc_entry->description);

    /* 分配上下文 */
    ctx = (pmc_driver_context_t *)OSAL_malloc(OSAL_sizeof(pmc_driver_context_t));
    if (!ctx)
    {
        LOG_ERROR("PDL_PMC", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(pmc_driver_context_t));
    OSAL_memcpy(&ctx->config, config, OSAL_sizeof(pconfig_pmc_config_t));
    ctx->interface = config->interface;
    ctx->link_quality = 100;  /* 初始链路质量 */
    OSAL_atomic_init_bool(&ctx->running, false);

    /* 创建互斥锁 */
    ret = OSAL_pthread_mutex_init(&ctx->mutex, NULL);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create mutex");
        OSAL_free(ctx);
        return ret;
    }

    /* 根据接口类型注册 ops（switch 只执行一次）*/
    ret = OSAL_ERR_GENERIC;
    switch (config->interface)
    {
        case PCONFIG_PMC_INTERFACE_ETHERNET:
            ctx->ops = &pmc_eth_ops;
            ret = ctx->ops->init(&config->hw.ethernet, &ctx->comm_handle);
            break;

        case PCONFIG_PMC_INTERFACE_CAN:
            ctx->ops = &pmc_can_ops;
            ret = ctx->ops->init(&config->hw.can, &ctx->comm_handle);
            break;

        default:
            LOG_ERROR("PDL_PMC", "Unknown interface type: %d", config->interface);
            ret = OSAL_ERR_GENERIC;
            break;
    }

    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to initialize interface");
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动接收线程 */
    OSAL_atomic_store_bool(&ctx->running, true);
    ret = OSAL_pthread_create(&ctx->rx_thread, NULL, rx_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create RX thread");
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动心跳线程 */
    ret = OSAL_pthread_create(&ctx->heartbeat_thread, NULL, heartbeat_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create heartbeat thread");
        OSAL_atomic_store_bool(&ctx->running, false);
        OSAL_pthread_join(ctx->rx_thread, NULL);
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    *handle = ctx;
    LOG_INFO("PDL_PMC", "PMC index %u initialized successfully", index);
    return OSAL_SUCCESS;
}

/*
 * 初始化 PMC 系统驱动（旧接口，保持向后兼容）
 */
int32_t PDL_PMC_init(const pdl_pmc_config_t *config,
                     pdl_pmc_handle_t *handle)
{
    pmc_driver_context_t *ctx;
    int32_t ret;

    if (!config || !handle)
    {
        LOG_ERROR("PDL_PMC", "Invalid parameters");
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 分配上下文 */
    ctx = (pmc_driver_context_t *)OSAL_malloc(OSAL_sizeof(pmc_driver_context_t));
    if (!ctx)
    {
        LOG_ERROR("PDL_PMC", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(pmc_driver_context_t));
    ctx->link_quality = 100;  /* 初始链路质量 */
    OSAL_atomic_init_bool(&ctx->running, false);

    /* 创建互斥锁 */
    ret = OSAL_pthread_mutex_init(&ctx->mutex, NULL);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create mutex");
        OSAL_free(ctx);
        return ret;
    }

    /* 使用旧的 Ethernet 初始化接口 */
    ctx->ops = &pmc_eth_ops;
    ret = pmc_eth_init(config, &ctx->comm_handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to init ethernet");
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动接收线程 */
    OSAL_atomic_store_bool(&ctx->running, true);
    ret = OSAL_pthread_create(&ctx->rx_thread, NULL, rx_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create RX thread");
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    /* 启动心跳线程 */
    ret = OSAL_pthread_create(&ctx->heartbeat_thread, NULL, heartbeat_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_PMC", "Failed to create heartbeat thread");
        OSAL_atomic_store_bool(&ctx->running, false);
        OSAL_pthread_join(ctx->rx_thread, NULL);
        if (ctx->ops && ctx->ops->deinit) {
            ctx->ops->deinit(ctx->comm_handle);
        }
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    *handle = ctx;
    LOG_INFO("PDL_PMC", "Initialized successfully");
    return OSAL_SUCCESS;
}

/*
 * 反初始化 PMC 系统驱动
 */
int32_t PDL_PMC_deinit(pdl_pmc_handle_t handle)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 停止线程 */
    OSAL_atomic_store_bool(&ctx->running, false);
    OSAL_pthread_join(ctx->rx_thread, NULL);
    OSAL_pthread_join(ctx->heartbeat_thread, NULL);

    /* 通过 ops 清理通信接口（无需 switch 判断）*/
    if (ctx->ops && ctx->ops->deinit) {
        ctx->ops->deinit(ctx->comm_handle);
    }

    /* 销毁互斥锁 */
    OSAL_pthread_mutex_destroy(&ctx->mutex);

    /* 释放上下文 */
    OSAL_free(ctx);

    LOG_INFO("PDL_PMC", "Deinitialized");
    return OSAL_SUCCESS;
}

/*
 * 注册遥测数据回调函数
 */
int32_t PDL_PMC_register_telemetry_callback(pdl_pmc_handle_t handle,
                                          pdl_pmc_telemetry_callback_t callback,
                                          void *user_data)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_pthread_mutex_lock(&ctx->mutex);
    ctx->tm_callback = callback;
    ctx->tm_user_data = user_data;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 注册遥控指令回调函数
 */
int32_t PDL_PMC_register_command_callback(pdl_pmc_handle_t handle,
                                         pdl_pmc_command_callback_t callback,
                                         void *user_data)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_pthread_mutex_lock(&ctx->mutex);
    ctx->tc_callback = callback;
    ctx->tc_user_data = user_data;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 获取驱动统计信息
 */
int32_t PDL_PMC_GetStats(pdl_pmc_handle_t handle,
                          uint32_t *rx_count,
                          uint32_t *tx_count,
                          uint32_t *error_count)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_pthread_mutex_lock(&ctx->mutex);
    if (rx_count) *rx_count = ctx->rx_count;
    if (tx_count) *tx_count = ctx->tx_count;
    if (error_count) *error_count = ctx->error_count;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 获取连接状态
 */
int32_t PDL_PMC_GetConnectionStatus(pdl_pmc_handle_t handle,
                                     bool *connected,
                                     uint32_t *link_quality)
{
    pmc_driver_context_t *ctx = (pmc_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    if (connected)
    {
        /* 通过 ops 检查连接状态（无需 switch 判断）*/
        if (ctx->ops && ctx->ops->is_connected) {
            *connected = ctx->ops->is_connected(ctx->comm_handle);
        } else {
            *connected = false;
        }
    }

    if (link_quality)
    {
        OSAL_pthread_mutex_lock(&ctx->mutex);
        *link_quality = ctx->link_quality;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }

    return OSAL_SUCCESS;
}
