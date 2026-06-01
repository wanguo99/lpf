/**
 * @file prl_gsc.h
 * @brief GSC Device Protocol Messages
 * @details GSC（地面站控制器）设备的消息类型和结构体定义
 */

#ifndef PRL_GSC_H
#define PRL_GSC_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GSC 消息类型
 * @note 使用设备类型 PRL_DEV_TYPE_GSC
 */
typedef enum {
    PRL_GSC_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_GSC_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_GSC_MSG_COMMAND         = 0x03,     /* 遥控指令 */
    PRL_GSC_MSG_FILE_TRANSFER   = 0x04,     /* 文件传输 */
    PRL_GSC_MSG_DATABASE_SYNC   = 0x05,     /* 数据库同步 */
    PRL_GSC_MSG_LOG_UPLOAD      = 0x06,     /* 日志上传 */
    PRL_GSC_MSG_ACK             = 0xFF,     /* 应答 */
} prl_gsc_msg_type_t;

/**
 * @brief GSC 心跳消息
 */
typedef struct {
    uint32_t gsc_status;        /* GSC 状态 */
    uint32_t cpu_usage;         /* CPU 使用率（百分比 * 100） */
    uint32_t mem_usage;         /* 内存使用率（百分比 * 100） */
    uint32_t disk_usage;        /* 磁盘使用率（百分比 * 100） */
    uint32_t uptime;            /* 运行时间（秒） */
} __attribute__((packed)) prl_gsc_heartbeat_t;

/**
 * @brief GSC 遥测数据
 */
typedef struct {
    uint32_t tm_id;             /* 遥测 ID */
    uint32_t tm_type;           /* 遥测类型 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t value_count;       /* 数据点数量 */
    uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_gsc_telemetry_t;

/**
 * @brief GSC 遥控指令
 */
typedef struct {
    uint32_t cmd_id;            /* 命令 ID */
    uint32_t cmd_type;          /* 命令类型 */
    uint32_t target_device;     /* 目标设备 */
    uint32_t param_count;       /* 参数数量 */
    uint8_t  params[0];         /* 命令参数（变长） */
} __attribute__((packed)) prl_gsc_command_t;

/**
 * @brief GSC 文件传输
 */
typedef struct {
    uint32_t file_id;           /* 文件 ID */
    uint32_t file_type;         /* 文件类型 */
    uint32_t total_size;        /* 文件总大小 */
    uint32_t chunk_index;       /* 分片索引 */
    uint32_t chunk_size;        /* 分片大小 */
    uint8_t  chunk_data[0];     /* 分片数据（变长） */
} __attribute__((packed)) prl_gsc_file_transfer_t;

/**
 * @brief GSC 数据库同步
 */
typedef struct {
    uint32_t db_type;           /* 数据库类型 */
    uint32_t operation;         /* 操作类型（增删改查） */
    uint32_t record_count;      /* 记录数量 */
    uint8_t  records[0];        /* 记录数据（变长） */
} __attribute__((packed)) prl_gsc_database_sync_t;

/**
 * @brief GSC 日志上传
 */
typedef struct {
    uint32_t log_level;         /* 日志级别 */
    uint32_t log_source;        /* 日志来源 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t log_length;        /* 日志长度 */
    uint8_t  log_data[0];       /* 日志数据（变长） */
} __attribute__((packed)) prl_gsc_log_upload_t;

/**
 * @brief GSC 状态信息
 */
typedef struct {
    uint8_t  connection_status; /* 连接状态 */
    uint8_t  command_queue_len; /* 指令队列长度 */
    uint16_t last_error;        /* 最后错误码 */
    uint32_t uptime;            /* 运行时间（秒） */
} prl_gsc_status_t;

#ifdef __cplusplus
}
#endif

#endif /* PRL_GSC_H */
