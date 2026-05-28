#include "libccm/libccm_protocol.h"

/* 解析遥控命令 */
int32_t PMC_Protocol_ParseTC(const pmc_can_frame_t *frame, pmc_tc_frame_t *tc)
{
    if (!frame || !tc || frame->can_id != PMC_CAN_ID_TC_CMD) {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (frame->dlc < 5) {
        return OSAL_ERR_INVALID_POINTER;
    }

    tc->cmd_type = (pmc_tc_type_t)frame->data[0];
    tc->param = (frame->data[1] << 24) | (frame->data[2] << 16) |
                (frame->data[3] << 8) | frame->data[4];

    return OSAL_SUCCESS;
}

/* 解析遥测请求 */
int32_t PMC_Protocol_ParseTM_Request(const pmc_can_frame_t *frame, pmc_tm_request_t *req)
{
    if (!frame || !req || frame->can_id != PMC_CAN_ID_TM_REQUEST) {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (frame->dlc < 1) {
        return OSAL_ERR_INVALID_POINTER;
    }

    req->tm_type = (pmc_tm_type_t)frame->data[0];

    return OSAL_SUCCESS;
}

/* 构造遥测应答 */
int32_t PMC_Protocol_BuildTM_Response(const pmc_tm_response_t *resp, pmc_can_frame_t *frame)
{
    uint32_t copy_size;

    if (!resp || !frame) {
        return OSAL_ERR_INVALID_POINTER;
    }

    frame->can_id = PMC_CAN_ID_TM_RESPONSE;
    frame->data[0] = (uint8_t)resp->tm_type;
    frame->data[1] = (uint8_t)resp->freshness;

    /* 数据长度限制在6字节内 */
    copy_size = (resp->data_size > 6) ? 6 : resp->data_size;
    OSAL_Memcpy(&frame->data[2], resp->data, copy_size);

    frame->dlc = 2 + copy_size;

    return OSAL_SUCCESS;
}

/* 构造心跳包 */
int32_t PMC_Protocol_BuildHeartbeat(const pmc_heartbeat_t *hb, pmc_can_frame_t *frame)
{
    if (!hb || !frame) {
        return OSAL_ERR_INVALID_POINTER;
    }

    frame->can_id = PMC_CAN_ID_HEARTBEAT;
    frame->data[0] = (hb->sequence >> 24) & 0xFF;
    frame->data[1] = (hb->sequence >> 16) & 0xFF;
    frame->data[2] = (hb->sequence >> 8) & 0xFF;
    frame->data[3] = hb->sequence & 0xFF;
    frame->data[4] = (hb->status >> 24) & 0xFF;
    frame->data[5] = (hb->status >> 16) & 0xFF;
    frame->data[6] = (hb->status >> 8) & 0xFF;
    frame->data[7] = hb->status & 0xFF;
    frame->dlc = 8;

    return OSAL_SUCCESS;
}

/* 编码遥控命令 */
int32_t PMC_Protocol_EncodeTC(const pmc_tc_frame_t *tc, pmc_can_frame_t *frame)
{
    if (!tc || !frame) {
        return OSAL_ERR_INVALID_POINTER;
    }

    frame->can_id = PMC_CAN_ID_TC_CMD;
    frame->data[0] = (uint8_t)tc->cmd_type;
    frame->data[1] = (tc->param >> 24) & 0xFF;
    frame->data[2] = (tc->param >> 16) & 0xFF;
    frame->data[3] = (tc->param >> 8) & 0xFF;
    frame->data[4] = tc->param & 0xFF;
    frame->dlc = 5;

    return OSAL_SUCCESS;
}

/* 编码遥测请求 */
int32_t PMC_Protocol_EncodeTM_Request(const pmc_tm_request_t *req, pmc_can_frame_t *frame)
{
    if (!req || !frame) {
        return OSAL_ERR_INVALID_POINTER;
    }

    frame->can_id = PMC_CAN_ID_TM_REQUEST;
    frame->data[0] = (uint8_t)req->tm_type;
    frame->dlc = 1;

    return OSAL_SUCCESS;
}
