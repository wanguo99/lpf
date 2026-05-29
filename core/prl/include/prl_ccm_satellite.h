/**
 * @file prl_ccm_satellite.h
 * @brief CCM-Satellite Protocol Definition
 * @details CCM（通信管理板）与卫星平台之间的通信协议
 *          传输方式：CAN 总线 + OC 指令
 */

#ifndef PRL_CCM_SATELLITE_H
#define PRL_CCM_SATELLITE_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CCM-Satellite 协议版本 */
#define PRL_CCM_SATELLITE_VERSION   0x0100

/* CCM-Satellite 消息类型 */
typedef enum {
    PRL_CCM_SAT_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_CCM_SAT_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_CCM_SAT_MSG_TELECOMMAND     = 0x03,     /* 遥控指令 */
    PRL_CCM_SAT_MSG_TIME_SYNC       = 0x04,     /* 时间同步 */
    PRL_CCM_SAT_MSG_ORBIT_DATA      = 0x05,     /* 轨道数据 */
    PRL_CCM_SAT_MSG_ATTITUDE_DATA   = 0x06,     /* 姿态数据 */
    PRL_CCM_SAT_MSG_POWER_STATUS    = 0x07,     /* 电源状态 */
    PRL_CCM_SAT_MSG_THERMAL_STATUS  = 0x08,     /* 热控状态 */
    PRL_CCM_SAT_MSG_ACK             = 0xFF,     /* 应答 */
} prl_ccm_satellite_msg_type_t;

/* OC 指令类型（卫星平台定义） */
typedef enum {
    PRL_CCM_SAT_OC_POWER_ON         = 0x01,     /* 上电 */
    PRL_CCM_SAT_OC_POWER_OFF        = 0x02,     /* 下电 */
    PRL_CCM_SAT_OC_RESET            = 0x03,     /* 复位 */
    PRL_CCM_SAT_OC_MODE_SWITCH      = 0x04,     /* 模式切换 */
    PRL_CCM_SAT_OC_DATA_DOWNLOAD    = 0x05,     /* 数据下传 */
    PRL_CCM_SAT_OC_PARAM_UPDATE     = 0x06,     /* 参数更新 */
} prl_ccm_satellite_oc_type_t;

/* 心跳消息 */
typedef struct {
    uint32_t ccm_status;        /* CCM 状态 */
    uint32_t satellite_status;  /* 卫星平台状态 */
    uint32_t link_status;       /* 链路状态 */
    uint32_t error_count;       /* 错误计数 */
} __attribute__((packed)) prl_ccm_satellite_heartbeat_t;

/* 遥测数据 */
typedef struct {
    uint32_t tm_id;             /* 遥测 ID */
    uint32_t tm_category;       /* 遥测类别 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t data_format;       /* 数据格式 */
    uint32_t data_length;       /* 数据长度 */
    uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_ccm_satellite_telemetry_t;

/* 遥控指令 */
typedef struct {
    uint32_t tc_id;             /* 遥控 ID */
    uint32_t oc_type;           /* OC 指令类型 */
    uint32_t target_subsystem;  /* 目标子系统 */
    uint32_t execution_time;    /* 执行时间（0表示立即执行） */
    uint32_t param_length;      /* 参数长度 */
    uint8_t  params[0];         /* 参数数据（变长） */
} __attribute__((packed)) prl_ccm_satellite_telecommand_t;

/* 时间同步 */
typedef struct {
    uint64_t utc_time_us;       /* UTC 时间（微秒） */
    uint32_t gps_week;          /* GPS 周 */
    uint32_t gps_second;        /* GPS 秒 */
    int32_t  time_offset_us;    /* 时间偏移（微秒） */
} __attribute__((packed)) prl_ccm_satellite_time_sync_t;

/* 轨道数据 */
typedef struct {
    uint64_t epoch_time;        /* 历元时间 */
    double   semi_major_axis;   /* 半长轴（km） */
    double   eccentricity;      /* 偏心率 */
    double   inclination;       /* 轨道倾角（度） */
    double   raan;              /* 升交点赤经（度） */
    double   arg_perigee;       /* 近地点幅角（度） */
    double   mean_anomaly;      /* 平近点角（度） */
} __attribute__((packed)) prl_ccm_satellite_orbit_data_t;

/* 姿态数据 */
typedef struct {
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    float    quaternion[4];     /* 四元数 */
    float    angular_velocity[3]; /* 角速度（rad/s） */
    float    euler_angles[3];   /* 欧拉角（度） */
    uint32_t attitude_mode;     /* 姿态模式 */
} __attribute__((packed)) prl_ccm_satellite_attitude_data_t;

/* 电源状态 */
typedef struct {
    uint32_t battery_voltage_mv;   /* 电池电压（毫伏） */
    int32_t  battery_current_ma;   /* 电池电流（毫安） */
    uint32_t battery_capacity;     /* 电池容量（%） */
    uint32_t solar_voltage_mv;     /* 太阳能板电压（毫伏） */
    uint32_t solar_current_ma;     /* 太阳能板电流（毫安） */
    uint32_t power_mode;           /* 电源模式 */
} __attribute__((packed)) prl_ccm_satellite_power_status_t;

/* 热控状态 */
typedef struct {
    int32_t  temp_sensor_count;    /* 温度传感器数量 */
    int32_t  temperatures[16];     /* 温度值（摄氏度 * 100） */
    uint32_t heater_status;        /* 加热器状态（位图） */
    uint32_t thermal_mode;         /* 热控模式 */
} __attribute__((packed)) prl_ccm_satellite_thermal_status_t;

/* 应答消息 */
typedef struct {
    uint32_t ack_seq;           /* 应答的序列号 */
    uint8_t  ack_type;          /* 应答的消息类型 */
    uint8_t  result;            /* 结果：0成功，非0失败 */
    uint16_t error_code;        /* 错误码 */
} __attribute__((packed)) prl_ccm_satellite_ack_t;

/* ========== 编码函数 ========== */

int prl_ccm_satellite_encode_heartbeat(const prl_ccm_satellite_heartbeat_t *msg,
                                        uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_telemetry(const prl_ccm_satellite_telemetry_t *msg,
                                        const uint8_t *data, size_t data_len,
                                        uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_telecommand(const prl_ccm_satellite_telecommand_t *msg,
                                          const uint8_t *params, size_t params_len,
                                          uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_time_sync(const prl_ccm_satellite_time_sync_t *msg,
                                        uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_orbit_data(const prl_ccm_satellite_orbit_data_t *msg,
                                         uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_attitude_data(const prl_ccm_satellite_attitude_data_t *msg,
                                            uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_power_status(const prl_ccm_satellite_power_status_t *msg,
                                           uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_thermal_status(const prl_ccm_satellite_thermal_status_t *msg,
                                             uint8_t *buf, size_t *len);

int prl_ccm_satellite_encode_ack(const prl_ccm_satellite_ack_t *msg,
                                  uint8_t *buf, size_t *len);

/* ========== 解码函数 ========== */

int prl_ccm_satellite_decode_heartbeat(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_heartbeat_t *msg);

int prl_ccm_satellite_decode_telemetry(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_telemetry_t *msg,
                                        uint8_t **data, size_t *data_len);

int prl_ccm_satellite_decode_telecommand(const uint8_t *buf, size_t len,
                                          prl_ccm_satellite_telecommand_t *msg,
                                          uint8_t **params, size_t *params_len);

int prl_ccm_satellite_decode_time_sync(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_time_sync_t *msg);

int prl_ccm_satellite_decode_orbit_data(const uint8_t *buf, size_t len,
                                         prl_ccm_satellite_orbit_data_t *msg);

int prl_ccm_satellite_decode_attitude_data(const uint8_t *buf, size_t len,
                                            prl_ccm_satellite_attitude_data_t *msg);

int prl_ccm_satellite_decode_power_status(const uint8_t *buf, size_t len,
                                           prl_ccm_satellite_power_status_t *msg);

int prl_ccm_satellite_decode_thermal_status(const uint8_t *buf, size_t len,
                                             prl_ccm_satellite_thermal_status_t *msg);

int prl_ccm_satellite_decode_ack(const uint8_t *buf, size_t len,
                                  prl_ccm_satellite_ack_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* PRL_CCM_SATELLITE_H */
