/************************************************************************
 * BMC服务实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理通信通道（网络/串口）和协议类型（IPMI/Redfish）
 * - 调度传输层和协议层模块
 ************************************************************************/

#include "pdl_bmc.h"
#include "pdl_bmc_internal.h"
#include "osal.h"

/*
 * BMC服务上下文
 */
typedef struct
{
    bmc_config_t config;

    /* 传输层句柄 */
    void *net_transport_handle;
    void *serial_transport_handle;

    /* 协议层句柄 */
    void *protocol_handle;
    bmc_protocol_t protocol_type;

    /* 当前通道 */
    bmc_channel_t current_channel;
    bool connected;

    /* 统计信息 */
    uint32_t cmd_count;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t switch_count;

    /* 互斥锁 */
    osal_id_t mutex;
} bmc_context_t;

/**
 * @brief 初始化BMC服务
 */
int32_t PDL_BMC_Init(const bmc_config_t *config,
                        bmc_handle_t *handle)
{
    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 分配上下文 */
    bmc_context_t *ctx = (bmc_context_t *)OSAL_Malloc(sizeof(bmc_context_t));
    if (NULL == ctx)
    {
        LOG_ERROR("BMC", "Failed to allocate context");
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_context_t));
    OSAL_Memcpy(&ctx->config, config, sizeof(bmc_config_t));
    ctx->current_channel = config->primary_channel;
    ctx->protocol_type = BMC_PROTOCOL_IPMI;

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex, "bmc_mutex", 0))
    {
        LOG_ERROR("BMC", "Failed to create mutex");
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化网络传输层 */
    if (config->network.enabled)
    {
        int32_t ret = bmc_transport_net_init(config->network.ip_addr,
                                             config->network.port,
                                             config->network.timeout_ms,
                                             &ctx->net_transport_handle);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Failed to open network transport");
        }
        else
        {
            LOG_INFO("BMC", "Network transport opened: %s:%d",
                     config->network.ip_addr, config->network.port);
        }
    }

    /* 初始化串口传输层 */
    if (config->serial.enabled)
    {
        int32_t ret = bmc_transport_serial_init(config->serial.device,
                                                config->serial.baudrate,
                                                config->serial.timeout_ms,
                                                &ctx->serial_transport_handle);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Failed to open serial transport");
        }
        else
        {
            LOG_INFO("BMC", "Serial transport opened: %s", config->serial.device);
        }
    }

    /* 初始化协议层（根据配置选择IPMI或Redfish） */
    void *transport_handle = (ctx->current_channel == BMC_CHANNEL_NETWORK) ?
                             ctx->net_transport_handle : ctx->serial_transport_handle;

    if (NULL != transport_handle)
    {
        if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
        {
            int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);
            send_recv = (ctx->current_channel == BMC_CHANNEL_NETWORK) ?
                        bmc_transport_net_send_recv : bmc_transport_serial_send_recv;

            int32_t ret = bmc_ipmi_init(transport_handle, send_recv, &ctx->protocol_handle);
            if (OSAL_SUCCESS != ret)
            {
                LOG_ERROR("BMC", "Failed to initialize IPMI protocol");
            }
            else
            {
                LOG_INFO("BMC", "IPMI protocol initialized");
                ctx->connected = true;
            }
        }
        else if (BMC_PROTOCOL_REDFISH == ctx->protocol_type)
        {
            int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);
            send_recv = (ctx->current_channel == BMC_CHANNEL_NETWORK) ?
                        bmc_transport_net_send_recv : bmc_transport_serial_send_recv;

            int32_t ret = bmc_redfish_init(transport_handle, send_recv,
                                          config->network.username,
                                          config->network.password,
                                          &ctx->protocol_handle);
            if (OSAL_SUCCESS != ret)
            {
                LOG_ERROR("BMC", "Failed to initialize Redfish protocol");
            }
            else
            {
                LOG_INFO("BMC", "Redfish protocol initialized");
                ctx->connected = true;
            }
        }
    }

    *handle = (bmc_handle_t)ctx;
    LOG_INFO("BMC", "BMC service initialized (protocol: %s)",
             (ctx->protocol_type == BMC_PROTOCOL_IPMI) ? "IPMI" : "Redfish");

    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化BMC服务
 */
int32_t PDL_BMC_Deinit(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    /* 关闭协议层 */
    if (NULL != ctx->protocol_handle)
    {
        if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
        {
            bmc_ipmi_deinit(ctx->protocol_handle);
        }
        else if (BMC_PROTOCOL_REDFISH == ctx->protocol_type)
        {
            bmc_redfish_deinit(ctx->protocol_handle);
        }
    }

    /* 关闭网络传输层 */
    if (NULL != ctx->net_transport_handle)
    {
        bmc_transport_net_deinit(ctx->net_transport_handle);
    }

    /* 关闭串口传输层 */
    if (NULL != ctx->serial_transport_handle)
    {
        bmc_transport_serial_deinit(ctx->serial_transport_handle);
    }

    /* 删除互斥锁 */
    OSAL_MutexDelete(ctx->mutex);

    OSAL_Free(ctx);
    LOG_INFO("BMC", "BMC service deinitialized");

    return OSAL_SUCCESS;
}

/**
 * @brief 电源开机
 */
int32_t PDL_BMC_PowerOn(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    int32_t ret;
    if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
    {
        ret = bmc_ipmi_power_on(ctx->protocol_handle);
    }
    else
    {
        ret = bmc_redfish_power_on(ctx->protocol_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power on command sent");
    }
    else
    {
        ctx->fail_count++;
        LOG_ERROR("BMC", "Failed to send power on command");
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 电源关机
 */
int32_t PDL_BMC_PowerOff(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    int32_t ret;
    if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
    {
        ret = bmc_ipmi_power_off(ctx->protocol_handle);
    }
    else
    {
        ret = bmc_redfish_power_off(ctx->protocol_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power off command sent");
    }
    else
    {
        ctx->fail_count++;
        LOG_ERROR("BMC", "Failed to send power off command");
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 电源复位
 */
int32_t PDL_BMC_PowerReset(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    int32_t ret;
    if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
    {
        ret = bmc_ipmi_power_reset(ctx->protocol_handle);
    }
    else
    {
        ret = bmc_redfish_power_reset(ctx->protocol_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power reset command sent");
    }
    else
    {
        ctx->fail_count++;
        LOG_ERROR("BMC", "Failed to send power reset command");
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 查询电源状态
 */
int32_t PDL_BMC_GetPowerState(bmc_handle_t handle,
                                 bmc_power_state_t *state)
{
    if (NULL == handle || NULL == state)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    int32_t ret;
    if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
    {
        ret = bmc_ipmi_get_power_state(ctx->protocol_handle, state);
    }
    else
    {
        ret = bmc_redfish_get_power_state(ctx->protocol_handle, state);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
    }
    else
    {
        ctx->fail_count++;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 读取传感器
 */
int32_t PDL_BMC_ReadSensors(bmc_handle_t handle,
                               bmc_sensor_type_t type,
                               bmc_sensor_reading_t *readings,
                               uint32_t max_count,
                               uint32_t *actual_count)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    int32_t ret;
    if (BMC_PROTOCOL_IPMI == ctx->protocol_type)
    {
        ret = bmc_ipmi_read_sensors(ctx->protocol_handle, type, readings, max_count, actual_count);
    }
    else
    {
        ret = bmc_redfish_read_sensors(ctx->protocol_handle, type, readings, max_count, actual_count);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
    }
    else
    {
        ctx->fail_count++;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 执行原始IPMI命令
 */
int32_t PDL_BMC_ExecuteCommand(bmc_handle_t handle,
                                  const char *cmd,
                                  char *response,
                                  uint32_t resp_size)
{
    if (NULL == handle || NULL == cmd)
    {
        return OSAL_ERR_GENERIC;
    }

    (void)response;
    (void)resp_size;
    return OSAL_ERR_GENERIC;
}

/**
 * @brief 切换通信通道
 */
int32_t PDL_BMC_SwitchChannel(bmc_handle_t handle,
                                 bmc_channel_t channel)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    /* 检查通道是否可用 */
    if (BMC_CHANNEL_NETWORK == channel && NULL == ctx->net_transport_handle)
    {
        LOG_ERROR("BMC", "Network channel not available");
        OSAL_MutexUnlock(ctx->mutex);
        return OSAL_ERR_GENERIC;
    }

    if (BMC_CHANNEL_SERIAL == channel && NULL == ctx->serial_transport_handle)
    {
        LOG_ERROR("BMC", "Serial channel not available");
        OSAL_MutexUnlock(ctx->mutex);
        return OSAL_ERR_GENERIC;
    }

    /* 切换通道 */
    ctx->current_channel = channel;
    ctx->connected = true;
    ctx->switch_count++;

    LOG_INFO("BMC", "Switched to %s channel",
             (channel == BMC_CHANNEL_NETWORK) ? "network" : "serial");

    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 获取当前通道
 */
bmc_channel_t PDL_BMC_GetChannel(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return BMC_CHANNEL_NETWORK;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);
    bmc_channel_t channel = ctx->current_channel;
    OSAL_MutexUnlock(ctx->mutex);

    return channel;
}

/**
 * @brief 检查连接状态
 */
bool PDL_BMC_IsConnected(bmc_handle_t handle)
{
    if (NULL == handle)
    {
        return false;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);
    bool connected = ctx->connected;
    OSAL_MutexUnlock(ctx->mutex);

    return connected;
}

/**
 * @brief 获取服务统计信息
 */
int32_t PDL_BMC_GetStats(bmc_handle_t handle,
                            uint32_t *cmd_count,
                            uint32_t *success_count,
                            uint32_t *fail_count,
                            uint32_t *switch_count)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == cmd_count && NULL == success_count &&
        NULL == fail_count && NULL == switch_count)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    bmc_context_t *ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    if (NULL != cmd_count) *cmd_count = ctx->cmd_count;
    if (NULL != success_count) *success_count = ctx->success_count;
    if (NULL != fail_count) *fail_count = ctx->fail_count;
    if (NULL != switch_count) *switch_count = ctx->switch_count;

    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}
