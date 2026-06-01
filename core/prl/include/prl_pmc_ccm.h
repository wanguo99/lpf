/**
 * @file prl_pmc_ccm.h
 * @brief PMC-CCM Protocol (Deprecated - Compatibility Layer)
 * @deprecated 本文件已废弃，请使用新的按设备类型组织的头文件：
 *             - prl_pmc.h (PMC 设备消息)
 *             - prl_ccm.h (CCM 设备消息)
 *
 * 本文件仅为兼容性保留，将在未来版本中删除。
 */

#ifndef PRL_PMC_CCM_H
#define PRL_PMC_CCM_H

/* 兼容性：包含新的头文件 */
#include "prl_api.h"
#include "prl_pmc.h"
#include "prl_ccm.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 兼容性：旧的类型定义（保持与 PDL 层兼容） */
typedef struct {
    uint32_t sender_status;     /* 发送方状态 */
    uint32_t link_quality;      /* 链路质量（0-100） */
    uint32_t packet_loss;       /* 丢包率（百分比 * 100） */
    uint32_t rtt_ms;            /* 往返时延（毫秒） */
} __attribute__((packed)) prl_pmc_ccm_heartbeat_t;

typedef struct {
    uint32_t tm_id;             /* 遥测 ID */
    uint32_t tm_source;         /* 遥测来源（设备ID） */
    uint64_t timestamp_us;      /* 时间戳（微秒） */
    uint32_t data_type;         /* 数据类型 */
    uint32_t data_length;       /* 数据长度 */
    uint8_t  data[0];           /* 遥测数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_telemetry_t;

typedef struct {
    uint32_t tc_id;             /* 遥控 ID */
    uint32_t cmd_id;            /* 命令 ID */
    uint32_t cmd_type;          /* 命令类型 */
    uint32_t tc_target;         /* 遥控目标 */
    uint32_t tc_action;         /* 遥控动作 */
    uint32_t priority;          /* 优先级 */
    uint32_t param_length;      /* 参数长度 */
    uint8_t  params[0];         /* 命令参数（变长） */
} __attribute__((packed)) prl_pmc_ccm_command_t;

typedef struct {
    uint32_t firmware_id;       /* 固件 ID */
    uint32_t target_device;     /* 目标设备 */
    uint32_t firmware_version;  /* 固件版本 */
    uint32_t total_size;        /* 固件总大小 */
    uint32_t offset;            /* 偏移量 */
    uint32_t chunk_size;        /* 分片大小 */
    uint8_t  md5[16];           /* MD5 校验 */
    uint8_t  chunk_data[0];     /* 分片数据（变长） */
} __attribute__((packed)) prl_pmc_ccm_firmware_update_t;

typedef struct {
    uint32_t operation;         /* 操作类型 */
    uint32_t node_id;           /* 节点 ID */
    uint32_t node_type;         /* 节点类型 */
    uint32_t node_status;       /* 节点状态 */
    char     node_name[32];     /* 节点名称 */
} __attribute__((packed)) prl_pmc_ccm_node_manage_t;

typedef struct {
    uint32_t power_domain;      /* 电源域 */
    uint32_t operation;         /* 操作类型 */
    uint32_t voltage_mv;        /* 电压（毫伏） */
    uint32_t current_ma;        /* 电流（毫安） */
    uint32_t power_status;      /* 电源状态 */
} __attribute__((packed)) prl_pmc_ccm_power_control_t;

typedef struct {
    uint32_t query_type;
    uint32_t query_target;
    uint32_t query_param;
} prl_pmc_ccm_status_query_t;

typedef struct {
    uint32_t ack_code;
    uint32_t ack_seq;
    uint32_t result;
    uint32_t error_code;
    uint32_t ack_data;
} prl_pmc_ccm_ack_t;

/* 兼容性：旧的消息类型映射到新类型 */
#define PRL_PMC_CCM_MSG_HEARTBEAT       PRL_PMC_MSG_HEARTBEAT
#define PRL_PMC_CCM_MSG_TELEMETRY       PRL_PMC_MSG_TELEMETRY
#define PRL_PMC_CCM_MSG_COMMAND         PRL_PMC_MSG_COMMAND
#define PRL_PMC_CCM_MSG_FIRMWARE_UPDATE PRL_PMC_MSG_FIRMWARE_UPDATE
#define PRL_PMC_CCM_MSG_NODE_MANAGE     PRL_PMC_MSG_NODE_MANAGE
#define PRL_PMC_CCM_MSG_POWER_CONTROL   PRL_PMC_MSG_POWER_CONTROL
#define PRL_PMC_CCM_MSG_STATUS_QUERY    PRL_PMC_MSG_STATUS_QUERY
#define PRL_PMC_CCM_MSG_ACK             PRL_PMC_MSG_ACK

/* 兼容性：旧的编解码函数（内联实现，调用新 API） */
static inline int prl_pmc_ccm_encode_heartbeat(const prl_pmc_ccm_heartbeat_t *hb,
                                                 uint8_t *buf, size_t *len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_HEARTBEAT,
                         hb, sizeof(*hb), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_encode_telemetry(const prl_pmc_ccm_telemetry_t *tm,
                                                 const uint8_t *data, uint32_t data_len,
                                                 uint8_t *buf, size_t *len)
{
    /* 简化实现：仅编码固定部分 */
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_TELEMETRY,
                         tm, sizeof(*tm), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_encode_command(const prl_pmc_ccm_command_t *tc,
                                               const uint8_t *params, uint32_t params_len,
                                               uint8_t *buf, size_t *len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_COMMAND,
                         tc, sizeof(*tc), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_encode_firmware_update(const prl_pmc_ccm_firmware_update_t *fw,
                                                       const uint8_t *data, uint32_t len,
                                                       uint8_t *buf, size_t *buf_len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_FIRMWARE_UPDATE,
                         fw, sizeof(*fw), buf, *buf_len, 0);
    if (ret > 0) {
        *buf_len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_encode_node_manage(const prl_pmc_ccm_node_manage_t *nm,
                                                   uint8_t *buf, size_t *len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_NODE_MANAGE,
                         nm, sizeof(*nm), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_encode_power_control(const prl_pmc_ccm_power_control_t *pc,
                                                     uint8_t *buf, size_t *len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_POWER_CONTROL,
                         pc, sizeof(*pc), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

/* 添加缺失的类型定义 - 已移到上面 */

static inline int prl_pmc_ccm_encode_status_query(const prl_pmc_ccm_status_query_t *sq,
                                                    uint8_t *buf, size_t *len)
{
    int ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_STATUS_QUERY,
                         sq, sizeof(*sq), buf, *len, 0);
    if (ret > 0) {
        *len = ret;
        return 0;
    }
    return ret;
}

static inline int prl_pmc_ccm_decode_telemetry(const uint8_t *buf, size_t len,
                                                 prl_pmc_ccm_telemetry_t *tm,
                                                 const uint8_t **data, uint32_t *data_len)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;

    int ret = PRL_Decode(buf, len, &dev_type, &msg_type, &payload, &payload_len);
    if (ret == PRL_OK && payload_len >= sizeof(*tm)) {
        memcpy(tm, payload, sizeof(*tm));
        *data = payload + sizeof(*tm);
        *data_len = payload_len - sizeof(*tm);
        return 0;
    }
    return -1;
}

static inline int prl_pmc_ccm_decode_command(const uint8_t *buf, size_t len,
                                               prl_pmc_ccm_command_t *tc,
                                               const uint8_t **params, uint32_t *params_len)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;

    int ret = PRL_Decode(buf, len, &dev_type, &msg_type, &payload, &payload_len);
    if (ret == PRL_OK && payload_len >= sizeof(*tc)) {
        memcpy(tc, payload, sizeof(*tc));
        *params = payload + sizeof(*tc);
        *params_len = payload_len - sizeof(*tc);
        return 0;
    }
    return -1;
}

static inline int prl_pmc_ccm_decode_ack(const uint8_t *buf, size_t len,
                                           prl_pmc_ccm_ack_t *ack,
                                           const uint8_t **data, size_t *data_len)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;

    int ret = PRL_Decode(buf, len, &dev_type, &msg_type, &payload, &payload_len);
    if (ret == PRL_OK && payload_len >= sizeof(*ack)) {
        memcpy(ack, payload, sizeof(*ack));
        if (data && data_len) {
            *data = payload + sizeof(*ack);
            *data_len = payload_len - sizeof(*ack);
        }
        return 0;
    }
    return -1;
}

#ifdef __cplusplus
}
#endif

#endif /* PRL_PMC_CCM_H */
