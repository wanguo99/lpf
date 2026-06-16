/************************************************************************
 * PRL测试 - 核心API测试
 * 测试 PRL_Init, PRL_Deinit, PRL_GetVersion, PRL_GetErrorString,
 * PRL_ResetSequence, PRL_GetCurrentSequence
 ************************************************************************/

#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include <test_framework/test_framework.h>

/*===========================================================================
 * 初始化和反初始化测试
 *===========================================================================*/

static void test_prl_Init_success(void)
{
    int ret;

    /* 初始化应该成功 */
    ret = PRL_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 清理 */
    PRL_Deinit();
}

static void test_prl_Deinit_success(void)
{
    int ret;

    /* 初始化 */
    PRL_Init();

    /* 反初始化应该成功 */
    ret = PRL_Deinit();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_prl_Init_Deinit_multiple(void)
{
    int ret;

    /* 多次初始化和反初始化 */
    for (int i = 0; i < 3; i++) {
        ret = PRL_Init();
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        ret = PRL_Deinit();
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }
}

/*===========================================================================
 * 版本信息测试
 *===========================================================================*/

static void test_prl_GetVersion_valid(void)
{
    uint8_t major, minor;

    /* 获取版本信息 */
    PRL_GetVersion(&major, &minor);

    /* 验证版本号 */
    TEST_ASSERT_EQUAL(PRL_VERSION_MAJOR, major);
    TEST_ASSERT_EQUAL(PRL_VERSION_MINOR, minor);
}

static void test_prl_GetVersion_null_params(void)
{
    uint8_t major, minor;

    /* NULL 参数应该安全处理（不崩溃） */
    PRL_GetVersion(NULL, &minor);
    PRL_GetVersion(&major, NULL);
    PRL_GetVersion(NULL, NULL);

    /* 正常调用验证 */
    PRL_GetVersion(&major, &minor);
    TEST_ASSERT_EQUAL(PRL_VERSION_MAJOR, major);
    TEST_ASSERT_EQUAL(PRL_VERSION_MINOR, minor);
}

/*===========================================================================
 * 错误字符串测试
 *===========================================================================*/

static void test_prl_GetErrorString_common_errors(void)
{
    const char *str;

    /* 测试常见错误码 */
    str = PRL_GetErrorString(OSAL_SUCCESS);
    TEST_ASSERT_NOT_NULL(str);

    str = PRL_GetErrorString(OSAL_ERR_INVALID_PARAM);
    TEST_ASSERT_NOT_NULL(str);

    str = PRL_GetErrorString(OSAL_EINVAL);
    TEST_ASSERT_NOT_NULL(str);

    str = PRL_GetErrorString(OSAL_ENOBUFS);
    TEST_ASSERT_NOT_NULL(str);

    str = PRL_GetErrorString(OSAL_EPROTO);
    TEST_ASSERT_NOT_NULL(str);

    str = PRL_GetErrorString(OSAL_EBADMSG);
    TEST_ASSERT_NOT_NULL(str);
}

static void test_prl_GetErrorString_unknown_error(void)
{
    const char *str;

    /* 未知错误码应该返回有效字符串 */
    str = PRL_GetErrorString(-9999);
    TEST_ASSERT_NOT_NULL(str);
}

/*===========================================================================
 * 序列号管理测试
 *===========================================================================*/

static void test_prl_ResetSequence_basic(void)
{
    uint32_t seq;

    /* 重置序列号为特定值 */
    PRL_ResetSequence(100);

    /* 获取当前序列号 */
    seq = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(100, seq);
}

static void test_prl_GetCurrentSequence_increment(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02};
    uint32_t seq1, seq2, seq3;
    int ret;

    /* 重置序列号 */
    PRL_ResetSequence(1000);

    /* 第一次获取 */
    seq1 = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(1000, seq1);

    /* 编码一次消息（会递增序列号） */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 再次获取应该递增 */
    seq2 = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(1001, seq2);

    /* 再编码一次 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 验证继续递增 */
    seq3 = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(1002, seq3);
}

static void test_prl_ResetSequence_zero(void)
{
    uint32_t seq;

    /* 重置为 0 */
    PRL_ResetSequence(0);

    seq = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(0, seq);
}

static void test_prl_ResetSequence_max_value(void)
{
    uint32_t seq;

    /* 重置为最大值 */
    PRL_ResetSequence(UINT32_MAX);

    seq = PRL_GetCurrentSequence();
    TEST_ASSERT_EQUAL(UINT32_MAX, seq);
}

static void test_prl_sequence_in_encoded_packet(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0xAA, 0xBB};
    prl_header_t *hdr = (prl_header_t *)buffer;
    uint32_t expected_seq;
    int ret;

    /* 重置序列号 */
    PRL_ResetSequence(5000);
    expected_seq = PRL_GetCurrentSequence();

    /* 编码消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                     payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 验证报文中的序列号 */
    TEST_ASSERT_EQUAL(OSAL_htonl(expected_seq), hdr->seq);
}

/*===========================================================================
 * 时间戳验证测试
 *===========================================================================*/

static void test_prl_timestamp_populated(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x11, 0x22};
    prl_header_t *hdr = (prl_header_t *)buffer;
    uint32_t timestamp;
    int ret;

    /* 编码消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 验证时间戳不为零（已填充） */
    timestamp = OSAL_ntohl(hdr->timestamp);
    TEST_ASSERT_NOT_EQUAL(0, timestamp);
}

static void test_prl_timestamp_reasonable(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    uint32_t timestamp;
    int ret;

    /* 编码消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 验证时间戳合理（应该接近当前时间） */
    timestamp = OSAL_ntohl(hdr->timestamp);

    /* 时间戳应该在合理范围内（例如：大于 2020-01-01 的时间戳） */
    /* Unix 时间戳 2020-01-01 约为 1577836800 */
    TEST_ASSERT_TRUE(timestamp > 1577836800 || timestamp < 100000);
    /* 注意：如果是相对时间戳（如启动后的秒数），则可能是较小值 */
}

/*===========================================================================
 * 标志位测试
 *===========================================================================*/

static void test_prl_Encode_flag_none(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    int ret;

    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     NULL, 0, buffer, OSAL_sizeof(buffer), PRL_FLAG_NONE);
    TEST_ASSERT_TRUE(ret > 0);

    TEST_ASSERT_EQUAL(PRL_FLAG_NONE, hdr->flags);
}

static void test_prl_Encode_flag_need_ack(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    int ret;

    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_SET_CONFIG,
                     NULL, 0, buffer, OSAL_sizeof(buffer), PRL_FLAG_NEED_ACK);
    TEST_ASSERT_TRUE(ret > 0);

    TEST_ASSERT_EQUAL(PRL_FLAG_NEED_ACK, hdr->flags);
}

static void test_prl_Encode_flag_is_ack(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    int ret;

    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                     NULL, 0, buffer, OSAL_sizeof(buffer), PRL_FLAG_IS_ACK);
    TEST_ASSERT_TRUE(ret > 0);

    TEST_ASSERT_EQUAL(PRL_FLAG_IS_ACK, hdr->flags);
}

static void test_prl_Encode_flag_combination(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    uint8_t flags = PRL_FLAG_NEED_ACK | PRL_FLAG_IS_ACK;
    int ret;

    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     NULL, 0, buffer, OSAL_sizeof(buffer), flags);
    TEST_ASSERT_TRUE(ret > 0);

    TEST_ASSERT_EQUAL(flags, hdr->flags);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

static void test_prl_Encode_exact_buffer_size(void)
{
    uint8_t buffer[PRL_HEADER_SIZE + 10];
    uint8_t payload[10] = {0};
    int ret;

    /* 缓冲区大小恰好等于所需大小 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     payload, OSAL_sizeof(payload),
                     buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(PRL_HEADER_SIZE + 10, ret);
}

static void test_prl_Encode_oversized_payload(void)
{
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t large_payload[PRL_MAX_PAYLOAD_SIZE + 1] = {0};
    int ret;

    /* 负载超过最大值 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     large_payload, OSAL_sizeof(large_payload),
                     buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

static void test_prl_Decode_exact_minimum_size(void)
{
    uint8_t buffer[PRL_HEADER_SIZE];
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    /* 先编码空负载消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                     NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(PRL_HEADER_SIZE, ret);

    /* 解码最小报文 */
    ret = PRL_Decode(buffer, PRL_HEADER_SIZE, &dev_type, &msg_type,
                     &payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0, payload_len);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_prl_Init_success",
        .func = test_prl_Init_success,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Deinit_success",
        .func = test_prl_Deinit_success,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Init_Deinit_multiple",
        .func = test_prl_Init_Deinit_multiple,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_GetVersion_valid",
        .func = test_prl_GetVersion_valid,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_GetVersion_null_params",
        .func = test_prl_GetVersion_null_params,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_GetErrorString_common_errors",
        .func = test_prl_GetErrorString_common_errors,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_GetErrorString_unknown_error",
        .func = test_prl_GetErrorString_unknown_error,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_ResetSequence_basic",
        .func = test_prl_ResetSequence_basic,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_GetCurrentSequence_increment",
        .func = test_prl_GetCurrentSequence_increment,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_ResetSequence_zero",
        .func = test_prl_ResetSequence_zero,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_ResetSequence_max_value",
        .func = test_prl_ResetSequence_max_value,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_sequence_in_encoded_packet",
        .func = test_prl_sequence_in_encoded_packet,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_timestamp_populated",
        .func = test_prl_timestamp_populated,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_timestamp_reasonable",
        .func = test_prl_timestamp_reasonable,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_flag_none",
        .func = test_prl_Encode_flag_none,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_flag_need_ack",
        .func = test_prl_Encode_flag_need_ack,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_flag_is_ack",
        .func = test_prl_Encode_flag_is_ack,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_flag_combination",
        .func = test_prl_Encode_flag_combination,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_exact_buffer_size",
        .func = test_prl_Encode_exact_buffer_size,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Encode_oversized_payload",
        .func = test_prl_Encode_oversized_payload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_prl_Decode_exact_minimum_size",
        .func = test_prl_Decode_exact_minimum_size,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "prl_api",
    .module_name = "prl_api",
    .layer_name = "PRL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 100,
        .description = "PRL core API tests (Init, Version, Sequence, Flags)"
    }
};

__attribute__((constructor))
static void register_prl_api_tests(void)
{
    libutest_register_suite(&test_suite);
}
