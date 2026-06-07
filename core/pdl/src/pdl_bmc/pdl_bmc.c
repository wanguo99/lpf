/************************************************************************
 * BMC服务实现
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理通信通道（网络/串口）和协议类型（IPMI/Redfish）
 * - 调度传输层和协议层模块
 ************************************************************************/

#include "pdl_bmc_internal.h"
#include "osal.h"
#include "pdl.h"

/*
 * BMC服务上下文
 */
typedef struct
{
    pdl_bmc_config_t config;

    /* 传输层句柄 */
    void *net_transport_handle;
    void *serial_transport_handle;

    /* 协议层句柄（支持双协议） */
    void *redfish_handle;
    void *ipmi_handle;
    pdl_bmc_protocol_t current_protocol;

    /* 当前通道 */
    pdl_bmc_channel_t current_channel;
    bool connected;

    /* 统计信息 */
    uint32_t cmd_count;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t switch_count;
    uint32_t protocol_switch_count;

    /* 互斥锁 */
    osal_mutex_t *mutex;
} bmc_context_t;

/**
 * @brief 初始化BMC服务
 */
int32_t PDL_BMC_Init(const pdl_bmc_config_t *config,
                        pdl_bmc_handle_t *handle)
{
    bmc_context_t *ctx;
    int32_t ret;
    void *transport_handle;
    int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);

    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 分配上下文 */
    ctx = (bmc_context_t *)OSAL_Malloc(sizeof(bmc_context_t));
    if (NULL == ctx)
    {
        LOG_ERROR("BMC", "Failed to allocate context");
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_context_t));
    OSAL_Memcpy(&ctx->config, config, sizeof(pdl_bmc_config_t));
    ctx->current_channel = config->primary_channel;
    ctx->current_protocol = PDL_BMC_PROTOCOL_REDFISH;

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex))
    {
        LOG_ERROR("BMC", "Failed to create mutex");
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化网络传输层 */
    if (config->network.enabled)
    {
        ret = bmc_transport_net_init(config->network.ip_addr,
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
        ret = bmc_transport_serial_init(config->serial.device,
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

    /* 初始化协议层（优先Redfish，失败则使用IPMI） */
    transport_handle = (ctx->current_channel == PDL_BMC_CHANNEL_NETWORK) ?
                       ctx->net_transport_handle : ctx->serial_transport_handle;

    if (NULL != transport_handle)
    {
        send_recv = (ctx->current_channel == PDL_BMC_CHANNEL_NETWORK) ?
                    bmc_transport_net_send_recv : bmc_transport_serial_send_recv;

        /* 优先尝试初始化Redfish */
        ret = bmc_redfish_init(transport_handle, send_recv,
                              config->network.username,
                              config->network.password,
                              &ctx->redfish_handle);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("BMC", "Redfish protocol initialized");
            ctx->current_protocol = PDL_BMC_PROTOCOL_REDFISH;
            ctx->connected = true;
        }
        else
        {
            LOG_WARN("BMC", "Redfish initialization failed, falling back to IPMI");
        }

        /* 初始化IPMI作为备用协议 */
        ret = bmc_ipmi_init(transport_handle, send_recv, &ctx->ipmi_handle);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("BMC", "IPMI protocol initialized");
            if (!ctx->connected)
            {
                ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
                ctx->connected = true;
            }
        }
        else
        {
            LOG_WARN("BMC", "IPMI initialization failed");
        }
    }

    *handle = (pdl_bmc_handle_t)ctx;
    LOG_INFO("BMC", "BMC service initialized (active protocol: %s)",
             (ctx->current_protocol == PDL_BMC_PROTOCOL_IPMI) ? "IPMI" : "Redfish");

    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化BMC服务
 */
int32_t PDL_BMC_Deinit(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    /* 关闭协议层 */
    if (NULL != ctx->redfish_handle)
    {
        bmc_redfish_deinit(ctx->redfish_handle);
    }

    if (NULL != ctx->ipmi_handle)
    {
        bmc_ipmi_deinit(ctx->ipmi_handle);
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
int32_t PDL_BMC_PowerOn(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    ret = OSAL_ERR_GENERIC;

    /* 优先使用Redfish */
    if (PDL_BMC_PROTOCOL_REDFISH == ctx->current_protocol && NULL != ctx->redfish_handle)
    {
        ret = bmc_redfish_power_on(ctx->redfish_handle);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Redfish power on failed, switching to IPMI");
            ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
            ctx->protocol_switch_count++;
        }
    }

    /* 失败则切换到IPMI */
    if (OSAL_SUCCESS != ret && NULL != ctx->ipmi_handle)
    {
        ret = bmc_ipmi_power_on(ctx->ipmi_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power on command sent (protocol: %s)",
                 (ctx->current_protocol == PDL_BMC_PROTOCOL_IPMI) ? "IPMI" : "Redfish");
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
int32_t PDL_BMC_PowerOff(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    ret = OSAL_ERR_GENERIC;

    /* 优先使用Redfish */
    if (PDL_BMC_PROTOCOL_REDFISH == ctx->current_protocol && NULL != ctx->redfish_handle)
    {
        ret = bmc_redfish_power_off(ctx->redfish_handle);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Redfish power off failed, switching to IPMI");
            ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
            ctx->protocol_switch_count++;
        }
    }

    /* 失败则切换到IPMI */
    if (OSAL_SUCCESS != ret && NULL != ctx->ipmi_handle)
    {
        ret = bmc_ipmi_power_off(ctx->ipmi_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power off command sent (protocol: %s)",
                 (ctx->current_protocol == PDL_BMC_PROTOCOL_IPMI) ? "IPMI" : "Redfish");
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
int32_t PDL_BMC_PowerReset(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    ret = OSAL_ERR_GENERIC;

    /* 优先使用Redfish */
    if (PDL_BMC_PROTOCOL_REDFISH == ctx->current_protocol && NULL != ctx->redfish_handle)
    {
        ret = bmc_redfish_power_reset(ctx->redfish_handle);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Redfish power reset failed, switching to IPMI");
            ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
            ctx->protocol_switch_count++;
        }
    }

    /* 失败则切换到IPMI */
    if (OSAL_SUCCESS != ret && NULL != ctx->ipmi_handle)
    {
        ret = bmc_ipmi_power_reset(ctx->ipmi_handle);
    }

    if (OSAL_SUCCESS == ret)
    {
        ctx->success_count++;
        LOG_INFO("BMC", "Power reset command sent (protocol: %s)",
                 (ctx->current_protocol == PDL_BMC_PROTOCOL_IPMI) ? "IPMI" : "Redfish");
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
int32_t PDL_BMC_GetPowerState(pdl_bmc_handle_t handle,
                                 pdl_bmc_power_state_t *state)
{
    bmc_context_t *ctx;
    int32_t ret;

    if (NULL == handle || NULL == state)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    ret = OSAL_ERR_GENERIC;

    /* 优先使用Redfish */
    if (PDL_BMC_PROTOCOL_REDFISH == ctx->current_protocol && NULL != ctx->redfish_handle)
    {
        ret = bmc_redfish_get_power_state(ctx->redfish_handle, state);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Redfish get power state failed, switching to IPMI");
            ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
            ctx->protocol_switch_count++;
        }
    }

    /* 失败则切换到IPMI */
    if (OSAL_SUCCESS != ret && NULL != ctx->ipmi_handle)
    {
        ret = bmc_ipmi_get_power_state(ctx->ipmi_handle, state);
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
int32_t PDL_BMC_ReadSensors(pdl_bmc_handle_t handle,
                               pdl_bmc_sensor_type_t type,
                               pdl_bmc_sensor_reading_t *readings,
                               uint32_t max_count,
                               uint32_t *actual_count)
{
    bmc_context_t *ctx;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    ctx->cmd_count++;

    ret = OSAL_ERR_GENERIC;

    /* 优先使用Redfish */
    if (PDL_BMC_PROTOCOL_REDFISH == ctx->current_protocol && NULL != ctx->redfish_handle)
    {
        ret = bmc_redfish_read_sensors(ctx->redfish_handle, type, readings, max_count, actual_count);
        if (OSAL_SUCCESS != ret)
        {
            LOG_WARN("BMC", "Redfish read sensors failed, switching to IPMI");
            ctx->current_protocol = PDL_BMC_PROTOCOL_IPMI;
            ctx->protocol_switch_count++;
        }
    }

    /* 失败则切换到IPMI */
    if (OSAL_SUCCESS != ret && NULL != ctx->ipmi_handle)
    {
        ret = bmc_ipmi_read_sensors(ctx->ipmi_handle, type, readings, max_count, actual_count);
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
int32_t PDL_BMC_ExecuteCommand(pdl_bmc_handle_t handle,
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
int32_t PDL_BMC_SwitchChannel(pdl_bmc_handle_t handle,
                                 pdl_bmc_channel_t channel)
{
    bmc_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    /* 检查通道是否可用 */
    if (PDL_BMC_CHANNEL_NETWORK == channel && NULL == ctx->net_transport_handle)
    {
        LOG_ERROR("BMC", "Network channel not available");
        OSAL_MutexUnlock(ctx->mutex);
        return OSAL_ERR_GENERIC;
    }

    if (PDL_BMC_CHANNEL_SERIAL == channel && NULL == ctx->serial_transport_handle)
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
             (channel == PDL_BMC_CHANNEL_NETWORK) ? "network" : "serial");

    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 获取当前通道
 */
pdl_bmc_channel_t PDL_BMC_GetChannel(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;
    pdl_bmc_channel_t channel;

    if (NULL == handle)
    {
        return PDL_BMC_CHANNEL_NETWORK;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);
    channel = ctx->current_channel;
    OSAL_MutexUnlock(ctx->mutex);

    return channel;
}

/**
 * @brief 检查连接状态
 */
bool PDL_BMC_IsConnected(pdl_bmc_handle_t handle)
{
    bmc_context_t *ctx;
    bool connected;

    if (NULL == handle)
    {
        return false;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);
    connected = ctx->connected;
    OSAL_MutexUnlock(ctx->mutex);

    return connected;
}

/**
 * @brief 获取服务统计信息
 */
int32_t PDL_BMC_GetStats(pdl_bmc_handle_t handle,
                            uint32_t *cmd_count,
                            uint32_t *success_count,
                            uint32_t *fail_count,
                            uint32_t *switch_count)
{
    bmc_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == cmd_count && NULL == success_count &&
        NULL == fail_count && NULL == switch_count)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    ctx = (bmc_context_t *)handle;

    OSAL_MutexLock(ctx->mutex);

    if (NULL != cmd_count) *cmd_count = ctx->cmd_count;
    if (NULL != success_count) *success_count = ctx->success_count;
    if (NULL != fail_count) *fail_count = ctx->fail_count;
    if (NULL != switch_count) *switch_count = ctx->switch_count;

    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_SUCCESS;
}
