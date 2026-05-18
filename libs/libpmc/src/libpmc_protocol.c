#include "libpmc_protocol.h"

/* 解析遥控命令 */
int32 PMC_Protocol_ParseTC(const pmc_can_frame_t *frame, pmc_tc_frame_t *tc)
{
    if (!frame || !tc || frame->can_id != PMC_CAN_ID_TC_CMD) {
        return OS_ERROR;
    }

    if (frame->dlc < 5) {
        return OS_ERROR;
    }

    tc->cmd_type = (pmc_tc_type_t)frame->data[0];
    tc->param = (frame->data[1] << 24) | (frame->data[2] << 16) |
                (frame->data[3] << 8) | frame->data[4];

    return OS_SUCCESS;
}

/* 解析遥测请求 */
int32 PMC_Protocol_ParseTM_Request(const pmc_can_frame_t *frame, pmc_tm_request_t *req)
{
    if (!frame || !req || frame->can_id != PMC_CAN_ID_TM_REQUEST) {
        return OS_ERROR;
    }

    if (frame->dlc < 1) {
        return OS_ERROR;
    }

    req->tm_type = (pmc_tm_type_t)frame->data[0];

    return OS_SUCCESS;
}

/* 构造遥测应答 */
int32 PMC_Protocol_BuildTM_Response(const pmc_tm_response_t *resp, pmc_can_frame_t *frame)
{
    if (!resp || !frame) {
        return OS_ERROR;
    }

    frame->can_id = PMC_CAN_ID_TM_RESPONSE;
    frame->data[0] = (uint8)resp->tm_type;
    frame->data[1] = (uint8)resp->freshness;

    /* 数据长度限制在6字节内 */
    uint32 copy_size = (resp->data_size > 6) ? 6 : resp->data_size;
    OSAL_Memcpy(&frame->data[2], resp->data, copy_size);

    frame->dlc = 2 + copy_size;

    return OS_SUCCESS;
}

/* 构造心跳包 */
int32 PMC_Protocol_BuildHeartbeat(const pmc_heartbeat_t *hb, pmc_can_frame_t *frame)
{
    if (!hb || !frame) {
        return OS_ERROR;
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

    return OS_SUCCESS;
}

/* 编码遥控命令 */
int32 PMC_Protocol_EncodeTC(const pmc_tc_frame_t *tc, pmc_can_frame_t *frame)
{
    if (!tc || !frame) {
        return OS_ERROR;
    }

    frame->can_id = PMC_CAN_ID_TC_CMD;
    frame->data[0] = (uint8)tc->cmd_type;
    frame->data[1] = (tc->param >> 24) & 0xFF;
    frame->data[2] = (tc->param >> 16) & 0xFF;
    frame->data[3] = (tc->param >> 8) & 0xFF;
    frame->data[4] = tc->param & 0xFF;
    frame->dlc = 5;

    return OS_SUCCESS;
}

/* 编码遥测请求 */
int32 PMC_Protocol_EncodeTM_Request(const pmc_tm_request_t *req, pmc_can_frame_t *frame)
{
    if (!req || !frame) {
        return OS_ERROR;
    }

    frame->can_id = PMC_CAN_ID_TM_REQUEST;
    frame->data[0] = (uint8)req->tm_type;
    frame->dlc = 1;

    return OS_SUCCESS;
}
