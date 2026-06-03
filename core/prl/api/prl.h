/**
 * @file prl.h
 * @brief PRL Protocol Layer - Unified API Entry Point
 * @details PRL 协议层统一入口头文件 - 包含所有对外 API
 *
 * 使用建议：
 * - 推荐只包含这一个头文件即可使用 PRL 所有功能
 * - 如果只需要特定设备协议，也可以单独包含对应的头文件
 *
 * 命名规范：
 * - 对外 API：PRL_ 前缀（大写）
 * - 类型定义：prl_xxx_t
 * - 宏定义：PRL_XXX
 * - 枚举值：PRL_XXX
 *
 * @example
 * #include "prl.h"
 *
 * // 编码 MCU 消息
 * uint8_t buffer[PRL_MAX_PACKET_SIZE];
 * int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
 *                      NULL, 0, buffer, sizeof(buffer), 0);
 *
 * // 解码消息
 * uint8_t dev_type, msg_type;
 * const uint8_t *payload;
 * uint16_t payload_len;
 * int ret = PRL_Decode(packet, packet_len,
 *                      &dev_type, &msg_type,
 *                      &payload, &payload_len);
 */

#ifndef PRL_H
#define PRL_H

/* 核心 API 和类型定义 */
#include "prl_types.h"
#include "prl_api.h"

/* 设备协议定义（按需包含） */
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"
#include "prl_power.h"

#endif /* PRL_H */
