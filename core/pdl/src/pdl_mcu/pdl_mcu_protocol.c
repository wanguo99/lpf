/************************************************************************
 * MCU协议工具函数
 *
 * 职责：
 * - CRC16校验计算
 * - 通用帧封装/解析（如果需要）
 ************************************************************************/

#include "pdl_mcu_internal.h"

/**
 * @brief 计算CRC16校验（MODBUS标准）
 *
 * 多项式：0xA001
 * 初始值：0xFFFF
 */
uint16_t mcu_protocol_calc_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;

    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int32_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief 封装通用帧（预留，目前由各通信模块自己实现）
 */
int32_t mcu_protocol_pack_frame(uint8_t cmd_code,
                             const uint8_t *data,
                             uint32_t data_len,
                             bool enable_crc,
                             uint8_t *frame,
                             uint32_t frame_size,
                             uint32_t *actual_size)
{
    (void)cmd_code;
    (void)data;
    (void)data_len;
    (void)enable_crc;
    (void)frame;
    (void)frame_size;
    (void)actual_size;
    /* 预留接口 */
    return OSAL_ERR_GENERIC;
}

/**
 * @brief 解析通用帧（预留，目前由各通信模块自己实现）
 */
int32_t mcu_protocol_unpack_frame(const uint8_t *frame,
                               uint32_t frame_len,
                               bool enable_crc,
                               uint8_t *cmd_code,
                               uint8_t *data,
                               uint32_t data_size,
                               uint32_t *actual_size)
{
    (void)frame;
    (void)frame_len;
    (void)enable_crc;
    (void)cmd_code;
    (void)data;
    (void)data_size;
    (void)actual_size;
    /* 预留接口 */
    return OSAL_ERR_GENERIC;
}
