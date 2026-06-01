/**
 * @file prl_device.h
 * @brief Protocol Layer Device Messages
 * @details 统一设备协议消息定义，所有设备共用相同的报文头和编解码逻辑
 *
 * @note 本文件已重构为按设备类型组织，具体设备消息定义请参考：
 *       - prl_mcu.h    - MCU 设备消息
 *       - prl_ccm.h    - CCM 设备消息
 *       - prl_pmc.h    - PMC 设备消息
 *       - prl_gsc.h    - GSC 设备消息
 *       - prl_power.h  - POWER 设备消息
 */

#ifndef PRL_DEVICE_H
#define PRL_DEVICE_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 包含各设备的消息定义 */
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"
#include "prl_power.h"

/* ========== 内部编解码接口（仅供 PRL 内部使用） ========== */

/**
 * @brief 编码设备消息（内部函数）
 * @param dev_type 设备类型
 * @param msg_type 消息类型
 * @param payload 负载数据
 * @param payload_len 负载长度
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param flags 标志位
 * @return 成功返回编码后的总长度，失败返回负数错误码
 * @note 内部函数，外部请使用 PRL_Encode()
 */
int prl_device_encode(uint8_t dev_type, uint8_t msg_type,
                      const void *payload, uint16_t payload_len,
                      uint8_t *buffer, size_t buffer_size, uint8_t flags);

/**
 * @brief 解码设备消息（内部函数）
 * @param packet 报文数据
 * @param packet_len 报文长度
 * @param dev_type 输出：设备类型
 * @param msg_type 输出：消息类型
 * @param payload 输出：负载数据指针（指向 packet 内部，零拷贝）
 * @param payload_len 输出：负载长度
 * @return 成功返回 PRL_OK，失败返回负数错误码
 * @note 内部函数，外部请使用 PRL_Decode()
 */
int prl_device_decode(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type, uint8_t *msg_type,
                      const uint8_t **payload, uint16_t *payload_len);

/**
 * @brief 验证设备类型是否有效（内部函数）
 * @param dev_type 设备类型
 * @return 有效返回 true，无效返回 false
 * @note 内部函数，外部请使用 PRL_IsDeviceTypeValid()
 */
bool prl_device_type_valid(uint8_t dev_type);

/**
 * @brief 获取设备类型名称（内部函数）
 * @param dev_type 设备类型
 * @return 设备类型名称字符串
 * @note 内部函数，外部请使用 PRL_GetDeviceTypeName()
 */
const char *prl_device_type_name(uint8_t dev_type);

#ifdef __cplusplus
}
#endif

#endif /* PRL_DEVICE_H */
