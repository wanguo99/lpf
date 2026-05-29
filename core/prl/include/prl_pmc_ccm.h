/**
 * @file prl_pmc_ccm.h
 * @brief PMC-CCM Protocol Definition
 * @details PMC（载荷管理器）与 CCM（通信管理板）之间的通信协议
 *          传输方式：千兆以太网
 */

#ifndef PRL_PMC_CCM_H
#define PRL_PMC_CCM_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PMC-CCM 协议版本 */
#define PRL_PMC_CCM_VERSION     0x0100

/* PMC-CCM 消息类型 */
typedef enum {
    PRL_PMC_CCM_MSG_HEARTBEAT       = 0x01,     /* 心跳 */
    PRL_PMC_CCM_MSG_TELEMETRY       = 0x02,     /* 遥测数据 */
    PRL_PMC_CCM_MSG_COMMAND         = 0x03,     /* 遥控指令 */
    PRL_PMC_CCM_MSG_FIRMWARE_UPDATE = 0x04,     /* 固件升级 */
    PRL_PMC_CCM_MSG_NODE_MANAGE     = 0x05,     /* 节点管理 */
    PRL_PMC_CCM_MSG_POWER_CONTROL   = 0x06,     /* 电源控制 */
    PRL_PMC_CCM_MSG_STATUS_QUERY    = 0x07,     /* 状态查询 */
    PRL_PMC_CCM_MSG_ACK             = 0xFF,     /* 应答 */
} prl_pmc_ccm_msg_type_t;

/* 心跳消息 */
typedef struct {
    uint32_t sender_status;     /* 发送方状态 */
    uint32_t link_quality;      /* 链路质量（0-100） */
    uint32_t packet_loss;       /* 丢包率（百分比 * 100） */
    uint32_t rtt_ms;            /* 往返时延（毫秒） */
} __attribute__((packed)) prl_pmc_ccm_heartbeat_t;

/* 遥测数据 */
typedef struct {
    uint32_t tm_id;             /* 遥测 ID */
    uint32_t tm_source;         /* 遥测来源（设备ID） */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t data_type;         /* 数据类型 */
    uint32_t data_length;       /* 数据长度 */
    uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_telemetry_t;

/* 遥控指令 */
typedef struct {
    uint32_t tc_id;             /* 遥控 ID */
    uint32_t tc_target;         /* 遥控目标（设备ID） */
    uint32_t tc_action;         /* 遥控动作 */
    uint32_t priority;          /* 优先级（0-255） */
    uint32_t param_length;      /* 参数长度 */
    uint8_t  params[0];         /* 参数数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_command_t;

/* 固件升级 */
typedef struct {
    uint32_t firmware_id;       /* 固件 ID */
    uint32_t target_device;     /* 目标设备 */
    uint32_t firmware_version;  /* 固件版本 */
    uint32_t total_size;        /* 固件总大小 */
    uint32_t offset;            /* 当前偏移 */
    uint32_t chunk_size;        /* 本次传输大小 */
    uint8_t  md5[16];           /* MD5 校验 */
    uint8_t  data[0];           /* 固件数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_firmware_update_t;

/* 节点管理 */
typedef struct {
    uint32_t node_id;           /* 节点 ID */
    uint32_t operation;         /* 操作类型：0查询 1启动 2停止 3重启 */
    uint32_t node_type;         /* 节点类型 */
    uint32_t node_status;       /* 节点状态 */
    uint8_t  node_name[64];     /* 节点名称 */
} __attribute__((packed)) prl_pmc_ccm_node_manage_t;

/* 电源控制 */
typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t operation;         /* 操作：0关闭 1打开 2查询 */
    uint32_t voltage_mv;        /* 电压（毫伏） */
    uint32_t current_ma;        /* 电流（毫安） */
    uint32_t power_status;      /* 电源状态 */
} __attribute__((packed)) prl_pmc_ccm_power_control_t;

/* 状态查询 */
typedef struct {
    uint32_t query_type;        /* 查询类型 */
    uint32_t query_target;      /* 查询目标 */
    uint32_t query_param;       /* 查询参数 */
} __attribute__((packed)) prl_pmc_ccm_status_query_t;

/* 应答消息 */
typedef struct {
    uint32_t ack_seq;           /* 应答的序列号 */
    uint8_t  ack_type;          /* 应答的消息类型 */
    uint8_t  result;            /* 结果：0成功，非0失败 */
    uint16_t error_code;        /* 错误码 */
    uint32_t data_length;       /* 附加数据长度 */
    uint8_t  data[0];           /* 附加数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_ack_t;

/* ========== 编码函数 ========== */

int prl_pmc_ccm_encode_heartbeat(const prl_pmc_ccm_heartbeat_t *msg,
                                  uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_telemetry(const prl_pmc_ccm_telemetry_t *msg,
                                  const uint8_t *data, size_t data_len,
                                  uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_command(const prl_pmc_ccm_command_t *msg,
                                const uint8_t *params, size_t params_len,
                                uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_firmware_update(const prl_pmc_ccm_firmware_update_t *msg,
                                        const uint8_t *data, size_t data_len,
                                        uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_node_manage(const prl_pmc_ccm_node_manage_t *msg,
                                    uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_power_control(const prl_pmc_ccm_power_control_t *msg,
                                      uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_status_query(const prl_pmc_ccm_status_query_t *msg,
                                     uint8_t *buf, size_t *len);

int prl_pmc_ccm_encode_ack(const prl_pmc_ccm_ack_t *msg,
                            const uint8_t *data, size_t data_len,
                            uint8_t *buf, size_t *len);

/* ========== 解码函数 ========== */

int prl_pmc_ccm_decode_heartbeat(const uint8_t *buf, size_t len,
                                  prl_pmc_ccm_heartbeat_t *msg);

int prl_pmc_ccm_decode_telemetry(const uint8_t *buf, size_t len,
                                  prl_pmc_ccm_telemetry_t *msg,
                                  uint8_t **data, size_t *data_len);

int prl_pmc_ccm_decode_command(const uint8_t *buf, size_t len,
                                prl_pmc_ccm_command_t *msg,
                                uint8_t **params, size_t *params_len);

int prl_pmc_ccm_decode_firmware_update(const uint8_t *buf, size_t len,
                                        prl_pmc_ccm_firmware_update_t *msg,
                                        uint8_t **data, size_t *data_len);

int prl_pmc_ccm_decode_node_manage(const uint8_t *buf, size_t len,
                                    prl_pmc_ccm_node_manage_t *msg);

int prl_pmc_ccm_decode_power_control(const uint8_t *buf, size_t len,
                                      prl_pmc_ccm_power_control_t *msg);

int prl_pmc_ccm_decode_status_query(const uint8_t *buf, size_t len,
                                     prl_pmc_ccm_status_query_t *msg);

int prl_pmc_ccm_decode_ack(const uint8_t *buf, size_t len,
                            prl_pmc_ccm_ack_t *msg,
                            uint8_t **data, size_t *data_len);

#ifdef __cplusplus
}
#endif

#endif /* PRL_PMC_CCM_H */
