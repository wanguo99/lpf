/************************************************************************
 * PRL测试 - 快速提取接口测试
 * 测试 PRL_ValidatePacket, PRL_GetDeviceType, PRL_GetMessageType, PRL_GetSequence
 * 这些是性能关键的路由和去重API
 ************************************************************************/

#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include <test_framework/test_framework.h>

/*===========================================================================
 * PRL_ValidatePacket 测试
 *===========================================================================*/

static void test_PRL_ValidatePacket_valid(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02, 0x03};
    int ret, encoded_len;

    /* 编码有效消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             payload, OSAL_sizeof(payload),
                             buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 验证应该成功 */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_PRL_ValidatePacket_invalid_magic(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改魔数 */
    hdr->magic = OSAL_htons(0x1234);

    /* 验证应该失败 */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_EPROTO, ret);
}

static void test_PRL_ValidatePacket_invalid_version(void)
{
    uint8_t buffer[256];
    prl_header_t *hdr = (prl_header_t *)buffer;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改版本 */
    hdr->version = 0xFF;

    /* 验证应该失败 */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_EPROTO, ret);
}

static void test_PRL_ValidatePacket_invalid_crc(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0xAA, 0xBB};
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             payload, OSAL_sizeof(payload),
                             buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改负载数据（破坏CRC） */
    buffer[PRL_HEADER_SIZE] ^= 0xFF;

    /* 验证应该失败 */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_EBADMSG, ret);
}

static void test_PRL_ValidatePacket_null_packet(void)
{
    int ret;

    /* NULL 报文 */
    ret = PRL_ValidatePacket(NULL, 100);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_PRL_ValidatePacket_too_short(void)
{
    uint8_t buffer[256];
    int ret;

    /* 编码消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer, OSAL_sizeof(buffer), 0);

    /* 长度太短 */
    ret = PRL_ValidatePacket(buffer, PRL_HEADER_SIZE - 1);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

static void test_PRL_ValidatePacket_zero_payload(void)
{
    uint8_t buffer[PRL_HEADER_SIZE];
    int ret, encoded_len;

    /* 编码空负载消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(PRL_HEADER_SIZE, encoded_len);

    /* 验证应该成功 */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * PRL_GetDeviceType 测试
 *===========================================================================*/

static void test_PRL_GetDeviceType_mcu(void)
{
    uint8_t buffer[256];
    uint8_t dev_type;
    int ret, encoded_len;

    /* 编码 MCU 消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 提取设备类型 */
    ret = PRL_GetDeviceType(buffer, encoded_len, &dev_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
}

static void test_PRL_GetDeviceType_all_types(void)
{
    uint8_t buffer[256];
    uint8_t dev_type;
    int ret, encoded_len;

    /* 测试所有设备类型 */
    const uint8_t types[] = {
        PRL_DEV_TYPE_MCU,
        PRL_DEV_TYPE_CCM,
        PRL_DEV_TYPE_PMC,
        PRL_DEV_TYPE_GSC,
        PRL_DEV_TYPE_POWER,
        PRL_DEV_TYPE_SATELLITE,
        PRL_DEV_TYPE_BMC
    };

    for (size_t i = 0; i < OSAL_sizeof(types); i++) {
        /* 编码消息 */
        encoded_len = PRL_Encode(types[i], 0x01,
                                 NULL, 0, buffer, OSAL_sizeof(buffer), 0);
        TEST_ASSERT_TRUE(encoded_len > 0);

        /* 提取并验证设备类型 */
        ret = PRL_GetDeviceType(buffer, encoded_len, &dev_type);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(types[i], dev_type);
    }
}

static void test_PRL_GetDeviceType_null_params(void)
{
    uint8_t buffer[256];
    uint8_t dev_type;
    int ret;

    /* 编码消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer, OSAL_sizeof(buffer), 0);

    /* NULL 报文 */
    ret = PRL_GetDeviceType(NULL, PRL_HEADER_SIZE, &dev_type);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* NULL 输出参数 */
    ret = PRL_GetDeviceType(buffer, PRL_HEADER_SIZE, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_PRL_GetDeviceType_too_short(void)
{
    uint8_t buffer[256];
    uint8_t dev_type;
    int ret;

    /* 编码消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer, OSAL_sizeof(buffer), 0);

    /* 长度太短（无法读取协议头） */
    ret = PRL_GetDeviceType(buffer, 5, &dev_type);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

static void test_PRL_GetDeviceType_no_crc_check(void)
{
    uint8_t buffer[256];
    uint8_t dev_type;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_TELEMETRY,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改负载数据（破坏CRC，但不影响协议头） */
    if (encoded_len > PRL_HEADER_SIZE) {
        buffer[PRL_HEADER_SIZE] ^= 0xFF;
    }

    /* 快速接口应该仍然成功（不验证CRC） */
    ret = PRL_GetDeviceType(buffer, encoded_len, &dev_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_CCM, dev_type);
}

/*===========================================================================
 * PRL_GetMessageType 测试
 *===========================================================================*/

static void test_PRL_GetMessageType_basic(void)
{
    uint8_t buffer[256];
    uint8_t msg_type;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 提取消息类型 */
    ret = PRL_GetMessageType(buffer, encoded_len, &msg_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
}

static void test_PRL_GetMessageType_various_types(void)
{
    uint8_t buffer[256];
    uint8_t msg_type;
    int ret, encoded_len;

    /* 测试不同消息类型 */
    const uint8_t test_msg_types[] = {
        PRL_MCU_MSG_HEARTBEAT,
        PRL_MCU_MSG_GET_VERSION,
        PRL_MCU_MSG_GET_STATUS,
        PRL_MCU_MSG_SET_CONFIG,
        PRL_MCU_MSG_RESET
    };

    for (size_t i = 0; i < OSAL_sizeof(test_msg_types); i++) {
        encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, test_msg_types[i],
                                 NULL, 0, buffer, OSAL_sizeof(buffer), 0);
        TEST_ASSERT_TRUE(encoded_len > 0);

        ret = PRL_GetMessageType(buffer, encoded_len, &msg_type);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        TEST_ASSERT_EQUAL(test_msg_types[i], msg_type);
    }
}

static void test_PRL_GetMessageType_null_params(void)
{
    uint8_t buffer[256];
    uint8_t msg_type;
    int ret;

    /* 编码消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer, OSAL_sizeof(buffer), 0);

    /* NULL 报文 */
    ret = PRL_GetMessageType(NULL, PRL_HEADER_SIZE, &msg_type);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* NULL 输出参数 */
    ret = PRL_GetMessageType(buffer, PRL_HEADER_SIZE, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_PRL_GetMessageType_no_crc_check(void)
{
    uint8_t buffer[256];
    uint8_t msg_type;
    uint8_t payload[] = {0x11, 0x22};
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_COMMAND,
                             payload, OSAL_sizeof(payload),
                             buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改负载（破坏CRC） */
    buffer[PRL_HEADER_SIZE] ^= 0xFF;

    /* 快速接口应该仍然成功（不验证CRC） */
    ret = PRL_GetMessageType(buffer, encoded_len, &msg_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_PMC_MSG_COMMAND, msg_type);
}

/*===========================================================================
 * PRL_GetSequence 测试
 *===========================================================================*/

static void test_PRL_GetSequence_basic(void)
{
    uint8_t buffer[256];
    uint32_t seq, expected_seq;
    int ret, encoded_len;

    /* 重置序列号 */
    PRL_ResetSequence(12345);
    expected_seq = PRL_GetCurrentSequence();

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 提取序列号 */
    ret = PRL_GetSequence(buffer, encoded_len, &seq);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(expected_seq, seq);
}

static void test_PRL_GetSequence_multiple_packets(void)
{
    uint8_t buffer1[256], buffer2[256], buffer3[256];
    uint32_t seq1, seq2, seq3;
    int ret;

    /* 重置序列号 */
    PRL_ResetSequence(1000);

    /* 编码三个消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer1, OSAL_sizeof(buffer1), 0);
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer2, OSAL_sizeof(buffer2), 0);
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer3, OSAL_sizeof(buffer3), 0);

    /* 提取序列号 */
    ret = PRL_GetSequence(buffer1, PRL_HEADER_SIZE, &seq1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PRL_GetSequence(buffer2, PRL_HEADER_SIZE, &seq2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PRL_GetSequence(buffer3, PRL_HEADER_SIZE, &seq3);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证序列号递增 */
    TEST_ASSERT_EQUAL(1000, seq1);
    TEST_ASSERT_EQUAL(1001, seq2);
    TEST_ASSERT_EQUAL(1002, seq3);
}

static void test_PRL_GetSequence_null_params(void)
{
    uint8_t buffer[256];
    uint32_t seq;
    int ret;

    /* 编码消息 */
    PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
               NULL, 0, buffer, OSAL_sizeof(buffer), 0);

    /* NULL 报文 */
    ret = PRL_GetSequence(NULL, PRL_HEADER_SIZE, &seq);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* NULL 输出参数 */
    ret = PRL_GetSequence(buffer, PRL_HEADER_SIZE, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);
}

static void test_PRL_GetSequence_no_crc_check(void)
{
    uint8_t buffer[256];
    uint32_t seq;
    uint8_t payload[] = {0x55, 0x66};
    int ret, encoded_len;

    /* 重置序列号 */
    PRL_ResetSequence(9999);

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_ORBIT_DATA,
                             payload, OSAL_sizeof(payload),
                             buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 篡改负载（破坏CRC） */
    buffer[PRL_HEADER_SIZE] ^= 0xFF;

    /* 快速接口应该仍然成功（不验证CRC） */
    ret = PRL_GetSequence(buffer, encoded_len, &seq);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(9999, seq);
}

static void test_PRL_GetSequence_deduplication_scenario(void)
{
    uint8_t buffer[256];
    uint32_t seq1, seq2;
    int ret, encoded_len;

    /* 模拟重复接收的场景 */
    PRL_ResetSequence(5000);

    /* 编码一次 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 模拟接收同一报文两次 */
    ret = PRL_GetSequence(buffer, encoded_len, &seq1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PRL_GetSequence(buffer, encoded_len, &seq2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 序列号应该相同（用于去重判断） */
    TEST_ASSERT_EQUAL(seq1, seq2);
}

/*===========================================================================
 * 性能关键场景测试
 *===========================================================================*/

static void test_fast_routing_scenario(void)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    uint32_t seq;
    int ret, encoded_len;

    /* 模拟快速路由场景：只需要设备类型和消息类型 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_TELEMETRY,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 快速提取路由信息（无需完整解码） */
    ret = PRL_GetDeviceType(buffer, encoded_len, &dev_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PRL_GetMessageType(buffer, encoded_len, &msg_type);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PRL_GetSequence(buffer, encoded_len, &seq);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证提取的信息 */
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_PMC, dev_type);
    TEST_ASSERT_EQUAL(PRL_PMC_MSG_TELEMETRY, msg_type);
    TEST_ASSERT_TRUE(seq > 0);
}

static void test_validate_before_decode_scenario(void)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret, encoded_len;

    /* 场景：先快速验证，再完整解码 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 第一步：快速验证（避免完整解码损坏的报文） */
    ret = PRL_ValidatePacket(buffer, encoded_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二步：完整解码 */
    ret = PRL_Decode(buffer, encoded_len, &dev_type, &msg_type,
                     &payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
    {
        .name = "test_PRL_ValidatePacket_valid",
        .func = test_PRL_ValidatePacket_valid,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_invalid_magic",
        .func = test_PRL_ValidatePacket_invalid_magic,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_invalid_version",
        .func = test_PRL_ValidatePacket_invalid_version,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_invalid_crc",
        .func = test_PRL_ValidatePacket_invalid_crc,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_null_packet",
        .func = test_PRL_ValidatePacket_null_packet,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_too_short",
        .func = test_PRL_ValidatePacket_too_short,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_ValidatePacket_zero_payload",
        .func = test_PRL_ValidatePacket_zero_payload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetDeviceType_mcu",
        .func = test_PRL_GetDeviceType_mcu,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetDeviceType_all_types",
        .func = test_PRL_GetDeviceType_all_types,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetDeviceType_null_params",
        .func = test_PRL_GetDeviceType_null_params,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetDeviceType_too_short",
        .func = test_PRL_GetDeviceType_too_short,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetDeviceType_no_crc_check",
        .func = test_PRL_GetDeviceType_no_crc_check,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetMessageType_basic",
        .func = test_PRL_GetMessageType_basic,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetMessageType_various_types",
        .func = test_PRL_GetMessageType_various_types,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetMessageType_null_params",
        .func = test_PRL_GetMessageType_null_params,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetMessageType_no_crc_check",
        .func = test_PRL_GetMessageType_no_crc_check,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetSequence_basic",
        .func = test_PRL_GetSequence_basic,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetSequence_multiple_packets",
        .func = test_PRL_GetSequence_multiple_packets,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetSequence_null_params",
        .func = test_PRL_GetSequence_null_params,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetSequence_no_crc_check",
        .func = test_PRL_GetSequence_no_crc_check,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_PRL_GetSequence_deduplication_scenario",
        .func = test_PRL_GetSequence_deduplication_scenario,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_fast_routing_scenario",
        .func = test_fast_routing_scenario,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_validate_before_decode_scenario",
        .func = test_validate_before_decode_scenario,
        .setup = NULL,
        .teardown = NULL
    },
};

static const test_suite_t test_suite = {
    .suite_name = "prl_fast_extraction",
    .module_name = "prl_fast_extraction",
    .layer_name = "PRL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 100,
        .description = "PRL fast extraction APIs (ValidatePacket, GetDeviceType, GetMessageType, GetSequence)"
    }
};

__attribute__((constructor))
static void register_prl_fast_extraction_tests(void)
{
    libutest_register_suite(&test_suite);
}
