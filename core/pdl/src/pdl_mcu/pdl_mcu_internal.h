/************************************************************************
 * MCU驱动内部接口
 *
 * 仅供pdl_mcu模块内部使用，不对外暴露
 ************************************************************************/

#ifndef PDL_MCU_INTERNAL_H
#define PDL_MCU_INTERNAL_H

#include "osal.h"
#include "pdl_mcu.h"

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
 * MCU通信接口操作函数表（类似Linux驱动的ops结构）
 *
 * 设计理念：
 * - 在初始化时根据接口类型注册ops（只执行一次switch判断）
 * - 运行时直接通过函数指针调用，无需重复判断接口类型
 * - 符合Linux内核驱动设计模式（如file_operations、platform_driver）
 */
typedef struct pdl_mcu_ops {
	/**
	 * @brief 初始化通信接口
	 * @param[in] config 接口配置（pconfig_can_config_t* 或 pconfig_serial_config_t*）
	 * @param[out] handle 返回通信句柄
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*init)(const void *config, void **handle);

	/**
	 * @brief 反初始化通信接口
	 * @param[in] handle 通信句柄
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*deinit)(void *handle);

	/**
	 * @brief 发送命令并接收响应
	 * @param[in] handle 通信句柄
	 * @param[in] cmd_code 命令码
	 * @param[in] data 命令数据
	 * @param[in] data_len 数据长度
	 * @param[out] response 响应缓冲区
	 * @param[in] resp_size 响应缓冲区大小
	 * @param[out] actual_size 实际响应长度
	 * @param[in] timeout_ms 超时时间（毫秒）
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*send_command)(void *handle,
	                        uint8_t cmd_code,
	                        const uint8_t *data,
	                        uint32_t data_len,
	                        uint8_t *response,
	                        uint32_t resp_size,
	                        uint32_t *actual_size,
	                        uint32_t timeout_ms);
} pdl_mcu_ops_t;

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

/* CAN接口的ops结构（在pdl_mcu_can.c中定义） */
extern const pdl_mcu_ops_t mcu_can_ops;

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

/* Serial接口的ops结构（在pdl_mcu_serial.c中定义） */
extern const pdl_mcu_ops_t mcu_serial_ops;

#endif /* PDL_MCU_INTERNAL_H */
