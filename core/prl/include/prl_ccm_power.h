/**
 * @file prl_ccm_power.h
 * @brief CCM-Power Protocol Definition
 * @details CCM（通信管理板）与电源管理板之间的通信协议
 *          传输方式：CAN 总线
 */

#ifndef PRL_CCM_POWER_H
#define PRL_CCM_POWER_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CCM-Power 协议版本 */
#define PRL_CCM_POWER_VERSION   0x0100

/* CCM-Power 消息类型 */
typedef enum {
    PRL_CCM_PWR_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_CCM_PWR_MSG_POWER_ON        = 0x02,     /* 上电 */
    PRL_CCM_PWR_MSG_POWER_OFF       = 0x03,     /* 下电 */
    PRL_CCM_PWR_MSG_VOLTAGE_QUERY   = 0x04,     /* 电压查询 */
    PRL_CCM_PWR_MSG_CURRENT_QUERY   = 0x05,     /* 电流查询 */
    PRL_CCM_PWR_MSG_TEMP_QUERY      = 0x06,     /* 温度查询 */
    PRL_CCM_PWR_MSG_STATUS_REPORT   = 0x07,     /* 状态上报 */
    PRL_CCM_PWR_MSG_ALARM           = 0x08,     /* 告警 */
    PRL_CCM_PWR_MSG_ACK             = 0xFF,     /* 应答 */
} prl_ccm_power_msg_type_t;

/* 电源域定义 */
typedef enum {
    PRL_CCM_PWR_DOMAIN_MAIN         = 0x01,     /* 主电源 */
    PRL_CCM_PWR_DOMAIN_BACKUP       = 0x02,     /* 备用电源 */
    PRL_CCM_PWR_DOMAIN_3V3          = 0x03,     /* 3.3V 电源 */
    PRL_CCM_PWR_DOMAIN_5V           = 0x04,     /* 5V 电源 */
    PRL_CCM_PWR_DOMAIN_12V          = 0x05,     /* 12V 电源 */
    PRL_CCM_PWR_DOMAIN_PAYLOAD      = 0x06,     /* 载荷电源 */
} prl_ccm_power_domain_t;

/* 告警类型 */
typedef enum {
    PRL_CCM_PWR_ALARM_OVER_VOLTAGE  = 0x01,     /* 过压 */
    PRL_CCM_PWR_ALARM_UNDER_VOLTAGE = 0x02,     /* 欠压 */
    PRL_CCM_PWR_ALARM_OVER_CURRENT  = 0x03,     /* 过流 */
    PRL_CCM_PWR_ALARM_OVER_TEMP     = 0x04,     /* 过温 */
    PRL_CCM_PWR_ALARM_SHORT_CIRCUIT = 0x05,     /* 短路 */
    PRL_CCM_PWR_ALARM_POWER_FAIL    = 0x06,     /* 电源故障 */
} prl_ccm_power_alarm_type_t;

/* 心跳消息 */
typedef struct {
    uint32_t power_board_status;    /* 电源板状态 */
    uint32_t active_domains;        /* 激活的电源域（位图） */
    uint32_t alarm_status;          /* 告警状态（位图） */
    uint32_t uptime;                /* 运行时间（秒） */
} __attribute__((packed)) prl_ccm_power_heartbeat_t;

/* 上电消息 */
typedef struct {
    uint32_t power_domain;          /* 电源域 */
    uint32_t delay_ms;              /* 延迟时间（毫秒） */
    uint32_t soft_start;            /* 软启动使能 */
} __attribute__((packed)) prl_ccm_power_on_t;

/* 下电消息 */
typedef struct {
    uint32_t power_domain;          /* 电源域 */
    uint32_t delay_ms;              /* 延迟时间（毫秒） */
    uint32_t force_off;             /* 强制关闭 */
} __attribute__((packed)) prl_ccm_power_off_t;

/* 电压查询 */
typedef struct {
    uint32_t power_domain;          /* 电源域 */
    uint32_t voltage_mv;            /* 电压（毫伏） */
    uint32_t voltage_min_mv;        /* 最小电压（毫伏） */
    uint32_t voltage_max_mv;        /* 最大电压（毫伏） */
} __attribute__((packed)) prl_ccm_power_voltage_query_t;

/* 电流查询 */
typedef struct {
    uint32_t power_domain;          /* 电源域 */
    uint32_t current_ma;            /* 电流（毫安） */
    uint32_t current_min_ma;        /* 最小电流（毫安） */
    uint32_t current_max_ma;        /* 最大电流（毫安） */
} __attribute__((packed)) prl_ccm_power_current_query_t;

/* 温度查询 */
typedef struct {
    uint32_t sensor_id;             /* 传感器 ID */
    int32_t  temperature;           /* 温度（摄氏度 * 100） */
    int32_t  temp_min;              /* 最小温度（摄氏度 * 100） */
    int32_t  temp_max;              /* 最大温度（摄氏度 * 100） */
} __attribute__((packed)) prl_ccm_power_temp_query_t;

/* 状态上报 */
typedef struct {
    uint32_t power_domain;          /* 电源域 */
    uint32_t power_status;          /* 电源状态：0关闭 1打开 */
    uint32_t voltage_mv;            /* 电压（毫伏） */
    uint32_t current_ma;            /* 电流（毫安） */
    int32_t  temperature;           /* 温度（摄氏度 * 100） */
    uint32_t error_count;           /* 错误计数 */
} __attribute__((packed)) prl_ccm_power_status_report_t;

/* 告警消息 */
typedef struct {
    uint32_t alarm_type;            /* 告警类型 */
    uint32_t alarm_level;           /* 告警级别：0信息 1警告 2严重 3致命 */
    uint32_t power_domain;          /* 电源域 */
    uint32_t alarm_value;           /* 告警值 */
    uint32_t threshold;             /* 阈值 */
    uint64_t timestamp_us;          /* 时间戳（微秒） */
} __attribute__((packed)) prl_ccm_power_alarm_t;

/* 应答消息 */
typedef struct {
    uint32_t ack_seq;               /* 应答的序列号 */
    uint8_t  ack_type;              /* 应答的消息类型 */
    uint8_t  result;                /* 结果：0成功，非0失败 */
    uint16_t error_code;            /* 错误码 */
} __attribute__((packed)) prl_ccm_power_ack_t;

/* ========== 编码函数 ========== */

int prl_ccm_power_encode_heartbeat(const prl_ccm_power_heartbeat_t *msg,
                                    uint8_t *buf, size_t *len);

int prl_ccm_power_encode_power_on(const prl_ccm_power_on_t *msg,
                                   uint8_t *buf, size_t *len);

int prl_ccm_power_encode_power_off(const prl_ccm_power_off_t *msg,
                                    uint8_t *buf, size_t *len);

int prl_ccm_power_encode_voltage_query(const prl_ccm_power_voltage_query_t *msg,
                                        uint8_t *buf, size_t *len);

int prl_ccm_power_encode_current_query(const prl_ccm_power_current_query_t *msg,
                                        uint8_t *buf, size_t *len);

int prl_ccm_power_encode_temp_query(const prl_ccm_power_temp_query_t *msg,
                                     uint8_t *buf, size_t *len);

int prl_ccm_power_encode_status_report(const prl_ccm_power_status_report_t *msg,
                                        uint8_t *buf, size_t *len);

int prl_ccm_power_encode_alarm(const prl_ccm_power_alarm_t *msg,
                                uint8_t *buf, size_t *len);

int prl_ccm_power_encode_ack(const prl_ccm_power_ack_t *msg,
                              uint8_t *buf, size_t *len);

/* ========== 解码函数 ========== */

int prl_ccm_power_decode_heartbeat(const uint8_t *buf, size_t len,
                                    prl_ccm_power_heartbeat_t *msg);

int prl_ccm_power_decode_power_on(const uint8_t *buf, size_t len,
                                   prl_ccm_power_on_t *msg);

int prl_ccm_power_decode_power_off(const uint8_t *buf, size_t len,
                                    prl_ccm_power_off_t *msg);

int prl_ccm_power_decode_voltage_query(const uint8_t *buf, size_t len,
                                        prl_ccm_power_voltage_query_t *msg);

int prl_ccm_power_decode_current_query(const uint8_t *buf, size_t len,
                                        prl_ccm_power_current_query_t *msg);

int prl_ccm_power_decode_temp_query(const uint8_t *buf, size_t len,
                                     prl_ccm_power_temp_query_t *msg);

int prl_ccm_power_decode_status_report(const uint8_t *buf, size_t len,
                                        prl_ccm_power_status_report_t *msg);

int prl_ccm_power_decode_alarm(const uint8_t *buf, size_t len,
                                prl_ccm_power_alarm_t *msg);

int prl_ccm_power_decode_ack(const uint8_t *buf, size_t len,
                              prl_ccm_power_ack_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* PRL_CCM_POWER_H */
