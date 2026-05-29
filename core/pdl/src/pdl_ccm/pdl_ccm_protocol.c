/************************************************************************
 * CCM 协议处理模块实现
 *
 * 职责：
 * - 调用 PRL_PMC_CCM 协议层进行编解码
 * - 提供简化的协议接口给 PDL_CCM 主模块
 ************************************************************************/

#include "pdl_ccm_internal.h"
#include "prl_pmc_ccm.h"
#include "util/osal_log.h"
#include "lib/osal_errno.h"
#include "osal.h"

/*
 * 编码遥测数据
 */
int32_t ccm_protocol_encode_telemetry(uint32_t tm_id,
                                       uint32_t tm_source,
                                       const uint8_t *data,
                                       size_t data_len,
                                       uint8_t *buf,
                                       size_t *buf_len)
{
    prl_pmc_ccm_telemetry_t tm;

    if (!data || !buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充遥测结构 */
    tm.tm_id = tm_id;
    tm.tm_source = tm_source;
    tm.timestamp_us = OSAL_GetMonotonicTime();
    tm.data_type = 0;  /* 默认数据类型 */
    tm.data_length = data_len;

    /* 调用 PRL 层编码（传递额外的数据参数） */
    return prl_pmc_ccm_encode_telemetry(&tm, data, data_len, buf, buf_len);
}

/*
 * 解码遥测数据
 */
int32_t ccm_protocol_decode_telemetry(const uint8_t *buf,
                                       size_t buf_len,
                                       uint32_t *tm_id,
                                       uint32_t *tm_source,
                                       uint8_t **data,
                                       size_t *data_len)
{
    prl_pmc_ccm_telemetry_t tm;
    int32_t ret;

    if (!buf || !tm_id || !tm_source || !data || !data_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_telemetry(buf, buf_len, &tm, data, data_len);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *tm_id = tm.tm_id;
    *tm_source = tm.tm_source;

    return OSAL_SUCCESS;
}

/*
 * 编码遥控指令
 */
int32_t ccm_protocol_encode_command(uint32_t tc_id,
                                     uint32_t tc_target,
                                     uint32_t tc_action,
                                     const uint8_t *params,
                                     size_t params_len,
                                     uint8_t *buf,
                                     size_t *buf_len)
{
    prl_pmc_ccm_command_t tc;

    if (!buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充遥控结构 */
    tc.tc_id = tc_id;
    tc.tc_target = tc_target;
    tc.tc_action = tc_action;
    tc.priority = 0;  /* 默认优先级 */
    tc.param_length = params_len;

    /* 调用 PRL 层编码（传递额外的参数数据） */
    return prl_pmc_ccm_encode_command(&tc, params, params_len, buf, buf_len);
}

/*
 * 解码遥控指令
 */
int32_t ccm_protocol_decode_command(const uint8_t *buf,
                                     size_t buf_len,
                                     uint32_t *tc_id,
                                     uint32_t *tc_target,
                                     uint32_t *tc_action,
                                     uint8_t **params,
                                     size_t *params_len)
{
    prl_pmc_ccm_command_t tc;
    int32_t ret;

    if (!buf || !tc_id || !tc_target || !tc_action || !params || !params_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_command(buf, buf_len, &tc, params, params_len);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *tc_id = tc.tc_id;
    *tc_target = tc.tc_target;
    *tc_action = tc.tc_action;

    return OSAL_SUCCESS;
}

/*
 * 编码固件升级
 */
int32_t ccm_protocol_encode_firmware_update(uint32_t firmware_id,
                                             uint32_t target_device,
                                             uint32_t firmware_version,
                                             uint32_t total_size,
                                             uint32_t offset,
                                             const uint8_t *data,
                                             size_t data_len,
                                             const uint8_t md5[16],
                                             uint8_t *buf,
                                             size_t *buf_len)
{
    prl_pmc_ccm_firmware_update_t fw;

    if (!data || !md5 || !buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充固件升级结构 */
    fw.firmware_id = firmware_id;
    fw.target_device = target_device;
    fw.firmware_version = firmware_version;
    fw.total_size = total_size;
    fw.offset = offset;
    fw.chunk_size = data_len;
    OSAL_Memcpy(fw.md5, md5, 16);

    /* 调用 PRL 层编码（传递额外的数据参数） */
    return prl_pmc_ccm_encode_firmware_update(&fw, data, data_len, buf, buf_len);
}

/*
 * 编码节点管理
 */
int32_t ccm_protocol_encode_node_manage(uint32_t node_id,
                                         uint32_t operation,
                                         uint8_t *buf,
                                         size_t *buf_len)
{
    prl_pmc_ccm_node_manage_t nm;

    if (!buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充节点管理结构 */
    nm.node_id = node_id;
    nm.operation = operation;
    nm.node_type = 0;
    nm.node_status = 0;
    OSAL_Memset(nm.node_name, 0, sizeof(nm.node_name));

    /* 调用 PRL 层编码 */
    return prl_pmc_ccm_encode_node_manage(&nm, buf, buf_len);
}

/*
 * 解码节点管理
 */
int32_t ccm_protocol_decode_node_manage(const uint8_t *buf,
                                         size_t buf_len,
                                         uint32_t *node_id,
                                         uint32_t *node_status)
{
    prl_pmc_ccm_node_manage_t nm;
    int32_t ret;

    if (!buf || !node_id || !node_status)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_node_manage(buf, buf_len, &nm);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *node_id = nm.node_id;
    *node_status = nm.node_status;

    return OSAL_SUCCESS;
}

/*
 * 编码电源控制
 */
int32_t ccm_protocol_encode_power_control(uint32_t power_domain,
                                           uint32_t operation,
                                           uint8_t *buf,
                                           size_t *buf_len)
{
    prl_pmc_ccm_power_control_t pc;

    if (!buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充电源控制结构 */
    pc.power_domain = power_domain;
    pc.operation = operation;
    pc.voltage_mv = 0;
    pc.current_ma = 0;
    pc.power_status = 0;

    /* 调用 PRL 层编码 */
    return prl_pmc_ccm_encode_power_control(&pc, buf, buf_len);
}

/*
 * 解码电源控制
 */
int32_t ccm_protocol_decode_power_control(const uint8_t *buf,
                                           size_t buf_len,
                                           uint32_t *power_domain,
                                           uint32_t *power_status)
{
    prl_pmc_ccm_power_control_t pc;
    int32_t ret;

    if (!buf || !power_domain || !power_status)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_power_control(buf, buf_len, &pc);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *power_domain = pc.power_domain;
    *power_status = pc.power_status;

    return OSAL_SUCCESS;
}

/*
 * 编码状态查询
 */
int32_t ccm_protocol_encode_status_query(uint32_t query_type,
                                          uint32_t query_target,
                                          uint8_t *buf,
                                          size_t *buf_len)
{
    prl_pmc_ccm_status_query_t sq;

    if (!buf || !buf_len)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 填充状态查询结构 */
    sq.query_type = query_type;
    sq.query_target = query_target;
    sq.query_param = 0;

    /* 调用 PRL 层编码 */
    return prl_pmc_ccm_encode_status_query(&sq, buf, buf_len);
}

/*
 * 解码状态查询
 */
int32_t ccm_protocol_decode_status_query(const uint8_t *buf,
                                          size_t buf_len,
                                          uint32_t *query_type,
                                          uint32_t *result)
{
    prl_pmc_ccm_status_query_t sq;
    int32_t ret;

    if (!buf || !query_type || !result)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_status_query(buf, buf_len, &sq);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *query_type = sq.query_type;
    *result = sq.query_param;  /* 使用 query_param 作为结果 */

    return OSAL_SUCCESS;
}

/*
 * 解码 ACK
 */
int32_t ccm_protocol_decode_ack(const uint8_t *buf,
                                 size_t buf_len,
                                 uint32_t *ack_seq,
                                 uint8_t *result,
                                 uint16_t *error_code)
{
    prl_pmc_ccm_ack_t ack;
    uint8_t *data = NULL;
    size_t data_len = 0;
    int32_t ret;

    if (!buf || !ack_seq || !result || !error_code)
    {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 调用 PRL 层解码 */
    ret = prl_pmc_ccm_decode_ack(buf, buf_len, &ack, &data, &data_len);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 返回解码结果 */
    *ack_seq = ack.ack_seq;
    *result = ack.result;
    *error_code = ack.error_code;

    return OSAL_SUCCESS;
}
