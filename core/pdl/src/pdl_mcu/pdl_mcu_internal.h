/************************************************************************
 * MCU驱动内部接口
 *
 * 仅供pdl_mcu模块内部使用，不对外暴露
 ************************************************************************/

#ifndef PDL_MCU_INTERNAL_H
#define PDL_MCU_INTERNAL_H

#include "osal.h"

/*
 * MCU命令码定义
 */
#define MCU_CMD_GET_VERSION    0x01   /* 获取版本 */
#define MCU_CMD_GET_STATUS     0x02   /* 获取状态 */
#define MCU_CMD_RESET          0x03   /* 复位 */
#define MCU_CMD_READ_REG       0x10   /* 读寄存器 */
#define MCU_CMD_WRITE_REG      0x11   /* 写寄存器 */
#define MCU_CMD_POWER_ON       0x20   /* 上电 */
#define MCU_CMD_POWER_OFF      0x21   /* 下电 */
#define MCU_CMD_CUSTOM         0xFF   /* 自定义命令 */

/*
 * CAN通信接口（pdl_mcu_can.c实现）
 */
int32_t mcu_can_init(const void *config, void **handle);
int32_t mcu_can_deinit(void *handle);
int32_t mcu_can_send_command(void *handle,
                          uint8_t cmd_code,
                          const uint8_t *data,
                          uint32_t data_len,
                          uint8_t *response,
                          uint32_t resp_size,
                          uint32_t *actual_size,
                          uint32_t timeout_ms);

/*
 * 串口通信接口（pdl_mcu_serial.c实现）
 */
int32_t mcu_serial_init(const void *config, void **handle);
int32_t mcu_serial_deinit(void *handle);
int32_t mcu_serial_send_command(void *handle,
                             uint8_t cmd_code,
                             const uint8_t *data,
                             uint32_t data_len,
                             uint8_t *response,
                             uint32_t resp_size,
                             uint32_t *actual_size,
                             uint32_t timeout_ms);

/*
 * 协议封装/解析接口（pdl_mcu_protocol.c实现）
 * 注意：CRC 强制启用，不再提供配置选项
 */
uint16_t mcu_protocol_calc_crc16(const uint8_t *data, uint32_t len);
int32_t mcu_protocol_pack_frame(uint8_t cmd_code,
                             const uint8_t *data,
                             uint32_t data_len,
                             uint8_t *frame,
                             uint32_t frame_size,
                             uint32_t *actual_size);
int32_t mcu_protocol_unpack_frame(const uint8_t *frame,
                               uint32_t frame_len,
                               uint8_t *cmd_code,
                               uint8_t *data,
                               uint32_t data_size,
                               uint32_t *actual_size);

#endif /* PDL_MCU_INTERNAL_H */
