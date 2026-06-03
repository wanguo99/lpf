/**
 * @file prl_common.h
 * @brief Protocol Layer Internal Common Functions
 * @details PRL 内部通用函数，仅供 PRL 模块内部使用
 */

#ifndef PRL_COMMON_H
#define PRL_COMMON_H

/* 包含公共类型定义 */
#include "prl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 内部工具函数（仅供 PRL 内部使用） ========== */

/**
 * @brief 计算 CRC16（内部函数）
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 值
 * @note 内部函数，外部请使用 PRL_xxx API
 */
uint16_t prl_calc_crc16(const uint8_t *data, size_t len);

/**
 * @brief 获取下一个序列号（内部函数）
 * @return 序列号
 * @note 内部函数，线程安全，使用原子操作
 */
uint32_t prl_get_next_seq(void);

/**
 * @brief 获取当前时间戳（内部函数）
 * @return 时间戳（秒）
 * @note 内部函数，从 OSAL 获取系统时间
 */
uint32_t prl_get_timestamp(void);

/**
 * @brief 初始化协议头（内部函数）
 * @param hdr 协议头指针
 * @param dev_type 设备类型
 * @param msg_type 消息类型
 * @param payload_len 负载长度
 * @param flags 标志位
 * @note 内部函数，自动填充魔数、版本、序列号、时间戳
 */
void prl_init_header(prl_header_t *hdr, uint8_t dev_type, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags);

/**
 * @brief 验证协议头（内部函数）
 * @param hdr 协议头指针
 * @param expected_type 期望的消息类型（0表示不检查）
 * @return PRL_OK 成功，其他值表示错误
 * @note 内部函数，验证魔数、版本、长度
 */
int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type);

/**
 * @brief 计算并设置报文 CRC（内部函数）
 * @param packet 报文指针
 * @param total_len 报文总长度
 * @note 内部函数，CRC 字段先置 0，然后计算整个报文的 CRC
 */
void prl_set_packet_crc(uint8_t *packet, size_t total_len);

/**
 * @brief 验证报文 CRC（内部函数）
 * @param packet 报文指针
 * @param total_len 报文总长度
 * @return true 校验通过，false 校验失败
 * @note 内部函数，分段计算 CRC，避免动态内存分配
 */
bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len);

#ifdef __cplusplus
}
#endif

#endif /* PRL_COMMON_H */
