/**
 * @file pdl_mcu_protocol.h
 * @brief MCU Protocol Wrapper
 * @details PDL_MCU 协议封装层，调用 PRL_DEVICE 统一协议
 */

#ifndef PDL_MCU_PROTOCOL_H
#define PDL_MCU_PROTOCOL_H

#include "pdl/pdl.h"
#include "prl/prl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 编码 MCU 版本查询请求
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_get_version(uint8_t *buffer, size_t buffer_size);

/**
 * @brief 解码 MCU 版本查询响应
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param version 输出：版本信息
 * @return 成功返回 0，失败返回负数错误码
 */
int32_t pdl_mcu_decode_get_version(const uint8_t *packet, size_t packet_len,
                                    pdl_mcu_version_t *version);

/**
 * @brief 编码 MCU 状态查询请求
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_get_status(uint8_t *buffer, size_t buffer_size);

/**
 * @brief 解码 MCU 状态查询响应
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param status 输出：状态信息
 * @return 成功返回 0，失败返回负数错误码
 */
int32_t pdl_mcu_decode_get_status(const uint8_t *packet, size_t packet_len,
                                   pdl_mcu_status_t *status);

/**
 * @brief 编码 MCU 复位请求
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_reset(uint8_t *buffer, size_t buffer_size);

/**
 * @brief 编码 MCU 寄存器读取请求
 * @param reg_addr 寄存器地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_read_register(uint8_t reg_addr, uint8_t *buffer, size_t buffer_size);

/**
 * @brief 解码 MCU 寄存器读取响应
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param value 输出：寄存器值
 * @return 成功返回 0，失败返回负数错误码
 */
int32_t pdl_mcu_decode_read_register(const uint8_t *packet, size_t packet_len,
                                      uint8_t *value);

/**
 * @brief 编码 MCU 寄存器写入请求
 * @param reg_addr 寄存器地址
 * @param value 写入值
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_write_register(uint8_t reg_addr, uint8_t value,
                                       uint8_t *buffer, size_t buffer_size);

/**
 * @brief 编码 MCU 自定义命令请求
 * @param cmd_code 命令码
 * @param data 命令数据
 * @param data_len 数据长度
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 成功返回编码后的长度，失败返回负数错误码
 */
int32_t pdl_mcu_encode_custom_command(uint8_t cmd_code, const uint8_t *data,
                                       uint32_t data_len, uint8_t *buffer,
                                       size_t buffer_size);

/**
 * @brief 解码 MCU 通用响应
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param response 输出：响应数据
 * @param resp_size 响应缓冲区大小
 * @param actual_size 输出：实际响应长度
 * @return 成功返回 0，失败返回负数错误码
 */
int32_t pdl_mcu_decode_response(const uint8_t *packet, size_t packet_len,
                                 uint8_t *response, uint32_t resp_size,
                                 uint32_t *actual_size);

#ifdef __cplusplus
}
#endif

#endif /* PDL_MCU_PROTOCOL_H */
