/************************************************************************
 * MCU外设驱动内部头文件
 *
 * 职责：
 * - 定义内部数据结构
 * - 声明内部接口（CAN/串口通信层）
 ************************************************************************/

#ifndef PDM_MCU_INTERNAL_H
#define PDM_MCU_INTERNAL_H

#include "osal.h"
#include "pconfig.h"
#include "prl.h"

/*===========================================================================
 * 通信层操作函数表
 *===========================================================================*/

/**
 * @brief MCU通信层操作函数表
 */
typedef struct {
	/**
     * @brief 初始化通信层
     * @param[in] config 配置参数
     * @param[out] handle 返回通信句柄
     * @return OSAL_SUCCESS 成功
     */
	int32_t (*init)(const void *config, void **handle);

	/**
     * @brief 反初始化通信层
     * @param[in] handle 通信句柄
     * @return OSAL_SUCCESS 成功
     */
	int32_t (*deinit)(void *handle);

	/**
     * @brief 发送 PRL 报文并接收响应
     * @param[in] handle 通信句柄
     * @param[in] packet PRL 报文数据
     * @param[in] packet_len 报文长度
     * @param[out] response 响应缓冲区
     * @param[in] resp_size 响应缓冲区大小
     * @param[out] actual_size 实际响应长度
     * @param[in] timeout_ms 超时时间（毫秒）
     * @return OSAL_SUCCESS 成功
     */
	int32_t (*send_packet)(void *handle, const uint8_t *packet,
						   uint32_t packet_len, uint8_t *response,
						   uint32_t resp_size, uint32_t *actual_size,
						   uint32_t timeout_ms);
} pdm_mcu_ops_t;

/*===========================================================================
 * CAN通信层接口
 *===========================================================================*/

int32_t mcu_can_init(const void *config, void **handle);
int32_t mcu_can_deinit(void *handle);
int32_t mcu_can_send_packet(void *handle, const uint8_t *packet,
							uint32_t packet_len, uint8_t *response,
							uint32_t resp_size, uint32_t *actual_size,
							uint32_t timeout_ms);

extern const pdm_mcu_ops_t mcu_can_ops;

/*===========================================================================
 * 串口通信层接口
 *===========================================================================*/

int32_t mcu_serial_init(const void *config, void **handle);
int32_t mcu_serial_deinit(void *handle);
int32_t mcu_serial_send_packet(void *handle, const uint8_t *packet,
							   uint32_t packet_len, uint8_t *response,
							   uint32_t resp_size, uint32_t *actual_size,
							   uint32_t timeout_ms);

extern const pdm_mcu_ops_t mcu_serial_ops;

#endif /* PDM_MCU_INTERNAL_H */
