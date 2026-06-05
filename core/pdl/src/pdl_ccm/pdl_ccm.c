/************************************************************************
 * CCM 系统驱动实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理心跳和消息处理任务
 * - 调度内部以太网通信模块
 ************************************************************************/

#include "pdl/pdl.h"
#include "pdl_ccm_internal.h"
#include "prl/prl.h"      /* 新的 PRL API */
#include "prl_pmc.h"      /* PMC 设备协议 */
#include "osal/osal.h"
#include <string.h>

/*
 * CCM 系统驱动上下文
 */
typedef struct
{
    pdl_ccm_config_t config;
    void *eth_handle;                 /* 以太网通信句柄 */

    /* 回调函数 */
    pdl_ccm_telemetry_callback_t tm_callback;
    void *tm_user_data;
    pdl_ccm_command_callback_t tc_callback;
    void *tc_user_data;

    /* 统计信息 */
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t error_count;
    uint32_t link_quality;

    /* 线程控制 */
    osal_thread_t rx_thread;
    osal_thread_t heartbeat_thread;
    volatile bool running;

    /* 互斥锁保护 */
    osal_mutex_t *mutex;
} ccm_driver_context_t;

/*
 * 心跳任务
 */
static void *heartbeat_task(void *arg)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)arg;
    uint8_t buf[256];
    size_t len;
    int32_t ret;

    LOG_INFO("PDL_CCM", "Heartbeat task started");

    while (ctx->running)
    {
        /* 检查连接状态 */
        if (!ccm_eth_is_connected(ctx->eth_handle))
        {
            LOG_WARN("PDL_CCM", "Connection lost, skip heartbeat");
            OSAL_msleep(ctx->config.heartbeat_interval_ms);
            continue;
        }

        /* 编码心跳消息 - 使用新的 PRL API */
        prl_pmc_heartbeat_t hb = {
            .sender_status = PDL_CCM_STATUS_OK,
            .link_quality = ctx->link_quality,
            .packet_loss = 0,
            .rtt_ms = 0
        };
        ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_HEARTBEAT,
                        &hb, sizeof(hb), buf, sizeof(buf), 0);
        if (ret < 0)
        {
            LOG_ERROR("PDL_CCM", "Failed to encode heartbeat");
            OSAL_MutexLock(ctx->mutex);
            ctx->error_count++;
            OSAL_MutexUnlock(ctx->mutex);
            OSAL_msleep(ctx->config.heartbeat_interval_ms);
            continue;
        }
        len = ret;  /* PRL_Encode 返回编码后的长度 */

        /* 发送心跳 */
        ccm_eth_msg_t msg = {
            .msg_type = 0x01,  /* 心跳消息类型 */
            .seq_num = 0,
            .payload_len = len,
        };
        OSAL_Memcpy(msg.payload, buf, len);

        ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
        if (ret == OSAL_SUCCESS)
        {
            OSAL_MutexLock(ctx->mutex);
            ctx->tx_count++;
            OSAL_MutexUnlock(ctx->mutex);
        }
        else
        {
            LOG_ERROR("PDL_CCM", "Failed to send heartbeat");
            OSAL_MutexLock(ctx->mutex);
            ctx->error_count++;
            OSAL_MutexUnlock(ctx->mutex);
        }

        /* 延迟 */
        OSAL_msleep(ctx->config.heartbeat_interval_ms);
    }

    LOG_INFO("PDL_CCM", "Heartbeat task stopped");
    return NULL;
}

/*
 * 以太网接收任务
 */
static void *eth_rx_task(void *arg)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)arg;
    ccm_eth_msg_t msg;
    int32_t ret;

    LOG_INFO("PDL_CCM", "Ethernet RX task started");

    while (ctx->running)
    {
        /* 接收以太网消息 */
        ret = ccm_eth_recv(ctx->eth_handle, &msg, ctx->config.recv_timeout_ms);

        if (ret == OSAL_SUCCESS)
        {
            OSAL_MutexLock(ctx->mutex);
            ctx->rx_count++;
            OSAL_MutexUnlock(ctx->mutex);

            /* 处理遥测数据 */
            if (msg.msg_type == 0x02)  /* 遥测消息类型 */
            {
                uint8_t dev_type, msg_type;
                const uint8_t *payload;
                uint16_t payload_len;

                ret = PRL_Decode(msg.payload, msg.payload_len,
                                &dev_type, &msg_type, &payload, &payload_len);
                if (ret == OSAL_SUCCESS && payload_len >= sizeof(prl_pmc_telemetry_t))
                {
                    const prl_pmc_telemetry_t *tm = (const prl_pmc_telemetry_t *)payload;
                    const uint8_t *data = payload + sizeof(prl_pmc_telemetry_t);
                    size_t data_len = payload_len - sizeof(prl_pmc_telemetry_t);

                    if (ctx->tm_callback)
                    {
                        ctx->tm_callback(tm->tm_id, tm->tm_source, data, data_len, ctx->tm_user_data);
                    }
                }
            }
            /* 处理遥控指令 */
            else if (msg.msg_type == 0x03)  /* 遥控消息类型 */
            {
                uint8_t dev_type, msg_type;
                const uint8_t *payload;
                uint16_t payload_len;

                ret = PRL_Decode(msg.payload, msg.payload_len,
                                &dev_type, &msg_type, &payload, &payload_len);
                if (ret == PRL_OK && payload_len >= sizeof(prl_pmc_command_t))
                {
                    const prl_pmc_command_t *tc = (const prl_pmc_command_t *)payload;
                    const uint8_t *params = payload + sizeof(prl_pmc_command_t);
                    size_t params_len = payload_len - sizeof(prl_pmc_command_t);

                    if (ctx->tc_callback)
                    {
                        ctx->tc_callback(tc->cmd_id, tc->target_node, tc->cmd_type, params, params_len, ctx->tc_user_data);
                    }
                }
            }
            /* 处理应答消息 */
            else if (msg.msg_type == 0xFF)  /* 应答消息类型 */
            {
                uint8_t dev_type, msg_type;
                const uint8_t *payload;
                uint16_t payload_len;

                ret = PRL_Decode(msg.payload, msg.payload_len,
                                &dev_type, &msg_type, &payload, &payload_len);
                if (ret == PRL_OK && payload_len >= sizeof(prl_pmc_ack_t))
                {
                    const prl_pmc_ack_t *ack = (const prl_pmc_ack_t *)payload;
                    LOG_DEBUG("PDL_CCM", "Received ACK seq=%u result=%u",
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
            LOG_ERROR("PDL_CCM", "Ethernet receive error");
            OSAL_MutexLock(ctx->mutex);
            ctx->error_count++;
            OSAL_MutexUnlock(ctx->mutex);
        }
    }

    LOG_INFO("PDL_CCM", "Ethernet RX task stopped");
    return NULL;
}

/*
 * 初始化 CCM 系统驱动
 */
int32_t PDL_CCM_Init(const pdl_ccm_config_t *config,
                     pdl_ccm_handle_t *handle)
{
    ccm_driver_context_t *ctx;
    int32_t ret;

    if (!config || !handle)
    {
        LOG_ERROR("PDL_CCM", "Invalid parameters");
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 分配上下文 */
    ctx = (ccm_driver_context_t *)OSAL_Malloc(sizeof(ccm_driver_context_t));
    if (!ctx)
    {
        LOG_ERROR("PDL_CCM", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_Memset(ctx, 0, sizeof(ccm_driver_context_t));
    OSAL_Memcpy(&ctx->config, config, sizeof(pdl_ccm_config_t));
    ctx->link_quality = 100;  /* 初始链路质量 */

    /* 创建互斥锁 */
    ret = OSAL_MutexCreate(&ctx->mutex);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_CCM", "Failed to create mutex");
        OSAL_Free(ctx);
        return ret;
    }

    /* 初始化以太网通信 */
    ret = ccm_eth_init(config, &ctx->eth_handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_CCM", "Failed to init ethernet");
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return ret;
    }

    /* 启动接收线程 */
    ctx->running = true;
    ret = OSAL_ThreadCreate(&ctx->rx_thread, eth_rx_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_CCM", "Failed to create RX thread");
        ccm_eth_deinit(ctx->eth_handle);
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return ret;
    }

    /* 启动心跳线程 */
    ret = OSAL_ThreadCreate(&ctx->heartbeat_thread, heartbeat_task, ctx);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("PDL_CCM", "Failed to create heartbeat thread");
        ctx->running = false;
        OSAL_ThreadJoin(ctx->rx_thread);
        ccm_eth_deinit(ctx->eth_handle);
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return ret;
    }

    *handle = ctx;
    LOG_INFO("PDL_CCM", "Initialized successfully");
    return OSAL_SUCCESS;
}

/*
 * 反初始化 CCM 系统驱动
 */
int32_t PDL_CCM_Deinit(pdl_ccm_handle_t handle)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 停止线程 */
    ctx->running = false;
    OSAL_ThreadJoin(ctx->rx_thread);
    OSAL_ThreadJoin(ctx->heartbeat_thread);

    /* 清理以太网通信 */
    ccm_eth_deinit(ctx->eth_handle);

    /* 销毁互斥锁 */
    OSAL_MutexDelete(ctx->mutex);

    /* 释放上下文 */
    OSAL_Free(ctx);

    LOG_INFO("PDL_CCM", "Deinitialized");
    return OSAL_SUCCESS;
}

/*
 * 注册遥测数据回调函数
 */
int32_t PDL_CCM_RegisterTelemetryCallback(pdl_ccm_handle_t handle,
                                          pdl_ccm_telemetry_callback_t callback,
                                          void *user_data)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_MutexLock(ctx->mutex);
    ctx->tm_callback = callback;
    ctx->tm_user_data = user_data;
    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 注册遥控指令回调函数
 */
int32_t PDL_CCM_RegisterCommandCallback(pdl_ccm_handle_t handle,
                                         pdl_ccm_command_callback_t callback,
                                         void *user_data)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_MutexLock(ctx->mutex);
    ctx->tc_callback = callback;
    ctx->tc_user_data = user_data;
    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 发送遥测数据到 CCM
 */
int32_t PDL_CCM_SendTelemetry(pdl_ccm_handle_t handle,
                               uint32_t tm_id,
                               uint32_t tm_source,
                               const uint8_t *data,
                               size_t len)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx || !data)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码遥测消息 - 使用新的 PRL API */
    uint8_t prl_buf[PRL_MAX_PACKET_SIZE];

    /* 构造完整的遥测负载（固定部分 + 变长数据） */
    prl_pmc_telemetry_t *tm = (prl_pmc_telemetry_t *)prl_buf;
    tm->tm_id = tm_id;
    tm->tm_source = tm_source;
    tm->timestamp_us = OSAL_GetMonotonicTime();
    tm->data_type = 0;
    tm->data_length = len;

    /* 拷贝变长数据 */
    if (len > 0 && len <= (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_telemetry_t)))
    {
        OSAL_Memcpy(prl_buf + sizeof(prl_pmc_telemetry_t), data, len);
    }
    else if (len > (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_telemetry_t)))
    {
        LOG_ERROR("PDL_CCM", "Telemetry data too large");
        return OSAL_ERR_INVALID_PARAM;
    }

    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_TELEMETRY,
                    prl_buf, sizeof(prl_pmc_telemetry_t) + len,
                    buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode telemetry");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x02,  /* 遥测消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 发送遥控指令到 CCM
 */
int32_t PDL_CCM_SendCommand(pdl_ccm_handle_t handle,
                             uint32_t tc_id,
                             uint32_t tc_target,
                             uint32_t tc_action,
                             const uint8_t *params,
                             size_t params_len)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码遥控消息 - 使用新的 PRL API */
    uint8_t prl_buf[PRL_MAX_PACKET_SIZE];

    /* 构造完整的遥控负载（固定部分 + 变长参数） */
    prl_pmc_command_t *tc = (prl_pmc_command_t *)prl_buf;
    tc->cmd_id = tc_id;
    tc->cmd_type = tc_action;  /* 使用 tc_action 作为 cmd_type */
    tc->target_node = tc_target;
    tc->param_length = params_len;

    /* 拷贝变长参数 */
    if (params_len > 0 && params && params_len <= (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_command_t)))
    {
        OSAL_Memcpy(prl_buf + sizeof(prl_pmc_command_t), params, params_len);
    }
    else if (params_len > (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_command_t)))
    {
        LOG_ERROR("PDL_CCM", "Command params too large");
        return OSAL_ERR_INVALID_PARAM;
    }

    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_COMMAND,
                    prl_buf, sizeof(prl_pmc_command_t) + params_len,
                    buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode command");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x03,  /* 遥控消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 发送固件升级数据到 CCM
 */
int32_t PDL_CCM_SendFirmwareUpdate(pdl_ccm_handle_t handle,
                                    uint32_t firmware_id,
                                    uint32_t target_device,
                                    uint32_t firmware_version,
                                    uint32_t total_size,
                                    uint32_t offset,
                                    const uint8_t *data,
                                    size_t len,
                                    const uint8_t md5[16])
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx || !data || !md5)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码固件升级消息 - 使用新的 PRL API */
    uint8_t prl_buf[PRL_MAX_PACKET_SIZE];

    /* 构造完整的固件升级负载（固定部分 + 变长数据） */
    prl_pmc_firmware_update_t *fw = (prl_pmc_firmware_update_t *)prl_buf;
    fw->firmware_id = firmware_id;
    fw->firmware_version = firmware_version;
    fw->total_size = total_size;
    fw->chunk_index = offset / len;  /* 计算分片索引 */
    fw->chunk_size = len;
    fw->chunk_crc = 0;  /* TODO: 计算 CRC */

    /* 拷贝变长数据 */
    if (len > 0 && len <= (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_firmware_update_t)))
    {
        OSAL_Memcpy(prl_buf + sizeof(prl_pmc_firmware_update_t), data, len);
    }
    else if (len > (PRL_MAX_PACKET_SIZE - sizeof(prl_pmc_firmware_update_t)))
    {
        LOG_ERROR("PDL_CCM", "Firmware chunk too large");
        return OSAL_ERR_INVALID_PARAM;
    }

    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_FIRMWARE_UPDATE,
                    prl_buf, sizeof(prl_pmc_firmware_update_t) + len,
                    buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode firmware update");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x04,  /* 固件升级消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 发送节点管理命令到 CCM
 */
int32_t PDL_CCM_NodeManage(pdl_ccm_handle_t handle,
                            uint32_t node_id,
                            pdl_ccm_node_op_t operation,
                            uint32_t *node_status)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码节点管理消息 - 使用新的 PRL API */
    prl_pmc_node_manage_t nm = {
        .operation = operation,
        .node_id = node_id,
        .node_type = 0,
        .node_status = 0
    };
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_NODE_MANAGE,
                    &nm, sizeof(nm), buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode node manage");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x05,  /* 节点管理消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);

        /* TODO: 等待应答并返回节点状态 */
        if (node_status)
        {
            *node_status = 0;  /* 临时返回 */
        }
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 发送电源控制命令到 CCM
 */
int32_t PDL_CCM_PowerControl(pdl_ccm_handle_t handle,
                              uint32_t power_domain,
                              pdl_ccm_power_op_t operation,
                              uint32_t *power_status)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码电源控制消息 - 使用新的 PRL API */
    prl_pmc_power_control_t pc = {
        .operation = operation,
        .target_device = 0,
        .power_domain = power_domain,
        .delay_ms = 0
    };
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_POWER_CONTROL,
                    &pc, sizeof(pc), buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode power control");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x06,  /* 电源控制消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);

        /* TODO: 等待应答并返回电源状态 */
        if (power_status)
        {
            *power_status = 0;  /* 临时返回 */
        }
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 查询 CCM 状态
 */
int32_t PDL_CCM_QueryStatus(pdl_ccm_handle_t handle,
                             uint32_t query_type,
                             uint32_t query_target,
                             uint32_t *result)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[CCM_ETH_MAX_MSG_SIZE];
    size_t buf_len;
    int32_t ret;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码状态查询消息 - 使用新的 PRL API */
    prl_pmc_status_query_t sq = {
        .query_type = query_type,
        .target_device = query_target,
        .param_count = 0
    };
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_STATUS_QUERY,
                    &sq, sizeof(sq), buf, sizeof(buf), 0);
    if (ret < 0)
    {
        LOG_ERROR("PDL_CCM", "Failed to encode status query");
        return ret;
    }
    buf_len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x07,  /* 状态查询消息类型 */
        .seq_num = 0,
        .payload_len = buf_len,
    };
    OSAL_Memcpy(msg.payload, buf, buf_len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);

        /* TODO: 等待应答并返回查询结果 */
        if (result)
        {
            *result = 0;  /* 临时返回 */
        }
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 发送心跳到 CCM
 */
int32_t PDL_CCM_SendHeartbeat(pdl_ccm_handle_t handle,
                               pdl_ccm_status_t status)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;
    uint8_t buf[256];
    size_t len;
    int32_t ret;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 编码心跳消息 - 使用新的 PRL API */
    prl_pmc_heartbeat_t hb = {
        .sender_status = status,
        .link_quality = ctx->link_quality,
        .packet_loss = 0,
        .rtt_ms = 0
    };
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_HEARTBEAT,
                    &hb, sizeof(hb), buf, sizeof(buf), 0);
    if (ret < 0)
    {
        return ret;
    }
    len = ret;

    /* 发送 */
    ccm_eth_msg_t msg = {
        .msg_type = 0x01,  /* 心跳消息类型 */
        .seq_num = 0,
        .payload_len = len,
    };
    OSAL_Memcpy(msg.payload, buf, len);

    ret = ccm_eth_send(ctx->eth_handle, &msg, ctx->config.send_timeout_ms);
    if (ret == OSAL_SUCCESS)
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->tx_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }
    else
    {
        OSAL_MutexLock(ctx->mutex);
        ctx->error_count++;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return ret;
}

/*
 * 获取驱动统计信息
 */
int32_t PDL_CCM_GetStats(pdl_ccm_handle_t handle,
                          uint32_t *rx_count,
                          uint32_t *tx_count,
                          uint32_t *error_count)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_MutexLock(ctx->mutex);
    if (rx_count) *rx_count = ctx->rx_count;
    if (tx_count) *tx_count = ctx->tx_count;
    if (error_count) *error_count = ctx->error_count;
    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}

/*
 * 获取连接状态
 */
int32_t PDL_CCM_GetConnectionStatus(pdl_ccm_handle_t handle,
                                     bool *connected,
                                     uint32_t *link_quality)
{
    ccm_driver_context_t *ctx = (ccm_driver_context_t *)handle;

    if (!ctx)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    if (connected)
    {
        *connected = ccm_eth_is_connected(ctx->eth_handle);
    }

    if (link_quality)
    {
        OSAL_MutexLock(ctx->mutex);
        *link_quality = ctx->link_quality;
        OSAL_MutexUnlock(ctx->mutex);
    }

    return OSAL_SUCCESS;
}
