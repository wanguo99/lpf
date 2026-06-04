/**
 * @file prl_ccm.h
 * @brief CCM Device Protocol Messages
 * @details CCM（通信管理板）设备的消息类型和结构体定义
 */

#ifndef PRL_CCM_H
#define PRL_CCM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CCM 消息类型
 * @note 使用设备类型 PRL_DEV_TYPE_CCM
 */
typedef enum {
	PRL_CCM_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
	PRL_CCM_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
	PRL_CCM_MSG_TELECOMMAND     = 0x03,     /* 遥控指令 */
	PRL_CCM_MSG_TIME_SYNC       = 0x04,     /* 时间同步 */
	PRL_CCM_MSG_ORBIT_DATA      = 0x05,     /* 轨道数据 */
	PRL_CCM_MSG_ATTITUDE_DATA   = 0x06,     /* 姿态数据 */
	PRL_CCM_MSG_POWER_STATUS    = 0x07,     /* 电源状态 */
	PRL_CCM_MSG_THERMAL_STATUS  = 0x08,     /* 热控状态 */
	PRL_CCM_MSG_ACK             = 0xFF,     /* 应答 */
} prl_ccm_msg_type_t;

/**
 * @brief CCM 心跳消息
 */
typedef struct {
	uint32_t ccm_status;        /* CCM 状态 */
	uint32_t link_quality;      /* 链路质量（0-100） */
	uint32_t signal_strength;   /* 信号强度（dBm） */
	uint32_t error_count;       /* 错误计数 */
} __attribute__((packed)) prl_ccm_heartbeat_t;

/**
 * @brief CCM 遥测数据
 */
typedef struct {
	uint32_t tm_id;             /* 遥测 ID */
	uint32_t tm_type;           /* 遥测类型 */
	uint64_t timestamp_us;      /* 时间戳（微秒） */
	uint32_t data_length;       /* 数据长度 */
	uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_ccm_telemetry_t;

/**
 * @brief CCM 遥控指令
 */
typedef struct {
	uint32_t tc_id;             /* 遥控 ID */
	uint32_t tc_type;           /* 遥控类型 */
	uint32_t target_subsystem;  /* 目标子系统 */
	uint32_t param_length;      /* 参数长度 */
	uint8_t  params[0];         /* 遥控参数（变长） */
} __attribute__((packed)) prl_ccm_telecommand_t;

/**
 * @brief CCM 时间同步
 */
typedef struct {
	uint64_t utc_time_us;       /* UTC 时间（微秒） */
	uint32_t time_source;       /* 时间源（GPS/地面站/本地） */
	uint32_t time_quality;      /* 时间质量（精度） */
	int32_t  time_offset_us;    /* 时间偏移（微秒） */
} __attribute__((packed)) prl_ccm_time_sync_t;

/**
 * @brief CCM 轨道数据
 */
typedef struct {
	uint64_t epoch_time_us;     /* 历元时间（微秒） */
	double   position_x;        /* X 位置（米） */
	double   position_y;        /* Y 位置（米） */
	double   position_z;        /* Z 位置（米） */
	double   velocity_x;        /* X 速度（米/秒） */
	double   velocity_y;        /* Y 速度（米/秒） */
	double   velocity_z;        /* Z 速度（米/秒） */
} __attribute__((packed)) prl_ccm_orbit_data_t;

/**
 * @brief CCM 姿态数据
 */
typedef struct {
	uint64_t timestamp_us;      /* 时间戳（微秒） */
	float    quaternion_w;      /* 四元数 W */
	float    quaternion_x;      /* 四元数 X */
	float    quaternion_y;      /* 四元数 Y */
	float    quaternion_z;      /* 四元数 Z */
	float    angular_rate_x;    /* 角速度 X（度/秒） */
	float    angular_rate_y;    /* 角速度 Y（度/秒） */
	float    angular_rate_z;    /* 角速度 Z（度/秒） */
} __attribute__((packed)) prl_ccm_attitude_data_t;

/**
 * @brief CCM 电源状态
 */
typedef struct {
	uint32_t power_domain;      /* 电源域 */
	uint32_t voltage_mv;        /* 电压（毫伏） */
	uint32_t current_ma;        /* 电流（毫安） */
	uint32_t power_mw;          /* 功率（毫瓦） */
	uint8_t  enabled;           /* 使能状态 */
	uint8_t  fault;             /* 故障标志 */
	uint16_t reserved;          /* 保留 */
} __attribute__((packed)) prl_ccm_power_status_t;

/**
 * @brief CCM 热控状态
 */
typedef struct {
	uint32_t sensor_id;         /* 传感器 ID */
	int32_t  temperature;       /* 温度（0.01°C） */
	uint32_t heater_status;     /* 加热器状态 */
	uint32_t fan_speed;         /* 风扇转速（RPM） */
} __attribute__((packed)) prl_ccm_thermal_status_t;

/**
 * @brief CCM 状态信息
 */
typedef struct {
	uint8_t  link_status;       /* 链路状态 */
	uint8_t  signal_strength;   /* 信号强度 */
	uint16_t error_count;       /* 错误计数 */
	uint32_t tx_bytes;          /* 发送字节数 */
	uint32_t rx_bytes;          /* 接收字节数 */
} prl_ccm_status_t;

#ifdef __cplusplus
}
#endif

#endif /* PRL_CCM_H */
