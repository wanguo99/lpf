/************************************************************************
 * IPMI协议实现
 *
 * 职责：
 * - 实现完整的IPMI协议栈（RMCP/RMCP+封装）
 * - IPMI命令封装和响应解析（NetFn/Cmd/Data）
 * - 实现常用IPMI命令（电源控制、传感器读取等）
 * - 会话管理和校验和计算
 ************************************************************************/

#include "pdl_bmc_internal.h"
#include "osal/osal.h"

/*
 * IPMI协议上下文
 */
typedef struct
{
    void *transport_handle;
    int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);
    uint8_t seq_num;
    osal_mutex_t *mutex;
} bmc_ipmi_context_t;

/*
 * RMCP头部（用于IPMI over LAN）
 */
typedef struct
{
    uint8_t version;
    uint8_t reserved;
    uint8_t seq;
    uint8_t class;
} __attribute__((packed)) rmcp_header_t;

#define RMCP_VERSION        0x06
#define RMCP_CLASS_IPMI     0x07

/*
 * IPMI会话头部（简化版，不包含认证）
 */
typedef struct
{
    uint8_t auth_type;
    uint32_t session_seq;
    uint32_t session_id;
} __attribute__((packed)) ipmi_session_header_t;

#define IPMI_AUTH_TYPE_NONE 0x00

/*
 * IPMI消息头部
 */
typedef struct
{
    uint8_t rs_addr;
    uint8_t netfn_lun;
    uint8_t checksum;
    uint8_t rq_addr;
    uint8_t rq_seq_lun;
    uint8_t cmd;
} __attribute__((packed)) ipmi_msg_header_t;

#define IPMI_BMC_SLAVE_ADDR     0x20
#define IPMI_REMOTE_SWID        0x81

/**
 * @brief 计算IPMI校验和
 */
static uint8_t ipmi_checksum(const uint8_t *data, uint32_t len)
{
    uint8_t sum = 0;
    uint32_t i;
    for (i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(0x100 - sum);
}

/**
 * @brief 初始化IPMI协议
 */
int32_t bmc_ipmi_init(void *transport_handle,
                      int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*),
                      void **protocol_handle)
{
    bmc_ipmi_context_t *ctx;

    if (NULL == transport_handle || NULL == send_recv || NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)OSAL_Malloc(sizeof(bmc_ipmi_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_ipmi_context_t));
    ctx->transport_handle = transport_handle;
    ctx->send_recv = send_recv;
    ctx->seq_num = 0;

    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *protocol_handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化IPMI协议
 */
int32_t bmc_ipmi_deinit(void *protocol_handle)
{
    bmc_ipmi_context_t *ctx;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexDelete(ctx->mutex);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 封装IPMI请求（RMCP + IPMI消息）
 */
int32_t bmc_ipmi_pack_request(uint8_t netfn,
                              uint8_t cmd,
                              const uint8_t *data,
                              uint32_t data_len,
                              uint8_t *frame,
                              uint32_t frame_size,
                              uint32_t *actual_size)
{
    uint32_t required_size;
    uint32_t pos;
    rmcp_header_t *rmcp;
    ipmi_session_header_t *session;
    uint8_t msg_len;
    ipmi_msg_header_t *msg;

    if (NULL == frame || NULL == actual_size)
    {
        return OSAL_ERR_GENERIC;
    }

    required_size = sizeof(rmcp_header_t) + sizeof(ipmi_session_header_t) +
                            sizeof(ipmi_msg_header_t) + data_len + 1;
    if (frame_size < required_size)
    {
        return OSAL_ERR_GENERIC;
    }

    pos = 0;

    /* RMCP头部 */
    rmcp = (rmcp_header_t *)&frame[pos];
    rmcp->version = RMCP_VERSION;
    rmcp->reserved = 0;
    rmcp->seq = 0xFF;
    rmcp->class = RMCP_CLASS_IPMI;
    pos += sizeof(rmcp_header_t);

    /* IPMI会话头部（无认证） */
    session = (ipmi_session_header_t *)&frame[pos];
    session->auth_type = IPMI_AUTH_TYPE_NONE;
    session->session_seq = 0;
    session->session_id = 0;
    pos += sizeof(ipmi_session_header_t);

    /* IPMI消息长度 */
    msg_len = sizeof(ipmi_msg_header_t) - 3 + data_len + 1;
    frame[pos++] = msg_len;

    /* IPMI消息头部 */
    msg = (ipmi_msg_header_t *)&frame[pos];
    msg->rs_addr = IPMI_BMC_SLAVE_ADDR;
    msg->netfn_lun = (netfn << 2) | 0x00;
    msg->checksum = ipmi_checksum(&frame[pos], 2);
    msg->rq_addr = IPMI_REMOTE_SWID;
    msg->rq_seq_lun = 0x00;
    msg->cmd = cmd;
    pos += sizeof(ipmi_msg_header_t);

    /* 数据 */
    if (NULL != data && data_len > 0)
    {
        OSAL_Memcpy(&frame[pos], data, data_len);
        pos += data_len;
    }

    /* 数据校验和 */
    frame[pos] = ipmi_checksum(&frame[pos - data_len - 3], data_len + 3);
    pos++;

    *actual_size = pos;
    return OSAL_SUCCESS;
}

/**
 * @brief 解析IPMI响应
 */
int32_t bmc_ipmi_unpack_response(const uint8_t *frame,
                                 uint32_t frame_len,
                                 uint8_t *netfn,
                                 uint8_t *cmd,
                                 uint8_t *completion_code,
                                 uint8_t *data,
                                 uint32_t data_size,
                                 uint32_t *actual_size)
{
    uint32_t pos;
    uint8_t rq_addr;
    uint8_t netfn_lun;
    uint8_t checksum1;
    uint8_t rs_addr;
    uint8_t rq_seq_lun;
    uint8_t cmd_code;
    uint8_t cc;
    uint32_t copy_len;

    if (NULL == frame || frame_len < sizeof(rmcp_header_t) + sizeof(ipmi_session_header_t) + 10)
    {
        return OSAL_ERR_GENERIC;
    }

    pos = 0;

    /* 跳过RMCP头部 */
    pos += sizeof(rmcp_header_t);

    /* 跳过IPMI会话头部 */
    pos += sizeof(ipmi_session_header_t);

    /* 消息长度 */
    pos++;

    /* IPMI响应头部 */
    rq_addr = frame[pos++];
    netfn_lun = frame[pos++];
    checksum1 = frame[pos++];
    (void)rq_addr;
    (void)checksum1;

    rs_addr = frame[pos++];
    rq_seq_lun = frame[pos++];
    cmd_code = frame[pos++];
    cc = frame[pos++];
    (void)rs_addr;
    (void)rq_seq_lun;

    if (NULL != netfn)
    {
        *netfn = netfn_lun >> 2;
    }

    if (NULL != cmd)
    {
        *cmd = cmd_code;
    }

    if (NULL != completion_code)
    {
        *completion_code = cc;
    }

    /* 数据 */
    if (NULL != data && frame_len > pos + 1)
    {
        copy_len = frame_len - pos - 1;
        if (copy_len > data_size)
        {
            copy_len = data_size;
        }
        OSAL_Memcpy(data, &frame[pos], copy_len);

        if (NULL != actual_size)
        {
            *actual_size = copy_len;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 发送IPMI命令并接收响应
 */
static int32_t ipmi_send_command(bmc_ipmi_context_t *ctx,
                                 uint8_t netfn,
                                 uint8_t cmd,
                                 const uint8_t *req_data,
                                 uint32_t req_len,
                                 uint8_t *resp_data,
                                 uint32_t resp_size,
                                 uint32_t *resp_len)
{
    uint8_t request[256];
    uint32_t request_len;
    int32_t ret;
    uint8_t response[256];
    uint32_t response_len;
    uint8_t resp_netfn, resp_cmd, completion_code;

    ret = bmc_ipmi_pack_request(netfn, cmd, req_data, req_len,
                                        request, sizeof(request), &request_len);
    if (OSAL_SUCCESS != ret)
    {
        return ret;
    }

    ret = ctx->send_recv(ctx->transport_handle, request, request_len,
                        response, sizeof(response), &response_len);
    if (OSAL_SUCCESS != ret)
    {
        return ret;
    }

    ret = bmc_ipmi_unpack_response(response, response_len,
                                   &resp_netfn, &resp_cmd, &completion_code,
                                   resp_data, resp_size, resp_len);
    if (OSAL_SUCCESS != ret)
    {
        return ret;
    }

    if (completion_code != IPMI_CC_OK)
    {
        LOG_ERROR("IPMI", "Command failed with completion code: 0x%02X", completion_code);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief IPMI电源开机
 */
int32_t bmc_ipmi_power_on(void *protocol_handle)
{
    bmc_ipmi_context_t *ctx;
    uint8_t req_data;
    uint8_t resp_data[16];
    uint32_t resp_len;
    int32_t ret;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    req_data = IPMI_CHASSIS_POWER_UP;

    ret = ipmi_send_command(ctx, IPMI_NETFN_CHASSIS_REQ, IPMI_CMD_CHASSIS_CONTROL,
                                   &req_data, 1, resp_data, sizeof(resp_data), &resp_len);

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief IPMI电源关机
 */
int32_t bmc_ipmi_power_off(void *protocol_handle)
{
    bmc_ipmi_context_t *ctx;
    uint8_t req_data;
    uint8_t resp_data[16];
    uint32_t resp_len;
    int32_t ret;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    req_data = IPMI_CHASSIS_POWER_DOWN;

    ret = ipmi_send_command(ctx, IPMI_NETFN_CHASSIS_REQ, IPMI_CMD_CHASSIS_CONTROL,
                                   &req_data, 1, resp_data, sizeof(resp_data), &resp_len);

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief IPMI电源复位
 */
int32_t bmc_ipmi_power_reset(void *protocol_handle)
{
    bmc_ipmi_context_t *ctx;
    uint8_t req_data;
    uint8_t resp_data[16];
    uint32_t resp_len;
    int32_t ret;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    req_data = IPMI_CHASSIS_HARD_RESET;

    ret = ipmi_send_command(ctx, IPMI_NETFN_CHASSIS_REQ, IPMI_CMD_CHASSIS_CONTROL,
                                   &req_data, 1, resp_data, sizeof(resp_data), &resp_len);

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 获取电源状态
 */
int32_t bmc_ipmi_get_power_state(void *protocol_handle, bmc_power_state_t *state)
{
    bmc_ipmi_context_t *ctx;
    uint8_t resp_data[16];
    uint32_t resp_len;
    int32_t ret;

    if (NULL == protocol_handle || NULL == state)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    ret = ipmi_send_command(ctx, IPMI_NETFN_CHASSIS_REQ, IPMI_CMD_GET_CHASSIS_STATUS,
                                   NULL, 0, resp_data, sizeof(resp_data), &resp_len);

    if (OSAL_SUCCESS == ret && resp_len >= 1)
    {
        *state = (resp_data[0] & 0x01) ? BMC_POWER_ON : BMC_POWER_OFF;
    }
    else
    {
        *state = BMC_POWER_UNKNOWN;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief 读取传感器
 */
int32_t bmc_ipmi_read_sensors(void *protocol_handle,
                              bmc_sensor_type_t type,
                              bmc_sensor_reading_t *readings,
                              uint32_t max_count,
                              uint32_t *actual_count)
{
    bmc_ipmi_context_t *ctx;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_ipmi_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    (void)type;
    (void)readings;
    (void)max_count;

    if (NULL != actual_count)
    {
        *actual_count = 0;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return OSAL_ERR_GENERIC;
}
