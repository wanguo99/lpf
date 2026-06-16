/************************************************************************
 * MCU外设驱动实现（主文件）
 *
 * 职责：
 * - 实现对外业务接口
 * - 管理MCU上下文和状态
 * - 调度内部通信模块（CAN/串口）
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"
#include "hal.h"
#include "pdl.h"
#include "pdl_mcu_internal.h"
#include "pdl_mcu_protocol.h"

/*
 * MCU驱动上下文
 */
typedef struct
{
    pconfig_mcu_config_t config;     /* 使用 PCONFIG 配置类型 */
    pconfig_mcu_interface_t interface;
    void *comm_handle;                /* 通信句柄（CAN/串口） */
    bool initialized;
    pdl_mcu_state_t state;            /* 设备状态 */
    osal_mutex_t mutex;
    pdl_mcu_version_t version;
    pdl_mcu_status_t status;
} mcu_context_t;

/**
 * @brief 初始化MCU驱动
 */
int32_t PDL_MCU_init(uint32_t index, pdl_mcu_handle_t *handle)
{
    mcu_context_t *ctx;
    const pconfig_platform_config_t *platform;
    const pconfig_mcu_entry_t *mcu_entry;
    const pconfig_mcu_config_t *config;
    int32_t ret;

    if (NULL == handle)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 从 PCONFIG 获取平台配置 */
    platform = PCONFIG_GetBoard();
    if (NULL == platform)
    {
        LOG_ERROR("PDL_MCU", "No platform config registered");
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 从 PCONFIG 获取 MCU 配置 */
    mcu_entry = PCONFIG_HW_GetMCU(platform, index);
    if (NULL == mcu_entry)
    {
        LOG_ERROR("PDL_MCU", "MCU config not found for index %u", index);
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 检查是否启用 */
    if (!mcu_entry->enabled)
    {
        LOG_WARN("PDL_MCU", "MCU index %u is disabled in config", index);
        return OSAL_ERR_GENERIC;
    }

    config = &mcu_entry->config;

    LOG_INFO("PDL_MCU", "Initializing MCU index %u: %s", index, mcu_entry->description);

    ctx = (mcu_context_t *)OSAL_malloc(OSAL_sizeof(mcu_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_NO_MEMORY;
    }

    OSAL_memset(ctx, 0, OSAL_sizeof(mcu_context_t));
    OSAL_memcpy(&ctx->config, config, OSAL_sizeof(pconfig_mcu_config_t));
    ctx->interface = config->interface;

    /* 创建互斥锁 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_init(&ctx->mutex, NULL))
    {
        OSAL_free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 初始化通信接口（转换 PCONFIG 配置到 HAL 配置） */
    ret = OSAL_ERR_GENERIC;
    switch (config->interface)
    {
        case PCONFIG_MCU_INTERFACE_CAN:
            ret = mcu_can_init(&config->hw.can, &ctx->comm_handle);
            break;
        case PCONFIG_MCU_INTERFACE_SERIAL:
            ret = mcu_serial_init(&config->hw.serial, &ctx->comm_handle);
            break;
        case PCONFIG_MCU_INTERFACE_I2C:
        case PCONFIG_MCU_INTERFACE_SPI:
            /* TODO: 实现I2C/SPI接口 */
            LOG_ERROR("PDL_MCU", "I2C/SPI interface not supported yet");
            ret = OSAL_ERR_NOT_SUPPORTED;
            break;
        default:
            LOG_ERROR("PDL_MCU", "Unknown interface type: %d", config->interface);
            ret = OSAL_ERR_GENERIC;
            break;
    }

    if (OSAL_SUCCESS != ret)
    {
        OSAL_pthread_mutex_destroy(&ctx->mutex);
        OSAL_free(ctx);
        return ret;
    }

    ctx->initialized = true;
    ctx->state = PDL_MCU_STATE_INIT;  /* 初始化完成，进入 INIT 状态 */
    *handle = (pdl_mcu_handle_t)ctx;

    LOG_INFO("PDL_MCU", "MCU index %u initialized successfully", index);

    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化MCU驱动
 */
int32_t PDL_MCU_deinit(pdl_mcu_handle_t handle)
{
    mcu_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 关闭通信接口 */
    switch (ctx->interface)
    {
        case PCONFIG_MCU_INTERFACE_CAN:
            mcu_can_deinit(ctx->comm_handle);
            break;
        case PCONFIG_MCU_INTERFACE_SERIAL:
            mcu_serial_deinit(ctx->comm_handle);
            break;
        default:
            break;
    }

    OSAL_pthread_mutex_destroy(&ctx->mutex);
    OSAL_free(ctx);

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
    pconfig_mcu_interface_t interface;
    void *comm_handle;
    uint32_t timeout_ms;

    /* 缩小锁范围：只在读取上下文时加锁 */
    OSAL_pthread_mutex_lock(&ctx->mutex);
    interface = ctx->interface;
    comm_handle = ctx->comm_handle;
    timeout_ms = ctx->config.cmd_timeout_ms;

    /* 进入 BUSY 状态 */
    ctx->state = PDL_MCU_STATE_BUSY;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    /* 发送命令时不持有锁，由 HAL 层提供线程安全保护 */
    switch (interface)
    {
        case PCONFIG_MCU_INTERFACE_CAN:
            ret = mcu_can_send_command(comm_handle, cmd_code, data, data_len,
                                      response, resp_size, actual_size,
                                      timeout_ms);
            break;
        case PCONFIG_MCU_INTERFACE_SERIAL:
            ret = mcu_serial_send_command(comm_handle, cmd_code, data, data_len,
                                         response, resp_size, actual_size,
                                         timeout_ms);
            break;
        default:
            ret = OSAL_ERR_GENERIC;
            break;
    }

    /* 根据结果更新状态 */
    OSAL_pthread_mutex_lock(&ctx->mutex);
    if (OSAL_SUCCESS == ret) {
        ctx->state = PDL_MCU_STATE_READY;  /* 命令成功，设备就绪 */
    } else if (OSAL_ERR_TIMEOUT == ret) {
        ctx->state = PDL_MCU_STATE_OFFLINE;  /* 超时，设备离线 */
    } else {
        ctx->state = PDL_MCU_STATE_ERROR;  /* 其他错误 */
    }
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return ret;
}

/**
 * @brief 获取MCU版本
 */
int32_t PDL_MCU_get_version(pdl_mcu_handle_t handle, pdl_mcu_version_t *version)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len, ret;
    uint32_t actual_size;

    if (NULL == handle || NULL == version)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_get_version(tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求并接收响应 */
    ret = mcu_send_command_internal(ctx, MCU_CMD_GET_VERSION,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &actual_size);

    if (OSAL_SUCCESS == ret)
    {
        /* 使用 PRL 协议解码响应 */
        ret = pdl_mcu_decode_get_version(rx_buf, actual_size, version);
    }

    return ret;
}

/**
 * @brief 获取MCU状态
 */
int32_t PDL_MCU_get_status(pdl_mcu_handle_t handle, pdl_mcu_status_t *status)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len, ret;
    uint32_t actual_size;

    if (NULL == handle || NULL == status)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_get_status(tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求并接收响应 */
    ret = mcu_send_command_internal(ctx, MCU_CMD_GET_STATUS,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &actual_size);

    if (OSAL_SUCCESS == ret)
    {
        /* 使用 PRL 协议解码响应 */
        ret = pdl_mcu_decode_get_status(rx_buf, actual_size, status);
        if (OSAL_SUCCESS == ret)
        {
            status->timestamp_us = OSAL_get_monotonic_time();
            /* 同步设备状态 */
            OSAL_pthread_mutex_lock(&ctx->mutex);
            status->state = ctx->state;
            OSAL_pthread_mutex_unlock(&ctx->mutex);
        }
    }
    else
    {
        status->online = false;
        /* 同步设备状态 */
        OSAL_pthread_mutex_lock(&ctx->mutex);
        status->state = ctx->state;
        OSAL_pthread_mutex_unlock(&ctx->mutex);
    }

    return ret;
}

/**
 * @brief MCU复位
 */
int32_t PDL_MCU_reset(pdl_mcu_handle_t handle)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len;
    uint32_t actual_size;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_reset(tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求 */
    return mcu_send_command_internal(ctx, MCU_CMD_RESET,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &actual_size);
}

/**
 * @brief 读取MCU寄存器
 */
int32_t PDL_MCU_ReadRegister(pdl_mcu_handle_t handle, uint8_t reg_addr, uint8_t *value)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len, ret;
    uint32_t actual_size;

    if (NULL == handle || NULL == value)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_read_register(reg_addr, tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求并接收响应 */
    ret = mcu_send_command_internal(ctx, MCU_CMD_READ_REG,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &actual_size);

    if (OSAL_SUCCESS == ret)
    {
        /* 使用 PRL 协议解码响应 */
        ret = pdl_mcu_decode_read_register(rx_buf, actual_size, value);
    }

    return ret;
}

/**
 * @brief 写入MCU寄存器
 */
int32_t PDL_MCU_WriteRegister(pdl_mcu_handle_t handle, uint8_t reg_addr, uint8_t value)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    int32_t tx_len;
    uint32_t actual_size;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_write_register(reg_addr, value, tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求 */
    return mcu_send_command_internal(ctx, MCU_CMD_WRITE_REG,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &actual_size);
}

/**
 * @brief 发送自定义命令到MCU
 */
int32_t PDL_MCU_send_command(pdl_mcu_handle_t handle,
                          uint8_t cmd_code,
                          const uint8_t *data,
                          uint32_t data_len,
                          uint8_t *response,
                          uint32_t resp_size,
                          uint32_t *actual_size)
{
    mcu_context_t *ctx;
    uint8_t tx_buf[512];
    uint8_t rx_buf[512];
    int32_t tx_len, ret;
    uint32_t rx_size;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_context_t *)handle;

    /* 使用 PRL 协议编码请求 */
    tx_len = pdl_mcu_encode_custom_command(cmd_code, data, data_len,
                                           tx_buf, OSAL_sizeof(tx_buf));
    if (tx_len < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送请求并接收响应 */
    ret = mcu_send_command_internal(ctx, cmd_code,
                                    tx_buf, (uint32_t)tx_len,
                                    rx_buf, OSAL_sizeof(rx_buf), &rx_size);

    if (OSAL_SUCCESS == ret)
    {
        /* 使用 PRL 协议解码响应 */
        ret = pdl_mcu_decode_response(rx_buf, rx_size, response, resp_size, actual_size);
    }

    return ret;
}

/**
 * @brief MCU固件升级
 */
int32_t PDL_MCU_FirmwareUpdate(pdl_mcu_handle_t handle,
                             const char *firmware_path,
                             void (*progress_callback)(uint32_t percent))
{
    (void)handle;
    (void)firmware_path;
    (void)progress_callback;
    /* TODO: 实现固件升级逻辑 */
    return OSAL_ERR_GENERIC;
}

/**
 * @brief 获取MCU设备状态
 */
pdl_mcu_state_t PDL_MCU_GetDeviceState(pdl_mcu_handle_t handle)
{
    mcu_context_t *ctx;
    pdl_mcu_state_t state;

    if (NULL == handle)
    {
        return PDL_MCU_STATE_UNINITIALIZED;
    }

    ctx = (mcu_context_t *)handle;

    OSAL_pthread_mutex_lock(&ctx->mutex);
    state = ctx->state;
    OSAL_pthread_mutex_unlock(&ctx->mutex);

    return state;
}

/**
 * @brief 获取设备状态名称
 */
const char* PDL_MCU_GetStateName(pdl_mcu_state_t state)
{
    static const char *state_names[] = {
        "UNINITIALIZED",  /* 0 */
        "INIT",           /* 1 */
        "READY",          /* 2 */
        "BUSY",           /* 3 */
        "ERROR",          /* 4 */
        "OFFLINE"         /* 5 */
    };

    if (state >= 0 && state < (int32_t)(OSAL_sizeof(state_names) / OSAL_sizeof(state_names[0])))
    {
        return state_names[state];
    }

    return "UNKNOWN";
}

/**
 * @brief MCU 测试调用链实现（调试接口）
 */
int32_t PDL_MCU_test_call(uint32_t index)
{
	const pconfig_platform_config_t *platform;
	const pconfig_mcu_entry_t *mcu_entry;
	const pconfig_mcu_config_t *config;
	int32_t ret;

	LOG_INFO("PDL_MCU", "========================================");
	LOG_INFO("PDL_MCU", "PDL Layer: MCU TestCall");
	LOG_INFO("PDL_MCU", "Device Index: %u", index);

	/* 从 PCONFIG 获取平台配置 */
	platform = PCONFIG_GetBoard();
	if (NULL == platform) {
		LOG_ERROR("PDL_MCU", "No platform config registered");
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	/* 从 PCONFIG 获取 MCU 配置 */
	mcu_entry = PCONFIG_HW_GetMCU(platform, index);
	if (NULL == mcu_entry) {
		LOG_ERROR("PDL_MCU", "MCU config not found for index %u", index);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	config = &mcu_entry->config;

	LOG_INFO("PDL_MCU", "MCU Name: %s", config->name);
	LOG_INFO("PDL_MCU", "Interface: %d", config->interface);

	/* 根据接口类型调用 HAL 层测试 */
	switch (config->interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
		LOG_INFO("PDL_MCU", "Calling HAL_CAN_test_call...");
		LOG_INFO("PDL_MCU", "CAN Device: %s", config->hw.can.device);
		LOG_INFO("PDL_MCU", "CAN Bitrate: %u", config->hw.can.bitrate);
		ret = HAL_CAN_test_call(NULL);
		break;

	case PCONFIG_MCU_INTERFACE_SERIAL:
		LOG_INFO("PDL_MCU", "Calling HAL_SERIAL_test_call...");
		LOG_INFO("PDL_MCU", "Serial Device: %s", config->hw.serial.device);
		LOG_INFO("PDL_MCU", "Serial Baudrate: %u", config->hw.serial.baudrate);
		ret = HAL_SERIAL_test_call(NULL);
		break;

	default:
		LOG_ERROR("PDL_MCU", "Unsupported interface type: %d", config->interface);
		ret = OSAL_ERR_NOT_SUPPORTED;
		break;
	}

	if (OSAL_SUCCESS == ret) {
		LOG_INFO("PDL_MCU", "PDL_MCU_test_call() completed successfully");
	} else {
		LOG_ERROR("PDL_MCU", "PDL_MCU_test_call() failed: %d", ret);
	}

	LOG_INFO("PDL_MCU", "========================================");

	return ret;
}

