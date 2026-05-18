/************************************************************************
 * MCU外设驱动实现（主文件）
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理MCU上下文和状态
 * - 调度内部通信模块（CAN/串口）
 ************************************************************************/

#include "pdl_mcu.h"
#include "pdl_mcu_internal.h"
#include "osal.h"

/*
 * MCU驱动上下文
 */
typedef struct
{
    mcu_config_t config;
    mcu_interface_t interface;
    void *comm_handle;                /* 通信句柄（CAN/串口） */
    bool initialized;
    osal_mutex_t *mutex;
    mcu_version_t version;
    mcu_status_t status;
} mcu_context_t;

/**
 * @brief 初始化MCU驱动
 */
int32_t PDL_MCU_Init(const mcu_config_t *config, mcu_handle_t *handle)
{
    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)OSAL_Malloc(sizeof(mcu_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(mcu_context_t));
    OSAL_Memcpy(&ctx->config, config, sizeof(mcu_config_t));
    ctx->interface = config->interface;

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化通信接口 */
    int32_t ret = OSAL_ERR_GENERIC;
    switch (config->interface)
    {
        case MCU_INTERFACE_CAN:
            ret = mcu_can_init(&config->can, &ctx->comm_handle);
            break;
        case MCU_INTERFACE_SERIAL:
            ret = mcu_serial_init(&config->serial, &ctx->comm_handle);
            break;
        case MCU_INTERFACE_I2C:
        case MCU_INTERFACE_SPI:
            /* TODO: 实现I2C/SPI接口 */
            ret = OSAL_ERR_GENERIC;
            break;
        default:
            ret = OSAL_ERR_GENERIC;
            break;
    }

    if (OSAL_SUCCESS != ret)
    {
        OSAL_MutexDelete(ctx->mutex);
        OSAL_Free(ctx);
        return ret;
    }

    ctx->initialized = true;
    *handle = (mcu_handle_t)ctx;

    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化MCU驱动
 */
int32_t PDL_MCU_Deinit(mcu_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;

    /* 关闭通信接口 */
    switch (ctx->interface)
    {
        case MCU_INTERFACE_CAN:
            mcu_can_deinit(ctx->comm_handle);
            break;
        case MCU_INTERFACE_SERIAL:
            mcu_serial_deinit(ctx->comm_handle);
            break;
        default:
            break;
    }

    OSAL_MutexDelete(ctx->mutex);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 发送命令到MCU（内部统一接口）
 */
static int32_t mcu_send_command_internal(mcu_context_t *ctx,
                                      uint8_t cmd_code,
                                      const uint8_t *data,
                                      uint32_t data_len,
                                      uint8_t *response,
                                      uint32_t resp_size,
                                      uint32_t *actual_size)
{
    int32_t ret = OSAL_ERR_GENERIC;

    OSAL_MutexLock(ctx->mutex);

    switch (ctx->interface)
    {
        case MCU_INTERFACE_CAN:
            ret = mcu_can_send_command(ctx->comm_handle, cmd_code, data, data_len,
                                      response, resp_size, actual_size,
                                      ctx->config.cmd_timeout_ms);
            break;
        case MCU_INTERFACE_SERIAL:
            ret = mcu_serial_send_command(ctx->comm_handle, cmd_code, data, data_len,
                                         response, resp_size, actual_size,
                                         ctx->config.cmd_timeout_ms);
            break;
        default:
            ret = OSAL_ERR_GENERIC;
            break;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 获取MCU版本
 */
int32_t PDL_MCU_GetVersion(mcu_handle_t handle, mcu_version_t *version)
{
    if (NULL == handle || NULL == version)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;
    uint8_t resp[4];
    uint32_t actual_size;

    int32_t ret = mcu_send_command_internal(ctx, MCU_CMD_GET_VERSION,
                                         NULL, 0, resp, sizeof(resp), &actual_size);

    if (OSAL_SUCCESS == ret && actual_size >= 4)
    {
        version->major = resp[0];
        version->minor = resp[1];
        version->patch = resp[2];
        version->build = resp[3];
        OSAL_Snprintf(version->version_string, sizeof(version->version_string),
                "%d.%d.%d.%d", version->major, version->minor,
                version->patch, version->build);
    }

    return ret;
}

/**
 * @brief 获取MCU状态
 */
int32_t PDL_MCU_GetStatus(mcu_handle_t handle, mcu_status_t *status)
{
    if (NULL == handle || NULL == status)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;
    uint8_t resp[16];
    uint32_t actual_size;

    int32_t ret = mcu_send_command_internal(ctx, MCU_CMD_GET_STATUS,
                                         NULL, 0, resp, sizeof(resp), &actual_size);

    if (OSAL_SUCCESS == ret && actual_size >= 8)
    {
        status->online = true;
        status->uptime_sec = (resp[0] << 24) | (resp[1] << 16) |
                            (resp[2] << 8) | resp[3];
        status->error_code = resp[4];
        status->temperature = (float)resp[5];
        status->voltage_mv = (resp[6] << 8) | resp[7];
    }
    else
    {
        status->online = false;
    }

    return ret;
}

/**
 * @brief MCU复位
 */
int32_t PDL_MCU_Reset(mcu_handle_t handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;
    uint8_t resp[4];
    uint32_t actual_size;

    return mcu_send_command_internal(ctx, MCU_CMD_RESET,
                                    NULL, 0, resp, sizeof(resp), &actual_size);
}

/**
 * @brief 读取MCU寄存器
 */
int32_t PDL_MCU_ReadRegister(mcu_handle_t handle, uint8_t reg_addr, uint8_t *value)
{
    if (NULL == handle || NULL == value)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;
    uint8_t resp[4];
    uint32_t actual_size;

    int32_t ret = mcu_send_command_internal(ctx, MCU_CMD_READ_REG,
                                         &reg_addr, 1, resp, sizeof(resp), &actual_size);

    if (OSAL_SUCCESS == ret && actual_size >= 1)
    {
        *value = resp[0];
    }

    return ret;
}

/**
 * @brief 写入MCU寄存器
 */
int32_t PDL_MCU_WriteRegister(mcu_handle_t handle, uint8_t reg_addr, uint8_t value)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;
    uint8_t data[2] = {reg_addr, value};
    uint8_t resp[4];
    uint32_t actual_size;

    return mcu_send_command_internal(ctx, MCU_CMD_WRITE_REG,
                                    data, 2, resp, sizeof(resp), &actual_size);
}

/**
 * @brief 发送自定义命令到MCU
 */
int32_t PDL_MCU_SendCommand(mcu_handle_t handle,
                          uint8_t cmd_code,
                          const uint8_t *data,
                          uint32_t data_len,
                          uint8_t *response,
                          uint32_t resp_size,
                          uint32_t *actual_size)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_context_t *ctx = (mcu_context_t *)handle;

    return mcu_send_command_internal(ctx, cmd_code, data, data_len,
                                    response, resp_size, actual_size);
}

/**
 * @brief MCU固件升级
 */
int32_t PDL_MCU_FirmwareUpdate(mcu_handle_t handle,
                             const char *firmware_path,
                             void (*progress_callback)(uint32_t percent))
{
    (void)handle;
    (void)firmware_path;
    (void)progress_callback;
    /* TODO: 实现固件升级逻辑 */
    return OSAL_ERR_GENERIC;
}
