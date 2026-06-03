/**
 * @file prl_power.h (compatibility header)
 * @brief POWER Device Protocol - Compatibility Header
 * @details 兼容性头文件 - 重定向到新的 API 目录
 * @deprecated 请直接包含 "prl.h" 或 "prl_power.h"（从 api 目录）
 */

#ifndef PRL_INCLUDE_PRL_POWER_H
#define PRL_INCLUDE_PRL_POWER_H

#warning "Including prl_power.h from include/ is deprecated. Please include from api/ or use prl.h"

/* 重定向到新的 API 头文件 */
#include "prl_power.h"

#endif /* PRL_INCLUDE_PRL_POWER_H */

/**
 * @brief POWER 消息类型
 * @note 使用设备类型 PRL_DEV_TYPE_POWER
 */
typedef enum {
    PRL_POWER_MSG_HEARTBEAT     = 0x01,     /* 心跳 */
    PRL_POWER_MSG_POWER_ON      = 0x02,     /* 上电 */
    PRL_POWER_MSG_POWER_OFF     = 0x03,     /* 下电 */
    PRL_POWER_MSG_VOLTAGE_QUERY = 0x04,     /* 电压查询 */
    PRL_POWER_MSG_CURRENT_QUERY = 0x05,     /* 电流查询 */
    PRL_POWER_MSG_TEMP_QUERY    = 0x06,     /* 温度查询 */
    PRL_POWER_MSG_STATUS_REPORT = 0x07,     /* 状态上报 */
    PRL_POWER_MSG_ALARM         = 0x08,     /* 告警 */
    PRL_POWER_MSG_ACK           = 0xFF,     /* 应答 */
} prl_power_msg_type_t;

/**
 * @brief 电源域定义
 */
typedef enum {
    PRL_POWER_DOMAIN_MAIN       = 0x01,     /* 主电源 */
    PRL_POWER_DOMAIN_BACKUP     = 0x02,     /* 备用电源 */
    PRL_POWER_DOMAIN_3V3        = 0x03,     /* 3.3V 电源 */
    PRL_POWER_DOMAIN_5V         = 0x04,     /* 5V 电源 */
    PRL_POWER_DOMAIN_12V        = 0x05,     /* 12V 电源 */
    PRL_POWER_DOMAIN_PAYLOAD    = 0x06,     /* 载荷电源 */
} prl_power_domain_t;

/**
 * @brief 告警类型
 */
typedef enum {
    PRL_POWER_ALARM_OVER_VOLTAGE  = 0x01,   /* 过压 */
    PRL_POWER_ALARM_UNDER_VOLTAGE = 0x02,   /* 欠压 */
    PRL_POWER_ALARM_OVER_CURRENT  = 0x03,   /* 过流 */
    PRL_POWER_ALARM_OVER_TEMP     = 0x04,   /* 过温 */
    PRL_POWER_ALARM_SHORT_CIRCUIT = 0x05,   /* 短路 */
    PRL_POWER_ALARM_POWER_FAIL    = 0x06,   /* 电源故障 */
} prl_power_alarm_type_t;

/**
 * @brief POWER 心跳消息
 */
typedef struct {
    uint32_t power_status;      /* 电源状态 */
    uint32_t total_voltage_mv;  /* 总电压（毫伏） */
    uint32_t total_current_ma;  /* 总电流（毫安） */
    uint32_t temperature;       /* 温度（0.01°C） */
} __attribute__((packed)) prl_power_heartbeat_t;

/**
 * @brief POWER 上电命令
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t delay_ms;          /* 延迟时间（毫秒） */
    uint32_t soft_start;        /* 软启动标志 */
} __attribute__((packed)) prl_power_on_t;

/**
 * @brief POWER 下电命令
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t delay_ms;          /* 延迟时间（毫秒） */
    uint32_t force_shutdown;    /* 强制关断标志 */
} __attribute__((packed)) prl_power_off_t;

/**
 * @brief POWER 电压查询
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
} __attribute__((packed)) prl_power_voltage_query_t;

/**
 * @brief POWER 电压响应
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t voltage_mv;        /* 电压（毫伏） */
    uint32_t voltage_min_mv;    /* 最小电压（毫伏） */
    uint32_t voltage_max_mv;    /* 最大电压（毫伏） */
} __attribute__((packed)) prl_power_voltage_response_t;

/**
 * @brief POWER 电流查询
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
} __attribute__((packed)) prl_power_current_query_t;

/**
 * @brief POWER 电流响应
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t current_ma;        /* 电流（毫安） */
    uint32_t current_min_ma;    /* 最小电流（毫安） */
    uint32_t current_max_ma;    /* 最大电流（毫安） */
} __attribute__((packed)) prl_power_current_response_t;

/**
 * @brief POWER 温度查询
 */
typedef struct {
    uint32_t sensor_id;         /* 传感器 ID */
} __attribute__((packed)) prl_power_temp_query_t;

/**
 * @brief POWER 温度响应
 */
typedef struct {
    uint32_t sensor_id;         /* 传感器 ID */
    int32_t  temperature;       /* 温度（0.01°C） */
    int32_t  temp_min;          /* 最小温度（0.01°C） */
    int32_t  temp_max;          /* 最大温度（0.01°C） */
} __attribute__((packed)) prl_power_temp_response_t;

/**
 * @brief POWER 状态上报
 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint8_t  enabled;           /* 使能状态 */
    uint8_t  fault;             /* 故障标志 */
    uint16_t voltage_mv;        /* 输出电压（毫伏） */
    uint16_t current_ma;        /* 输出电流（毫安） */
    uint16_t temperature;       /* 温度（0.1°C） */
} __attribute__((packed)) prl_power_status_report_t;

/**
 * @brief POWER 告警消息
 */
typedef struct {
    uint32_t alarm_type;        /* 告警类型 */
    uint32_t power_domain;      /* 电源域 */
    uint32_t alarm_value;       /* 告警值 */
    uint32_t threshold;         /* 阈值 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
} __attribute__((packed)) prl_power_alarm_t;

/**
 * @brief POWER 状态信息
 */
typedef struct {
    uint8_t  enabled;           /* 使能状态 */
    uint8_t  fault;             /* 故障标志 */
    uint16_t voltage;           /* 输出电压（mV） */
    uint16_t current;           /* 输出电流（mA） */
    uint16_t temperature;       /* 温度（0.1°C） */
} prl_power_status_t;

#ifdef __cplusplus
}
#endif

#endif /* PRL_PRL_POWER_H */
