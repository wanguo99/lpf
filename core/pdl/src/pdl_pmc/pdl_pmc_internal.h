/************************************************************************
 * PMC 系统驱动内部头文件
 *
 * 说明：
 * - 定义内部数据结构和函数
 * - 不对外暴露
 ************************************************************************/

#ifndef PDL_PMC_INTERNAL_H
#define PDL_PMC_INTERNAL_H

#include "osal.h"
#include "pconfig.h"
#include "pdl_pmc.h"

/* 消息最大长度 */
#define PMC_MAX_MSG_SIZE    0x1000

/* 重试间隔 */
#define PMC_RETRY_INTERVAL_MS   0x64

/*
 * PMC 消息结构（Ethernet 和 CAN 通用）
 */
typedef struct
{
    uint8_t msg_type;       /* 消息类型 */
    uint32_t seq_num;       /* 序列号 */
    uint32_t payload_len;   /* 负载长度 */
    uint8_t payload[PMC_MAX_MSG_SIZE];  /* 负载数据 */
} pmc_msg_t;

/* 为了保持向后兼容 */
typedef pmc_msg_t pmc_eth_msg_t;

/*
 * PMC 通信接口操作函数表（类似 Linux 驱动的 ops 结构）
 *
 * 设计理念：
 * - 在初始化时根据接口类型注册 ops（只执行一次 switch 判断）
 * - 运行时直接通过函数指针调用，无需重复判断接口类型
 * - 符合 Linux 内核驱动设计模式
 */
typedef struct pdl_pmc_ops {
	/**
	 * @brief 初始化通信接口
	 * @param[in] config 接口配置
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
	 * @brief 发送消息
	 * @param[in] handle 通信句柄
	 * @param[in] msg 消息结构
	 * @param[in] timeout_ms 超时时间（毫秒）
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*send)(void *handle, const pmc_msg_t *msg, uint32_t timeout_ms);

	/**
	 * @brief 接收消息
	 * @param[in] handle 通信句柄
	 * @param[out] msg 消息结构
	 * @param[in] timeout_ms 超时时间（毫秒）
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*recv)(void *handle, pmc_msg_t *msg, uint32_t timeout_ms);

	/**
	 * @brief 检查连接状态
	 * @param[in] handle 通信句柄
	 * @return 1 已连接，0 未连接
	 */
	int32_t (*is_connected)(void *handle);
} pdl_pmc_ops_t;

/*
 * Ethernet 通信接口（pdl_pmc_eth.c 实现）
 */
int32_t pmc_eth_init(const pdl_pmc_config_t *config, void **handle);
int32_t pmc_eth_deinit(void *handle);
int32_t pmc_eth_send(void *handle, const pmc_eth_msg_t *msg, uint32_t timeout_ms);
int32_t pmc_eth_recv(void *handle, pmc_eth_msg_t *msg, uint32_t timeout_ms);
int32_t pmc_eth_is_connected(void *handle);

/* Ethernet 接口的 ops 结构（在 pdl_pmc_eth.c 中定义） */
extern const pdl_pmc_ops_t pmc_eth_ops;

/*
 * CAN 通信接口（pdl_pmc_can.c 实现）
 */
/* CAN 接口的 ops 结构（在 pdl_pmc_can.c 中定义） */
extern const pdl_pmc_ops_t pmc_can_ops;

#endif /* PDL_PMC_INTERNAL_H */
