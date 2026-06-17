/************************************************************************
 * PCONFIG CCM 配置类型定义
 *
 * 功能：
 * - CCM（通信管理板）外设配置（支持多种通信接口）
 ************************************************************************/

#ifndef PCONFIG_CCM_H
#define PCONFIG_CCM_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * CCM 接口类型
 *===========================================================================*/

/**
 * @brief CCM 通信接口类型
 */
typedef enum {
	PCONFIG_CCM_INTERFACE_ETHERNET = 0,  /* 以太网 */
	PCONFIG_CCM_INTERFACE_CAN = 1,       /* CAN 总线 */
} pconfig_ccm_interface_t;

/*===========================================================================
 * CCM 接口配置（union 方式）
 *===========================================================================*/

/**
 * @brief CCM Ethernet 接口配置
 */
typedef struct {
	const char *ip_address;      /* IP 地址 */
	uint16_t port;               /* 端口号 */
	uint32_t connect_timeout_ms; /* 连接超时 */
	uint32_t send_timeout_ms;    /* 发送超时 */
	uint32_t recv_timeout_ms;    /* 接收超时 */
} pconfig_ccm_ethernet_config_t;

/**
 * @brief CCM CAN 接口配置
 */
typedef struct {
	const char *device;          /* CAN 设备名（如 "can0"） */
	uint32_t bitrate;            /* 波特率 */
	uint32_t tx_id;              /* 发送 CAN ID */
	uint32_t rx_id;              /* 接收 CAN ID */
	uint32_t rx_timeout_ms;      /* 接收超时（毫秒） */
	uint32_t tx_timeout_ms;      /* 发送超时（毫秒） */
} pconfig_ccm_can_config_t;

/*===========================================================================
 * CCM 完整配置
 *===========================================================================*/

/**
 * @brief CCM 外设配置
 */
typedef struct {
	pconfig_ccm_interface_t interface;  /* 接口类型 */

	/* 接口配置（union）*/
	union {
		pconfig_ccm_ethernet_config_t ethernet;
		pconfig_ccm_can_config_t can;
	} hw;

	/* 业务配置 */
	uint32_t heartbeat_interval_ms;  /* 心跳间隔 */
	uint32_t send_timeout_ms;        /* 发送超时 */
	uint32_t recv_timeout_ms;        /* 接收超时 */
} pconfig_ccm_config_t;

/**
 * @brief CCM 配置条目（带启用标志）
 */
typedef struct {
	bool enabled;                        /* 是否启用 */
	const char *description;             /* 描述信息 */
	pconfig_ccm_config_t config;         /* 配置数据 */
} pconfig_ccm_entry_t;

#endif /* PCONFIG_CCM_H */
