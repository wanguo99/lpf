/************************************************************************
 * MCU串口通信实现
 *
 * 职责：
 * - 封装串口收发
 * - 实现MCU的串口协议（帧格式、起止符、CRC校验）
 * - 超时和重试机制
 ************************************************************************/

#include "pdl_mcu_internal.h"
#include "hal.h"
#include "osal.h"
#include "pdl.h"

/*
 * 串口协议帧格式：
 * [0xAA][0x55][cmd][len][data...][crc16_h][crc16_l]
 */
#define FRAME_HEADER_0    0xAA
#define FRAME_HEADER_1    0x55
#define FRAME_HEADER_SIZE 2
#define FRAME_CRC_SIZE    2
#define FRAME_OVERHEAD    (FRAME_HEADER_SIZE + 2 + FRAME_CRC_SIZE)  /* 头+cmd+len+CRC */

/*
 * 串口通信上下文
 */
typedef struct
{
    hal_serial_handle_t serial_handle;
    osal_mutex_t *rx_mutex;
    /* 注意：CRC 强制启用，不再提供配置选项 */
} mcu_serial_context_t;

/**
 * @brief 初始化串口通信
 */
int32_t mcu_serial_init(const void *config, void **handle)
{
    const pdl_mcu_config_t *mcu_cfg;
    mcu_serial_context_t *ctx;
    hal_serial_config_t serial_config;

    if (NULL == config || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    mcu_cfg = (const pdl_mcu_config_t *)config;
    ctx = (mcu_serial_context_t *)OSAL_Malloc(sizeof(mcu_serial_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(mcu_serial_context_t));
    /* CRC 强制启用 */

    /* 打开串口设备 */
    serial_config.baud_rate = mcu_cfg->serial.baudrate;
    serial_config.data_bits = mcu_cfg->serial.data_bits;
    serial_config.stop_bits = mcu_cfg->serial.stop_bits;
    serial_config.parity = mcu_cfg->serial.parity;
    serial_config.flow_control = 0;  /* NONE */

    if (OSAL_SUCCESS != HAL_Serial_Open(mcu_cfg->serial.device, &serial_config, &ctx->serial_handle))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    /* 创建接收互斥锁 */
    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->rx_mutex))
    {
        HAL_Serial_Close(ctx->serial_handle);
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化串口通信
 */
int32_t mcu_serial_deinit(void *handle)
{
    mcu_serial_context_t *ctx;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_serial_context_t *)handle;

    HAL_Serial_Close(ctx->serial_handle);
    OSAL_MutexDelete(ctx->rx_mutex);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 封装串口帧
 */
static int32_t mcu_serial_pack_frame(uint8_t cmd_code,
                                   const uint8_t *data,
                                   uint32_t data_len,
                                   uint8_t *frame,
                                   uint32_t frame_size,
                                   uint32_t *actual_size)
{
    uint32_t required_size;
    uint32_t pos;
    uint16_t crc;

    required_size = FRAME_OVERHEAD + data_len;
    if (frame_size < required_size)
    {
        return OSAL_ERR_GENERIC;
    }

    pos = 0;

    /* 帧头 */
    frame[pos++] = FRAME_HEADER_0;
    frame[pos++] = FRAME_HEADER_1;

    /* 命令和长度 */
    frame[pos++] = cmd_code;
    frame[pos++] = (uint8_t)data_len;

    /* 数据 */
    if (NULL != data && data_len > 0)
    {
        OSAL_Memcpy(&frame[pos], data, data_len);
        pos += data_len;
    }

    /* CRC校验（强制启用） */
    crc = mcu_protocol_calc_crc16(&frame[FRAME_HEADER_SIZE], pos - FRAME_HEADER_SIZE);
    frame[pos++] = (uint8_t)(crc >> 8);
    frame[pos++] = (uint8_t)(crc & 0xFF);

    *actual_size = pos;
    return OSAL_SUCCESS;
}

/**
 * @brief 解析串口帧
 */
static int32_t mcu_serial_unpack_frame(const uint8_t *frame,
                                     uint32_t frame_len,
                                     uint8_t *status,
                                     uint8_t *data,
                                     uint32_t data_size,
                                     uint32_t *actual_size)
{
    uint16_t crc_recv;
    uint16_t crc_calc;
    uint8_t data_len;
    uint32_t copy_len;

    /* 最小帧长度检查 */
    if (frame_len < FRAME_OVERHEAD)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 帧头检查 */
    if (frame[0] != FRAME_HEADER_0 || frame[1] != FRAME_HEADER_1)
    {
        return OSAL_ERR_GENERIC;
    }

    /* CRC校验（强制启用） */
    crc_recv = (frame[frame_len - 2] << 8) | frame[frame_len - 1];
    crc_calc = mcu_protocol_calc_crc16(&frame[FRAME_HEADER_SIZE], frame_len - FRAME_OVERHEAD);
    if (crc_recv != crc_calc)
    {
        return OSAL_ERR_GENERIC;  /* CRC 校验失败 */
    }

    /* 解析状态和数据 */
    *status = frame[2];
    data_len = frame[3];

    if (NULL != data && data_len > 0)
    {
        copy_len = (data_len < data_size) ? data_len : data_size;
        OSAL_Memcpy(data, &frame[4], copy_len);
        if (NULL != actual_size)
        {
            *actual_size = copy_len;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 发送命令并接收响应
 */
int32_t mcu_serial_send_command(void *handle,
                             uint8_t cmd_code,
                             const uint8_t *data,
                             uint32_t data_len,
                             uint8_t *response,
                             uint32_t resp_size,
                             uint32_t *actual_size,
                             uint32_t timeout_ms)
{
    mcu_serial_context_t *ctx;
    uint8_t tx_frame[256];
    uint32_t tx_len;
    uint8_t rx_frame[256];
    int32_t rx_len;
    uint8_t status;
    int32_t ret;
    uint64_t start_time_us;
    uint64_t elapsed_us;
    uint32_t remaining_timeout_ms;

    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (mcu_serial_context_t *)handle;

    /* 记录起始时间，用于总超时控制 */
    start_time_us = OSAL_GetMonotonicTime();

    /* 封装发送帧 */
    if (OSAL_SUCCESS != mcu_serial_pack_frame(cmd_code, data, data_len,
                              tx_frame, sizeof(tx_frame), &tx_len))
    {
        return OSAL_ERR_GENERIC;
    }

    /* 发送 */
    if (HAL_Serial_Write(ctx->serial_handle, tx_frame, tx_len, timeout_ms) != (int32_t)tx_len)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 计算发送后剩余的超时时间 */
    elapsed_us = OSAL_GetMonotonicTime() - start_time_us;
    if (elapsed_us / 1000 >= timeout_ms)
    {
        return OSAL_ERR_TIMEOUT;  /* 发送阶段已超时 */
    }
    remaining_timeout_ms = timeout_ms - (uint32_t)(elapsed_us / 1000);

    /* 接收响应，使用剩余超时时间 */
    OSAL_MutexLock(ctx->rx_mutex);

    rx_len = HAL_Serial_Read(ctx->serial_handle, rx_frame, sizeof(rx_frame), remaining_timeout_ms);

    if (rx_len > 0)
    {
        ret = mcu_serial_unpack_frame(rx_frame, rx_len,
                                            &status, response, resp_size, actual_size);

        OSAL_MutexUnlock(ctx->rx_mutex);

        if (OSAL_SUCCESS != ret || OSAL_SUCCESS != status)
        {
            return OSAL_ERR_GENERIC;
        }

        return OSAL_SUCCESS;
    }

    OSAL_MutexUnlock(ctx->rx_mutex);

    return (0 == rx_len) ? OSAL_ERR_TIMEOUT : OSAL_ERR_GENERIC;
}
