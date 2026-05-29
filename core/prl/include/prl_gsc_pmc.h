/**
 * @file prl_gsc_pmc.h
 * @brief GSC-PMC Protocol Definition
 * @details GSC（地面站）与 PMC（载荷管理器）之间的通信协议
 *          传输方式：万兆以太网
 */

#ifndef PRL_GSC_PMC_H
#define PRL_GSC_PMC_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GSC-PMC 协议版本 */
#define PRL_GSC_PMC_VERSION     0x0100

/* GSC-PMC 消息类型 */
typedef enum {
    PRL_GSC_PMC_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_GSC_PMC_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_GSC_PMC_MSG_COMMAND         = 0x03,     /* 遥控指令 */
    PRL_GSC_PMC_MSG_FILE_TRANSFER   = 0x04,     /* 文件传输 */
    PRL_GSC_PMC_MSG_DATABASE_SYNC   = 0x05,     /* 数据库同步 */
    PRL_GSC_PMC_MSG_LOG_UPLOAD      = 0x06,     /* 日志上传 */
    PRL_GSC_PMC_MSG_ACK             = 0xFF,     /* 应答 */
} prl_gsc_pmc_msg_type_t;

/* 心跳消息 */
typedef struct {
    uint32_t pmc_status;        /* PMC 状态 */
    uint32_t cpu_usage;         /* CPU 使用率（百分比 * 100） */
    uint32_t mem_usage;         /* 内存使用率（百分比 * 100） */
    uint32_t disk_usage;        /* 磁盘使用率（百分比 * 100） */
    uint32_t uptime;            /* 运行时间（秒） */
} __attribute__((packed)) prl_gsc_pmc_heartbeat_t;

/* 遥测数据 */
typedef struct {
    uint32_t tm_id;             /* 遥测 ID */
    uint32_t tm_type;           /* 遥测类型 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t value_count;       /* 数据点数量 */
    uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_gsc_pmc_telemetry_t;

/* 遥控指令 */
typedef struct {
    uint32_t tc_id;             /* 遥控 ID */
    uint32_t tc_type;           /* 遥控类型 */
    uint32_t target_device;     /* 目标设备 */
    uint32_t param_count;       /* 参数数量 */
    uint8_t  params[0];         /* 参数数据（变长） */
} __attribute__((packed)) prl_gsc_pmc_command_t;

/* 文件传输 */
typedef struct {
    uint32_t file_id;           /* 文件 ID */
    uint32_t total_size;        /* 文件总大小 */
    uint32_t offset;            /* 当前偏移 */
    uint32_t chunk_size;        /* 本次传输大小 */
    uint8_t  file_name[256];    /* 文件名 */
    uint8_t  data[0];           /* 文件数据（变长） */
} __attribute__((packed)) prl_gsc_pmc_file_transfer_t;

/* 数据库同步 */
typedef struct {
    uint32_t db_type;           /* 数据库类型 */
    uint32_t operation;         /* 操作类型（增删改查） */
    uint32_t record_count;      /* 记录数量 */
    uint8_t  data[0];           /* 数据库记录（变长） */
} __attribute__((packed)) prl_gsc_pmc_database_sync_t;

/* 日志上传 */
typedef struct {
    uint32_t log_level;         /* 日志级别 */
    uint32_t log_source;        /* 日志来源 */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t log_length;        /* 日志长度 */
    uint8_t  log_data[0];       /* 日志内容（变长） */
} __attribute__((packed)) prl_gsc_pmc_log_upload_t;

/* 应答消息 */
typedef struct {
    uint32_t ack_seq;           /* 应答的序列号 */
    uint8_t  ack_type;          /* 应答的消息类型 */
    uint8_t  result;            /* 结果：0成功，非0失败 */
    uint16_t error_code;        /* 错误码 */
} __attribute__((packed)) prl_gsc_pmc_ack_t;

/* ========== 编码函数 ========== */

/**
 * @brief 编码心跳消息
 * @param msg 心跳消息指针
 * @param buf 输出缓冲区
 * @param len 输出长度
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_encode_heartbeat(const prl_gsc_pmc_heartbeat_t *msg,
                                  uint8_t *buf, size_t *len);

/**
 * @brief 编码遥测数据
 * @param msg 遥测消息指针
 * @param data 遥测数据
 * @param data_len 数据长度
 * @param buf 输出缓冲区
 * @param len 输出长度
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_encode_telemetry(const prl_gsc_pmc_telemetry_t *msg,
                                  const uint8_t *data, size_t data_len,
                                  uint8_t *buf, size_t *len);

/**
 * @brief 编码遥控指令
 * @param msg 遥控消息指针
 * @param params 参数数据
 * @param params_len 参数长度
 * @param buf 输出缓冲区
 * @param len 输出长度
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_encode_command(const prl_gsc_pmc_command_t *msg,
                                const uint8_t *params, size_t params_len,
                                uint8_t *buf, size_t *len);

/**
 * @brief 编码应答消息
 * @param msg 应答消息指针
 * @param buf 输出缓冲区
 * @param len 输出长度
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_encode_ack(const prl_gsc_pmc_ack_t *msg,
                            uint8_t *buf, size_t *len);

/* ========== 解码函数 ========== */

/**
 * @brief 解码心跳消息
 * @param buf 输入缓冲区
 * @param len 输入长度
 * @param msg 心跳消息指针
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_decode_heartbeat(const uint8_t *buf, size_t len,
                                  prl_gsc_pmc_heartbeat_t *msg);

/**
 * @brief 解码遥测数据
 * @param buf 输入缓冲区
 * @param len 输入长度
 * @param msg 遥测消息指针
 * @param data 遥测数据输出
 * @param data_len 数据长度输出
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_decode_telemetry(const uint8_t *buf, size_t len,
                                  prl_gsc_pmc_telemetry_t *msg,
                                  uint8_t **data, size_t *data_len);

/**
 * @brief 解码遥控指令
 * @param buf 输入缓冲区
 * @param len 输入长度
 * @param msg 遥控消息指针
 * @param params 参数数据输出
 * @param params_len 参数长度输出
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_decode_command(const uint8_t *buf, size_t len,
                                prl_gsc_pmc_command_t *msg,
                                uint8_t **params, size_t *params_len);

/**
 * @brief 解码应答消息
 * @param buf 输入缓冲区
 * @param len 输入长度
 * @param msg 应答消息指针
 * @return PRL_OK 成功，其他值表示错误
 */
int prl_gsc_pmc_decode_ack(const uint8_t *buf, size_t len,
                            prl_gsc_pmc_ack_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* PRL_GSC_PMC_H */
