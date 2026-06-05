/************************************************************************
 * CCM 系统驱动对外API
 *
 * 功能：
 * - 封装以太网通信协议（PMC-CCM 协议）
 * - 处理与 CCM 的命令请求和响应
 * - 管理心跳和状态上报
 * - 提供统一的 CCM 交互接口
 *
 * 使用场景：
 * - PMC 产品中使用，用于与 CCM 通信
 ************************************************************************/

#ifndef PDL_CCM_H
#define PDL_CCM_H

#include "osal.h"

/* CCM 状态码 */
typedef enum
{
	PDL_CCM_STATUS_OK = 0x00,
	PDL_CCM_STATUS_ERROR = 0x01,
	PDL_CCM_STATUS_BUSY = 0x02,
	PDL_CCM_STATUS_TIMEOUT = 0x03,
	PDL_CCM_STATUS_DISCONNECTED = 0x04
} pdl_ccm_status_t;

/* CCM 电源操作 */
typedef enum
{
	PDL_CCM_POWER_OFF = 0x00,
	PDL_CCM_POWER_ON = 0x01,
	PDL_CCM_POWER_QUERY = 0x02
} pdl_ccm_power_op_t;

/* CCM 节点操作 */
typedef enum
{
	PDL_CCM_NODE_QUERY = 0x00,
	PDL_CCM_NODE_START = 0x01,
	PDL_CCM_NODE_STOP = 0x02,
	PDL_CCM_NODE_RESTART = 0x03
} pdl_ccm_node_op_t;

/*
 * CCM 系统驱动句柄
 */
typedef void* pdl_ccm_handle_t;

/*
 * CCM 系统驱动配置
 *
 * 说明：直接嵌入网络配置，避免重复定义
 */
typedef struct
{
	/* 网络配置 */
	const char *ccm_ip;              /* CCM IP 地址 */
	uint16_t ccm_port;               /* CCM 端口 */
	uint32_t connect_timeout_ms;     /* 连接超时(ms) */
	uint32_t send_timeout_ms;        /* 发送超时(ms) */
	uint32_t recv_timeout_ms;        /* 接收超时(ms) */

	/* 业务配置 */
	uint32_t heartbeat_interval_ms;  /* 心跳间隔(ms) */
	uint32_t cmd_timeout_ms;         /* 命令超时(ms) */
	uint8_t max_retries;             /* 最大重试次数 */
} pdl_ccm_config_t;

/*
 * 遥测数据回调函数类型
 */
typedef void (*pdl_ccm_telemetry_callback_t)(uint32_t tm_id,
                                              uint32_t tm_source,
                                              const uint8_t *data,
                                              size_t len,
                                              void *user_data);

/*
 * 遥控指令回调函数类型
 */
typedef void (*pdl_ccm_command_callback_t)(uint32_t tc_id,
                                            uint32_t tc_target,
                                            uint32_t tc_action,
                                            const uint8_t *params,
                                            size_t params_len,
                                            void *user_data);

/**
 * @brief 初始化 CCM 系统驱动
 *
 * @param[in] config 配置参数
 * @param[out] handle 驱动句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_CCM_Init(const pdl_ccm_config_t *config,
                     pdl_ccm_handle_t *handle);

/**
 * @brief 反初始化 CCM 系统驱动
 *
 * @param[in] handle 驱动句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_Deinit(pdl_ccm_handle_t handle);

/**
 * @brief 注册遥测数据回调函数
 *
 * @param[in] handle 驱动句柄
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_RegisterTelemetryCallback(pdl_ccm_handle_t handle,
                                          pdl_ccm_telemetry_callback_t callback,
                                          void *user_data);

/**
 * @brief 注册遥控指令回调函数
 *
 * @param[in] handle 驱动句柄
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_RegisterCommandCallback(pdl_ccm_handle_t handle,
                                         pdl_ccm_command_callback_t callback,
                                         void *user_data);

/**
 * @brief 发送遥测数据到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] tm_id 遥测 ID
 * @param[in] tm_source 遥测来源
 * @param[in] data 遥测数据
 * @param[in] len 数据长度
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_SendTelemetry(pdl_ccm_handle_t handle,
                               uint32_t tm_id,
                               uint32_t tm_source,
                               const uint8_t *data,
                               size_t len);

/**
 * @brief 发送遥控指令到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] tc_id 遥控 ID
 * @param[in] tc_target 遥控目标
 * @param[in] tc_action 遥控动作
 * @param[in] params 参数数据
 * @param[in] params_len 参数长度
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_SendCommand(pdl_ccm_handle_t handle,
                             uint32_t tc_id,
                             uint32_t tc_target,
                             uint32_t tc_action,
                             const uint8_t *params,
                             size_t params_len);

/**
 * @brief 发送固件升级数据到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] firmware_id 固件 ID
 * @param[in] target_device 目标设备
 * @param[in] firmware_version 固件版本
 * @param[in] total_size 固件总大小
 * @param[in] offset 当前偏移
 * @param[in] data 固件数据
 * @param[in] len 数据长度
 * @param[in] md5 MD5 校验值
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_SendFirmwareUpdate(pdl_ccm_handle_t handle,
                                    uint32_t firmware_id,
                                    uint32_t target_device,
                                    uint32_t firmware_version,
                                    uint32_t total_size,
                                    uint32_t offset,
                                    const uint8_t *data,
                                    size_t len,
                                    const uint8_t md5[16]);

/**
 * @brief 发送节点管理命令到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] node_id 节点 ID
 * @param[in] operation 操作类型
 * @param[out] node_status 节点状态（查询时返回）
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_NodeManage(pdl_ccm_handle_t handle,
                            uint32_t node_id,
                            pdl_ccm_node_op_t operation,
                            uint32_t *node_status);

/**
 * @brief 发送电源控制命令到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] power_domain 电源域
 * @param[in] operation 操作类型
 * @param[out] power_status 电源状态（查询时返回）
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_PowerControl(pdl_ccm_handle_t handle,
                              uint32_t power_domain,
                              pdl_ccm_power_op_t operation,
                              uint32_t *power_status);

/**
 * @brief 查询 CCM 状态
 *
 * @param[in] handle 驱动句柄
 * @param[in] query_type 查询类型
 * @param[in] query_target 查询目标
 * @param[out] result 查询结果
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_QueryStatus(pdl_ccm_handle_t handle,
                             uint32_t query_type,
                             uint32_t query_target,
                             uint32_t *result);

/**
 * @brief 发送心跳到 CCM
 *
 * @param[in] handle 驱动句柄
 * @param[in] status 当前状态
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_SendHeartbeat(pdl_ccm_handle_t handle,
                               pdl_ccm_status_t status);

/**
 * @brief 获取驱动统计信息
 *
 * @param[in] handle 驱动句柄
 * @param[out] rx_count 接收计数
 * @param[out] tx_count 发送计数
 * @param[out] error_count 错误计数
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_GetStats(pdl_ccm_handle_t handle,
                          uint32_t *rx_count,
                          uint32_t *tx_count,
                          uint32_t *error_count);

/**
 * @brief 获取连接状态
 *
 * @param[in] handle 驱动句柄
 * @param[out] connected 是否连接
 * @param[out] link_quality 链路质量（0-100）
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_CCM_GetConnectionStatus(pdl_ccm_handle_t handle,
                                     bool *connected,
                                     uint32_t *link_quality);

#endif /* PDL_CCM_H */
