/************************************************************************
 * MCU CAN通信实现
 *
 * 职责：
 * - 封装CAN总线收发
 * - 实现MCU的CAN协议（帧格式、ID映射）
 * - 超时和重试机制
 ************************************************************************/

#include "pdl_mcu_internal.h"
#include "pdl_mcu.h"
#include "hal_can.h"
#include "osal.h"

/*
 * CAN通信上下文
 */
typedef struct
{
    hal_can_handle_t can_handle;
    uint32_t tx_id;
    uint32_t rx_id;
    osal_mutex_t *rx_mutex;
} mcu_can_context_t;

/**
 * @brief 初始化CAN通信
 */
int32_t mcu_can_init(const void *config, void **handle)
{
    const mcu_config_t *mcu_cfg;
    mcu_can_context_t *ctx;
    hal_can_config_t can_config;

    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_cfg = (const mcu_config_t *)config;
    ctx = (mcu_can_context_t *)OSAL_Malloc(sizeof(mcu_can_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(mcu_can_context_t));
    ctx->tx_id = mcu_cfg->can.tx_id;
    ctx->rx_id = mcu_cfg->can.rx_id;

    /* 打开CAN设备 */
    can_config.interface = mcu_cfg->can.device;
    can_config.baudrate = mcu_cfg->can.bitrate;
    can_config.rx_timeout = 1000;
    can_config.tx_timeout = 1000;

    if (OSAL_SUCCESS != HAL_CAN_Init(&can_config, &ctx->can_handle))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 创建接收互斥锁 */
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->rx_mutex))
    {
        HAL_CAN_Deinit(ctx->can_handle);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化CAN通信
 */
int32_t mcu_can_deinit(void *handle)
{
    mcu_can_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_can_context_t *)handle;

    HAL_CAN_Deinit(ctx->can_handle);
    OSAL_MutexDelete(ctx->rx_mutex);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 发送命令并接收响应
 */
int32_t mcu_can_send_command(void *handle,
                          uint8_t cmd_code,
                          const uint8_t *data,
                          uint32_t data_len,
                          uint8_t *response,
                          uint32_t resp_size,
                          uint32_t *actual_size,
                          uint32_t timeout_ms)
{
    mcu_can_context_t *ctx;
    uint8_t tx_frame[8];
    uint32_t tx_len;
    uint32_t copy_len;
    can_frame_t can_frame;
    can_frame_t rx_frame;
    int32_t ret;
    uint8_t status;
    uint8_t resp_len;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_can_context_t *)handle;

    /* 封装CAN帧：[cmd_code][data_len][data...] */
    tx_len = 0;

    tx_frame[tx_len++] = cmd_code;
    tx_frame[tx_len++] = (uint8_t)data_len;

    if (NULL != data && data_len > 0)
    {
        copy_len = (data_len > 6) ? 6 : data_len;  /* CAN最多8字节，留2字节给头 */
        OSAL_Memcpy(&tx_frame[tx_len], data, copy_len);
        tx_len += copy_len;
    }

    /* 发送CAN帧 */
    can_frame.can_id = ctx->tx_id;
    can_frame.dlc = tx_len;
    OSAL_Memcpy(can_frame.data, tx_frame, tx_len);

    if (OSAL_SUCCESS != HAL_CAN_Send(ctx->can_handle, &can_frame))
    {
        return OSAL_ERR_GENERIC;
    }

    /* 接收响应 */
    OSAL_MutexLock(ctx->rx_mutex);

    ret = HAL_CAN_Recv(ctx->can_handle, &rx_frame, timeout_ms);

    if (OSAL_SUCCESS == ret)
    {
        /* 检查CAN ID */
        if (rx_frame.can_id != ctx->rx_id)
        {
            OSAL_MutexUnlock(ctx->rx_mutex);
            return OSAL_ERR_GENERIC;
        }

        /* 解析响应：[status][data_len][data...] */
        if (rx_frame.dlc >= 2)
        {
            status = rx_frame.data[0];
            resp_len = rx_frame.data[1];

            if (0 != status)
            {
                OSAL_MutexUnlock(ctx->rx_mutex);
                return OSAL_ERR_GENERIC;
            }

            if (NULL != response && resp_len > 0)
            {
                copy_len = (resp_len < resp_size) ? resp_len : resp_size;
                copy_len = (copy_len < ((uint32_t)rx_frame.dlc - 2)) ? copy_len : ((uint32_t)rx_frame.dlc - 2);
                OSAL_Memcpy(response, &rx_frame.data[2], copy_len);

                if (NULL != actual_size)
                {
                    *actual_size = copy_len;
                }
            }
        }
    }

    OSAL_MutexUnlock(ctx->rx_mutex);

    return ret;
}
