/**
 * @file osal_crc.h
 * @brief OSAL CRC 校验算法
 */

#ifndef OSAL_CRC_H
#define OSAL_CRC_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 计算 CRC16-CCITT 校验值
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 校验值
 */
uint16_t OSAL_crc16_ccitt(const uint8_t *data, size_t len);

/**
 * @brief 增量计算 CRC16-CCITT 校验值
 * @param crc 当前 CRC 值
 * @param data 数据指针
 * @param len 数据长度
 * @return 更新后的 CRC16 校验值
 */
uint16_t OSAL_crc16_ccitt_update(uint16_t crc, const uint8_t *data, size_t len);

/**
 * @brief 计算 CRC16-MODBUS 校验值
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 校验值
 * @note 多项式：0xA001，初始值：0xFFFF
 */
uint16_t OSAL_crc16_modbus(const uint8_t *data, size_t len);

/**
 * @brief 计算 CRC32 校验值
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC32 校验值
 */
uint32_t OSAL_crc32(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CRC_H */
