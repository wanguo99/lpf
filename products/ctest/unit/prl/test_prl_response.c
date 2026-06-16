/************************************************************************
 * PRL测试 - 应答报文构建测试
 * 测试 PRL_BuildResponse API
 ************************************************************************/

#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include <test_framework/test_framework.h>

/*===========================================================================
 * PRL_BuildResponse 基础测试
 *===========================================================================*/

static void test_prl_BuildResponse_basic(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    uint8_t request_payload[] = {0x01, 0x02};
    uint8_t response_payload[] = {0x11, 0x22, 0x33};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    prl_header_t *response_hdr;
    int ret, request_len, response_len;

    /* 编码请求消息 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             request_payload, OSAL_sizeof(request_payload),
                             request_buffer, OSAL_sizeof(request_buffer),
                             PRL_FLAG_NEED_ACK);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 构建应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      response_payload, OSAL_sizeof(response_payload),
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 解码应答消息 */
    ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证设备类型和消息类型与请求相同 */
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);

    /* 验证应答负载 */
    TEST_ASSERT_EQUAL(OSAL_sizeof(response_payload), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_memcmp(response_payload, decoded_payload, payload_len));

    /* 验证应答标志位（应该有 IS_ACK） */
    response_hdr = (prl_header_t *)response_buffer;
    TEST_ASSERT_TRUE(response_hdr->flags & PRL_FLAG_IS_ACK);
}

static void test_prl_BuildResponse_empty_payload(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret, request_len, response_len;

    /* 编码请求消息 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 构建空应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      NULL, 0,
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 解码应答 */
    ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_HEARTBEAT, msg_type);
    TEST_ASSERT_EQUAL(0, payload_len);
}

static void test_prl_BuildResponse_preserves_sequence(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    prl_header_t *request_hdr = (prl_header_t *)request_buffer;
    prl_header_t *response_hdr = (prl_header_t *)response_buffer;
    uint32_t request_seq;
    int request_len, response_len;

    /* 重置序列号 */
    PRL_ResetSequence(7777);

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_TELEMETRY,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 获取请求序列号 */
    request_seq = OSAL_ntohl(request_hdr->seq);

    /* 构建应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      NULL, 0,
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 验证应答序列号与请求相同 */
    TEST_ASSERT_EQUAL(request_seq, OSAL_ntohl(response_hdr->seq));
    TEST_ASSERT_EQUAL(7777, OSAL_ntohl(response_hdr->seq));
}

static void test_prl_BuildResponse_sets_ack_flag(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    prl_header_t *response_hdr = (prl_header_t *)response_buffer;
    int request_len, response_len;

    /* 编码不带 ACK 标志的请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_SET_CONFIG,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer),
                             PRL_FLAG_NONE);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 构建应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      NULL, 0,
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 验证应答有 IS_ACK 标志 */
    TEST_ASSERT_TRUE(response_hdr->flags & PRL_FLAG_IS_ACK);
}

static void test_prl_BuildResponse_preserves_device_type(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret, request_len, response_len;

    /* 测试多种设备类型 */
    const uint8_t types[] = {
        PRL_DEV_TYPE_MCU,
        PRL_DEV_TYPE_CCM,
        PRL_DEV_TYPE_PMC,
        PRL_DEV_TYPE_GSC,
        PRL_DEV_TYPE_POWER
    };

    for (size_t i = 0; i < OSAL_sizeof(types); i++) {
        /* 编码请求 */
        request_len = PRL_Encode(types[i], 0x01,
                                 NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
        TEST_ASSERT_TRUE(request_len > 0);

        /* 构建应答 */
        response_len = PRL_BuildResponse(request_buffer, request_len,
                                          NULL, 0,
                                          response_buffer, OSAL_sizeof(response_buffer));
        TEST_ASSERT_TRUE(response_len > 0);

        /* 验证设备类型 */
        ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                         &payload, &payload_len);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(types[i], dev_type);
    }
}

/*===========================================================================
 * 错误处理测试
 *===========================================================================*/

static void test_prl_BuildResponse_null_request(void)
{
    uint8_t response_buffer[256];
    uint8_t response_payload[] = {0x01};
    int ret;

    /* NULL 请求报文 */
    ret = PRL_BuildResponse(NULL, 100,
                            response_payload, OSAL_sizeof(response_payload),
                            response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_prl_BuildResponse_null_response_buffer(void)
{
    uint8_t request_buffer[256];
    uint8_t response_payload[] = {0x01};
    int request_len, ret;

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* NULL 应答缓冲区 */
    ret = PRL_BuildResponse(request_buffer, request_len,
                            response_payload, OSAL_sizeof(response_payload),
                            NULL, 256);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_prl_BuildResponse_invalid_request(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    prl_header_t *hdr = (prl_header_t *)request_buffer;
    int request_len, ret;

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 篡改请求魔数 */
    hdr->magic = 0x1234;

    /* 构建应答应该失败 */
    ret = PRL_BuildResponse(request_buffer, request_len,
                            NULL, 0,
                            response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(ret < 0);
}

static void test_prl_BuildResponse_request_too_short(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    int ret;

    /* 编码请求 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
               NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);

    /* 使用过短的请求长度 */
    ret = PRL_BuildResponse(request_buffer, 10,
                            NULL, 0,
                            response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(ret < 0);
}

static void test_prl_BuildResponse_buffer_too_small(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[10];
    uint8_t response_payload[100];
    int request_len, ret;

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 应答缓冲区太小 */
    ret = PRL_BuildResponse(request_buffer, request_len,
                            response_payload, OSAL_sizeof(response_payload),
                            response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_EQUAL(OSAL_ENOBUFS, ret);
}

/*===========================================================================
 * 典型使用场景测试
 *===========================================================================*/

static void test_prl_BuildResponse_request_response_pattern(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    uint8_t request_payload[] = {0xAA};
    uint8_t response_payload[] = {0xBB, 0xCC};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    uint32_t request_seq, response_seq;
    int ret, request_len, response_len;

    /* 步骤1: 客户端发送请求 */
    PRL_ResetSequence(1234);
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                             request_payload, OSAL_sizeof(request_payload),
                             request_buffer, OSAL_sizeof(request_buffer),
                             PRL_FLAG_NEED_ACK);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 步骤2: 服务端接收并解析请求 */
    ret = PRL_Decode(request_buffer, request_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_STATUS, msg_type);

    /* 提取请求序列号 */
    ret = PRL_GetSequence(request_buffer, request_len, &request_seq);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 步骤3: 服务端构建应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      response_payload, OSAL_sizeof(response_payload),
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 步骤4: 客户端接收并验证应答 */
    ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证应答属性 */
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_STATUS, msg_type);
    TEST_ASSERT_EQUAL(OSAL_sizeof(response_payload), payload_len);

    /* 验证序列号匹配 */
    ret = PRL_GetSequence(response_buffer, response_len, &response_seq);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(request_seq, response_seq);
}

static void test_prl_BuildResponse_error_response(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    uint8_t error_code[] = {0xFF};  /* 错误码 */
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret, request_len, response_len;

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_SET_CONFIG,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer),
                             PRL_FLAG_NEED_ACK);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 构建错误应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      error_code, OSAL_sizeof(error_code),
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 验证错误应答 */
    ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(1, payload_len);
    TEST_ASSERT_EQUAL(0xFF, decoded_payload[0]);
}

static void test_prl_BuildResponse_large_response_payload(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[PRL_MAX_PACKET_SIZE];
    uint8_t large_response[1024];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret, request_len, response_len;

    /* 填充大应答负载 */
    for (size_t i = 0; i < OSAL_sizeof(large_response); i++) {
        large_response[i] = (uint8_t)(i & 0xFF);
    }

    /* 编码请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_TELEMETRY,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer), 0);
    TEST_ASSERT_TRUE(request_len > 0);

    /* 构建大应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      large_response, OSAL_sizeof(large_response),
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 验证应答 */
    ret = PRL_Decode(response_buffer, response_len, &dev_type, &msg_type,
                     &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(OSAL_sizeof(large_response), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_memcmp(large_response, decoded_payload, payload_len));
}

static void test_prl_BuildResponse_with_need_ack_request(void)
{
    uint8_t request_buffer[256];
    uint8_t response_buffer[256];
    prl_header_t *request_hdr = (prl_header_t *)request_buffer;
    prl_header_t *response_hdr = (prl_header_t *)response_buffer;
    int request_len, response_len;

    /* 编码带 NEED_ACK 的请求 */
    request_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_SET_CONFIG,
                             NULL, 0, request_buffer, OSAL_sizeof(request_buffer),
                             PRL_FLAG_NEED_ACK);
    TEST_ASSERT_TRUE(request_len > 0);
    TEST_ASSERT_TRUE(request_hdr->flags & PRL_FLAG_NEED_ACK);

    /* 构建应答 */
    response_len = PRL_BuildResponse(request_buffer, request_len,
                                      NULL, 0,
                                      response_buffer, OSAL_sizeof(response_buffer));
    TEST_ASSERT_TRUE(response_len > 0);

    /* 验证应答有 IS_ACK 标志 */
    TEST_ASSERT_TRUE(response_hdr->flags & PRL_FLAG_IS_ACK);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_prl_BuildResponse_basic",
        .func = test_prl_BuildResponse_basic,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_empty_payload",
        .func = test_prl_BuildResponse_empty_payload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_preserves_sequence",
        .func = test_prl_BuildResponse_preserves_sequence,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_sets_ack_flag",
        .func = test_prl_BuildResponse_sets_ack_flag,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_preserves_device_type",
        .func = test_prl_BuildResponse_preserves_device_type,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_null_request",
        .func = test_prl_BuildResponse_null_request,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_null_response_buffer",
        .func = test_prl_BuildResponse_null_response_buffer,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_invalid_request",
        .func = test_prl_BuildResponse_invalid_request,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_request_too_short",
        .func = test_prl_BuildResponse_request_too_short,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_buffer_too_small",
        .func = test_prl_BuildResponse_buffer_too_small,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_request_response_pattern",
        .func = test_prl_BuildResponse_request_response_pattern,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_error_response",
        .func = test_prl_BuildResponse_error_response,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_large_response_payload",
        .func = test_prl_BuildResponse_large_response_payload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_BuildResponse_with_need_ack_request",
        .func = test_prl_BuildResponse_with_need_ack_request,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "prl_response",
    .module_name = "prl_response",
    .layer_name = "PRL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 100,
        .description = "PRL response building API tests (PRL_BuildResponse)"
    }
};

__attribute__((constructor))
static void register_prl_response_tests(void)
{
    libutest_register_suite(&test_suite);
}
