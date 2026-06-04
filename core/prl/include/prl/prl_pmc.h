/**
 * @file prl_pmc.h
 * @brief PMC Device Protocol Messages
 * @details PMC（载荷管理器）设备的消息类型和结构体定义
 */

#ifndef PRL_PMC_H
#define PRL_PMC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PMC 消息类型
 * @note 使用设备类型 PRL_DEV_TYPE_PMC
 */
typedef enum {
	PRL_PMC_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
	PRL_PMC_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
	PRL_PMC_MSG_COMMAND         = 0x03,     /* 遥控指令 */
	PRL_PMC_MSG_FIRMWARE_UPDATE = 0x04,     /* 固件升级 */
	PRL_PMC_MSG_NODE_MANAGE     = 0x05,     /* 节点管理 */
	PRL_PMC_MSG_POWER_CONTROL   = 0x06,     /* 电源控制 */
	PRL_PMC_MSG_STATUS_QUERY    = 0x07,     /* 状态查询 */
	PRL_PMC_MSG_ACK             = 0xFF,     /* 应答 */
} prl_pmc_msg_type_t;

/**
 * @brief PMC 心跳消息
 */
typedef struct {
	uint32_t sender_status;     /* 发送方状态 */
	uint32_t link_quality;      /* 链路质量（0-100） */
	uint32_t packet_loss;       /* 丢包率（百分比 * 100） */
	uint32_t rtt_ms;            /* 往返时延（毫秒） */
} __attribute__((packed)) prl_pmc_heartbeat_t;

/**
 * @brief PMC 遥测数据
 */
typedef struct {
	uint32_t tm_id;             /* 遥测 ID */
	uint32_t tm_source;         /* 遥测来源（设备ID） */
	uint64_t timestamp_us;      /* 时间戳（微秒） */
	uint32_t data_type;         /* 数据类型 */
	uint32_t data_length;       /* 数据长度 */
	uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_pmc_telemetry_t;

/**
 * @brief PMC 遥控指令
 */
typedef struct {
	uint32_t cmd_id;            /* 命令 ID */
	uint32_t cmd_type;          /* 命令类型 */
	uint32_t target_node;       /* 目标节点 */
	uint32_t param_length;      /* 参数长度 */
	uint8_t  params[0];         /* 命令参数（变长） */
} __attribute__((packed)) prl_pmc_command_t;

/**
 * @brief PMC 固件升级
 */
typedef struct {
	uint32_t firmware_id;       /* 固件 ID */
	uint32_t firmware_version;  /* 固件版本 */
	uint32_t total_size;        /* 固件总大小 */
	uint32_t chunk_index;       /* 分片索引 */
	uint32_t chunk_size;        /* 分片大小 */
	uint32_t chunk_crc;         /* 分片 CRC */
	uint8_t  chunk_data[0];     /* 分片数据（变长） */
} __attribute__((packed)) prl_pmc_firmware_update_t;

/**
 * @brief PMC 节点管理操作类型
 */
typedef enum {
	PRL_PMC_NODE_OP_ADD         = 0x01,     /* 添加节点 */
	PRL_PMC_NODE_OP_REMOVE      = 0x02,     /* 移除节点 */
	PRL_PMC_NODE_OP_ENABLE      = 0x03,     /* 使能节点 */
	PRL_PMC_NODE_OP_DISABLE     = 0x04,     /* 禁用节点 */
	PRL_PMC_NODE_OP_QUERY       = 0x05,     /* 查询节点 */
} prl_pmc_node_operation_t;

/**
 * @brief PMC 节点管理
 */
typedef struct {
	uint32_t operation;         /* 操作类型 */
	uint32_t node_id;           /* 节点 ID */
	uint32_t node_type;         /* 节点类型 */
	uint32_t node_status;       /* 节点状态 */
} __attribute__((packed)) prl_pmc_node_manage_t;

/**
 * @brief PMC 电源控制操作类型
 */
typedef enum {
	PRL_PMC_POWER_OP_ON         = 0x01,     /* 上电 */
	PRL_PMC_POWER_OP_OFF        = 0x02,     /* 下电 */
	PRL_PMC_POWER_OP_RESET      = 0x03,     /* 复位 */
	PRL_PMC_POWER_OP_CYCLE      = 0x04,     /* 电源循环 */
} prl_pmc_power_operation_t;

/**
 * @brief PMC 电源控制
 */
typedef struct {
	uint32_t operation;         /* 操作类型 */
	uint32_t target_device;     /* 目标设备 */
	uint32_t power_domain;      /* 电源域 */
	uint32_t delay_ms;          /* 延迟时间（毫秒） */
} __attribute__((packed)) prl_pmc_power_control_t;

/**
 * @brief PMC 状态查询
 */
typedef struct {
	uint32_t query_type;        /* 查询类型 */
	uint32_t target_device;     /* 目标设备 */
	uint32_t param_count;       /* 参数数量 */
	uint8_t  params[0];         /* 查询参数（变长） */
} __attribute__((packed)) prl_pmc_status_query_t;

/**
 * @brief PMC 应答消息
 */
typedef struct {
	uint32_t ack_code;          /* 应答码 */
	uint32_t ack_seq;           /* 应答序列号 */
	uint32_t ack_result;        /* 应答结果（0=成功） */
	uint32_t error_code;        /* 错误码 */
} __attribute__((packed)) prl_pmc_ack_t;

#ifdef __cplusplus
}
#endif

#endif /* PRL_PMC_H */
