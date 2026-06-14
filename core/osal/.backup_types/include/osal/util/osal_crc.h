/**
 * @file osal_crc.h
 * @brief OSAL CRC 校验算法
 *
 * 提供常用的 CRC 校验算法实现：
 * - CRC16-CCITT：用于通信协议校验
 * - CRC32：用于文件完整性校验（预留）
 */

#ifndef OSAL_CRC_H
#define OSAL_CRC_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * CRC16-CCITT 算法
 *===========================================================================*/

/**
 * @brief 计算 CRC16-CCITT 校验值
 *
 * 算法参数：
 * - 多项式：0x1021 (x^16 + x^12 + x^5 + 1)
 * - 初始值：0xFFFF
 * - 输入反转：否
 * - 输出反转：否
 * - 异或输出：0x0000
 *
 * @param[in] data 数据指针
 * @param[in] len  数据长度
 *
 * @return CRC16 值
 *
 * @note 使用查找表优化，速度快
 * @note 线程安全（无状态函数）
 */
uint16_t OSAL_CRC16_CCITT(const uint8_t *data, size_t len);

/**
 * @brief 增量计算 CRC16-CCITT
 *
 * 用于分段计算大数据的 CRC，或跳过某些字段
 *
 * @param[in] crc  当前 CRC 值（初始值为 0xFFFF）
 * @param[in] data 数据指针
 * @param[in] len  数据长度
 *
 * @return 更新后的 CRC16 值
 *
 * @example 分段计算：
 * @code
 * uint16_t crc = 0xFFFF;
 * crc = OSAL_CRC16_CCITT_Update(crc, data1, len1);
 * crc = OSAL_CRC16_CCITT_Update(crc, data2, len2);
 * // crc 为最终结果
 * @endcode
 *
 * @example 跳过字段：
 * @code
 * uint16_t crc = 0xFFFF;
 * crc = OSAL_CRC16_CCITT_Update(crc, data, offset);      // 计算前半部分
 * crc = OSAL_CRC16_CCITT_Update(crc, zeros, 2);          // CRC字段作为0处理
 * crc = OSAL_CRC16_CCITT_Update(crc, data+offset+2, len); // 计算后半部分
 * @endcode
 */
uint16_t OSAL_CRC16_CCITT_Update(uint16_t crc, const uint8_t *data, size_t len);

/*===========================================================================
 * CRC32 算法（预留）
 *===========================================================================*/

/**
 * @brief 计算 CRC32 校验值（预留）
 *
 * 算法参数：
 * - 多项式：0x04C11DB7
 * - 初始值：0xFFFFFFFF
 * - 输入反转：是
 * - 输出反转：是
 * - 异或输出：0xFFFFFFFF
 *
 * @param[in] data 数据指针
 * @param[in] len  数据长度
 *
 * @return CRC32 值
 *
 * @note 暂未实现，返回 0
 */
uint32_t OSAL_CRC32(const uint8_t *data, size_t len);

/**
 * @brief 增量计算 CRC32（预留）
 *
 * @param[in] crc  当前 CRC 值（初始值为 0xFFFFFFFF）
 * @param[in] data 数据指针
 * @param[in] len  数据长度
 *
 * @return 更新后的 CRC32 值
 *
 * @note 暂未实现，返回输入的 crc
 */
uint32_t OSAL_CRC32_Update(uint32_t crc, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CRC_H */
