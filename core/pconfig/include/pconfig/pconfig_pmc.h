/************************************************************************
 * PCONFIG PMC 配置类型定义
 *
 * 功能：
 * - PMC（通信管理板）外设配置（支持多种通信接口）
 ************************************************************************/

#ifndef PCONFIG_PMC_H
#define PCONFIG_PMC_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * PMC 接口类型
 *===========================================================================*/

/**
 * @brief PMC 通信接口类型
 */
typedef enum {
	PCONFIG_PMC_INTERFACE_ETHERNET = 0,  /* 以太网 */
	PCONFIG_PMC_INTERFACE_CAN = 1,       /* CAN 总线 */
} pconfig_pmc_interface_t;

/*===========================================================================
 * PMC 接口配置（union 方式）
 *===========================================================================*/

/**
 * @brief PMC Ethernet 接口配置
 */
typedef struct {
	const char *ip_address;      /* IP 地址 */
	uint16_t port;               /* 端口号 */
	uint32_t connect_timeout_ms; /* 连接超时 */
	uint32_t send_timeout_ms;    /* 发送超时 */
	uint32_t recv_timeout_ms;    /* 接收超时 */
} pconfig_pmc_ethernet_config_t;

/**
 * @brief PMC CAN 接口配置
 */
typedef struct {
	const char *device;          /* CAN 设备名（如 "can0"） */
	uint32_t bitrate;            /* 波特率 */
	uint32_t tx_id;              /* 发送 CAN ID */
	uint32_t rx_id;              /* 接收 CAN ID */
	uint32_t rx_timeout_ms;      /* 接收超时（毫秒） */
	uint32_t tx_timeout_ms;      /* 发送超时（毫秒） */
} pconfig_pmc_can_config_t;

/*===========================================================================
 * PMC 完整配置
 *===========================================================================*/

/**
 * @brief PMC 外设配置
 */
typedef struct {
	pconfig_pmc_interface_t interface;  /* 接口类型 */

	/* 接口配置（union）*/
	union {
		pconfig_pmc_ethernet_config_t ethernet;
		pconfig_pmc_can_config_t can;
	} hw;

	/* 业务配置 */
	uint32_t heartbeat_interval_ms;  /* 心跳间隔 */
	uint32_t send_timeout_ms;        /* 发送超时 */
	uint32_t recv_timeout_ms;        /* 接收超时 */
} pconfig_pmc_config_t;

/**
 * @brief PMC 配置条目（带启用标志）
 */
typedef struct {
	bool enabled;                        /* 是否启用 */
	const char *description;             /* 描述信息 */
	pconfig_pmc_config_t config;         /* 配置数据 */
} pconfig_pmc_entry_t;

#endif /* PCONFIG_PMC_H */
