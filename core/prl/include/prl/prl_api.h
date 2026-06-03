/**
 * @file prl_api.h
 * @brief Protocol Layer Public API
 * @details PRL 协议层对外 API，提供统一的协议编解码接口
 *
 * 命名规范：
 * - 对外 API：PRL_ 前缀（大写）
 * - 内部函数：prl_ 前缀（小写）
 * - 类型定义：prl_xxx_t
 * - 宏定义：PRL_XXX
 */

#ifndef PRL_API_H
#define PRL_API_H

#include "prl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 PRL 协议层
 * @return PRL_OK 成功，其他值失败
 * @note 可选调用，主要用于初始化全局状态（如序列号）
 */
int PRL_Init(void);

/**
 * @brief 反初始化 PRL 协议层
 * @return PRL_OK 成功，其他值失败
 */
int PRL_Deinit(void);

/**
 * @brief 编码设备消息
 *
 * @param[in] dev_type 设备类型（PRL_DEV_TYPE_xxx）
 * @param[in] msg_type 消息类型（设备特定）
 * @param[in] payload 负载数据指针（可为 NULL）
 * @param[in] payload_len 负载数据长度
 * @param[out] buffer 输出缓冲区
 * @param[in] buffer_size 缓冲区大小
 * @param[in] flags 标志位（PRL_FLAG_xxx）
 *
 * @return 成功返回编码后的总长度（>0），失败返回负数错误码
 *
 * @note
 * - buffer 必须足够大，建议使用 PRL_MAX_PACKET_SIZE
 * - 函数会自动填充协议头、计算 CRC、管理序列号和时间戳
 * - 编码后的数据可以直接通过 HAL 层发送
 *
 * @example
 * uint8_t buffer[PRL_MAX_PACKET_SIZE];
 * prl_mcu_version_t version = {1, 2, 3};
 * int len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
 *                      &version, sizeof(version),
 *                      buffer, sizeof(buffer), 0);
 * if (len > 0) {
 *     HAL_CAN_Send(handle, buffer, len);
 * }
 */
int PRL_Encode(uint8_t dev_type, uint8_t msg_type,
               const void *payload, uint16_t payload_len,
               uint8_t *buffer, size_t buffer_size, uint8_t flags);

/**
 * @brief 解码设备消息
 *
 * @param[in] packet 报文数据
 * @param[in] packet_len 报文长度
 * @param[out] dev_type 输出：设备类型
 * @param[out] msg_type 输出：消息类型
 * @param[out] payload 输出：负载数据指针（指向 packet 内部，零拷贝）
 * @param[out] payload_len 输出：负载长度
 *
 * @return PRL_OK 成功，其他值失败
 *
 * @note
 * - payload 指向 packet 内部，无需额外分配内存（零拷贝）
 * - payload 的生命周期与 packet 相同
 * - 函数会自动验证魔数、版本、CRC 等
 *
 * @example
 * uint8_t dev_type, msg_type;
 * const uint8_t *payload;
 * uint16_t payload_len;
 *
 * int ret = PRL_Decode(packet, packet_len,
 *                      &dev_type, &msg_type,
 *                      &payload, &payload_len);
 * if (ret == PRL_OK) {
 *     if (dev_type == PRL_DEV_TYPE_MCU) {
 *         handle_mcu_message(msg_type, payload, payload_len);
 *     }
 * }
 */
int PRL_Decode(const uint8_t *packet, size_t packet_len,
               uint8_t *dev_type, uint8_t *msg_type,
               const uint8_t **payload, uint16_t *payload_len);

/**
 * @brief 验证设备类型是否有效
 *
 * @param[in] dev_type 设备类型
 * @return true 有效，false 无效
 *
 * @note 有效的设备类型范围：PRL_DEV_TYPE_MCU ~ PRL_DEV_TYPE_BMC
 */
bool PRL_IsDeviceTypeValid(uint8_t dev_type);

/**
 * @brief 获取设备类型名称
 *
 * @param[in] dev_type 设备类型
 * @return 设备类型名称字符串（静态字符串，无需释放）
 *
 * @example
 * const char *name = PRL_GetDeviceTypeName(PRL_DEV_TYPE_MCU);
 * printf("Device: %s\n", name);  // 输出：Device: MCU
 */
const char *PRL_GetDeviceTypeName(uint8_t dev_type);

/**
 * @brief 获取错误码描述
 *
 * @param[in] error_code 错误码（负数）
 * @return 错误描述字符串（静态字符串，无需释放）
 *
 * @example
 * int ret = PRL_Encode(...);
 * if (ret < 0) {
 *     LOG_ERROR("PRL", "Encode failed: %s", PRL_GetErrorString(ret));
 * }
 */
const char *PRL_GetErrorString(int error_code);

/**
 * @brief 获取协议版本
 *
 * @param[out] major 主版本号
 * @param[out] minor 次版本号
 */
void PRL_GetVersion(uint8_t *major, uint8_t *minor);

/**
 * @brief 重置序列号
 *
 * @param[in] seq 新的序列号起始值
 * @note 通常不需要调用，仅用于测试或特殊场景
 */
void PRL_ResetSequence(uint32_t seq);

/**
 * @brief 获取当前序列号
 *
 * @return 当前序列号
 * @note 返回的是下一个将要使用的序列号
 */
uint32_t PRL_GetCurrentSequence(void);

/**
 * @brief 验证报文完整性（魔数、版本、CRC）
 *
 * @param[in] packet 报文数据
 * @param[in] packet_len 报文长度
 * @return PRL_OK 验证通过，其他值验证失败
 *
 * @note 这是一个快速验证接口，不解析负载数据
 */
int PRL_ValidatePacket(const uint8_t *packet, size_t packet_len);

/**
 * @brief 获取报文中的设备类型（快速接口）
 *
 * @param[in] packet 报文数据
 * @param[in] packet_len 报文长度
 * @param[out] dev_type 输出：设备类型
 * @return PRL_OK 成功，其他值失败
 *
 * @note 仅读取协议头，不验证 CRC，用于快速路由
 */
int PRL_GetDeviceType(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type);

/**
 * @brief 获取报文中的消息类型（快速接口）
 *
 * @param[in] packet 报文数据
 * @param[in] packet_len 报文长度
 * @param[out] msg_type 输出：消息类型
 * @return PRL_OK 成功，其他值失败
 *
 * @note 仅读取协议头，不验证 CRC，用于快速路由
 */
int PRL_GetMessageType(const uint8_t *packet, size_t packet_len,
                       uint8_t *msg_type);

/**
 * @brief 获取报文中的序列号（快速接口）
 *
 * @param[in] packet 报文数据
 * @param[in] packet_len 报文长度
 * @param[out] seq 输出：序列号
 * @return PRL_OK 成功，其他值失败
 *
 * @note 仅读取协议头，不验证 CRC，用于去重
 */
int PRL_GetSequence(const uint8_t *packet, size_t packet_len,
                    uint32_t *seq);

/**
 * @brief 构建应答报文
 *
 * @param[in] request_packet 请求报文
 * @param[in] request_len 请求报文长度
 * @param[in] response_payload 应答负载数据
 * @param[in] response_payload_len 应答负载长度
 * @param[out] response_buffer 输出缓冲区
 * @param[in] response_buffer_size 缓冲区大小
 *
 * @return 成功返回应答报文长度（>0），失败返回负数错误码
 *
 * @note
 * - 自动从请求报文中提取设备类型和消息类型
 * - 自动设置 PRL_FLAG_IS_ACK 标志
 * - 序列号与请求报文相同
 */
int PRL_BuildResponse(const uint8_t *request_packet, size_t request_len,
                      const void *response_payload, uint16_t response_payload_len,
                      uint8_t *response_buffer, size_t response_buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* PRL_API_H */
