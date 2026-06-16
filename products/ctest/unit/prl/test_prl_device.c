/************************************************************************
 * PRL测试 - 统一设备协议测试
 ************************************************************************/

#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"
#include "prl_power.h"
#include <test_framework/test_framework.h>

/*===========================================================================
 * 基础编解码测试
 *===========================================================================*/

static void test_PRL_Encode_decode_basic(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 编码 MCU 版本查询消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 解码消息 */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
    TEST_ASSERT_EQUAL(OSAL_sizeof(payload), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_memcmp(payload, decoded_payload, payload_len));
}

static void test_PRL_Encode_empty_payload(void)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 编码无负载消息（如心跳） */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 解码消息 */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_HEARTBEAT, msg_type);
    TEST_ASSERT_EQUAL(0, payload_len);
    TEST_ASSERT_NULL(decoded_payload);
}

/*===========================================================================
 * 设备类型测试
 *===========================================================================*/

static void test_prl_device_type_valid(void)
{
    /* 有效的设备类型 */
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_MCU));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_CCM));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_PMC));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_GSC));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_SATELLITE));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_POWER));
    TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_BMC));

    /* 无效的设备类型 */
    TEST_ASSERT_FALSE(PRL_IsDeviceTypeValid(0xFF));
    TEST_ASSERT_FALSE(PRL_IsDeviceTypeValid(0xFF));
}

static void test_prl_device_type_name(void)
{
    /* 验证设备类型名称 */
    TEST_ASSERT_STRING_EQUAL("MCU", PRL_GetDeviceTypeName(PRL_DEV_TYPE_MCU));
    TEST_ASSERT_STRING_EQUAL("CCM", PRL_GetDeviceTypeName(PRL_DEV_TYPE_CCM));
    TEST_ASSERT_STRING_EQUAL("PMC", PRL_GetDeviceTypeName(PRL_DEV_TYPE_PMC));
    TEST_ASSERT_STRING_EQUAL("GSC", PRL_GetDeviceTypeName(PRL_DEV_TYPE_GSC));
    TEST_ASSERT_STRING_EQUAL("SATELLITE", PRL_GetDeviceTypeName(PRL_DEV_TYPE_SATELLITE));
    TEST_ASSERT_STRING_EQUAL("POWER", PRL_GetDeviceTypeName(PRL_DEV_TYPE_POWER));
    TEST_ASSERT_STRING_EQUAL("BMC", PRL_GetDeviceTypeName(PRL_DEV_TYPE_BMC));
    TEST_ASSERT_STRING_EQUAL("UNKNOWN", PRL_GetDeviceTypeName(0xFF));
    TEST_ASSERT_STRING_EQUAL("INVALID", PRL_GetDeviceTypeName(0xFF));
}

/*===========================================================================
 * 多设备类型测试
 *===========================================================================*/

static void test_PRL_Encode_all_device_types(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0xAA, 0xBB};
    int ret;

    /* 测试所有设备类型的编码 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_GSC, PRL_GSC_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_POWER, PRL_POWER_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);
}

/*===========================================================================
 * 错误处理测试
 *===========================================================================*/

static void test_PRL_Encode_invalid_params(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02};
    int ret;

    /* NULL 缓冲区 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), NULL, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* 无效设备类型 */
    ret = PRL_Encode(0xFF, PRL_MCU_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 缓冲区太小 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, OSAL_sizeof(payload), buffer, 10, 0);
    TEST_ASSERT_EQUAL(OSAL_ENOBUFS, ret);
}

static void test_PRL_Decode_invalid_params(void)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    /* 先编码一个有效消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            NULL, 0, buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* NULL 参数 */
    ret = PRL_Decode(NULL, ret, &dev_type, &msg_type, &payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    ret = PRL_Decode(buffer, ret, NULL, &msg_type, &payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* 长度太短 */
    ret = PRL_Decode(buffer, 10, &dev_type, &msg_type, &payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/*===========================================================================
 * CRC 校验测试
 *===========================================================================*/

static void test_prl_device_crc_verification(void)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x11, 0x22, 0x33};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                                     payload, OSAL_sizeof(payload), buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(encoded_len > 0);

    /* 正常解码应该成功 */
    ret = PRL_Decode(buffer, encoded_len, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 篡改数据，CRC 校验应该失败 */
    buffer[PRL_HEADER_SIZE] ^= 0xFF;
    ret = PRL_Decode(buffer, encoded_len, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_EBADMSG, ret);
}

/*===========================================================================
 * 大负载测试
 *===========================================================================*/

static void test_prl_device_large_payload(void)
{
    uint8_t buffer[4096 + PRL_HEADER_SIZE];
    uint8_t large_payload[4096];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 填充大负载数据 */
    for (int i = 0; i < OSAL_sizeof(large_payload); i++) {
        large_payload[i] = (uint8_t)(i & 0xFF);
    }

    /* 编码大负载消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_TELEMETRY,
                            large_payload, OSAL_sizeof(large_payload),
                            buffer, OSAL_sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 解码并验证 */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_PMC, dev_type);
    TEST_ASSERT_EQUAL(PRL_PMC_MSG_TELEMETRY, msg_type);
    TEST_ASSERT_EQUAL(OSAL_sizeof(large_payload), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_memcmp(large_payload, decoded_payload, payload_len));
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_PRL_Encode_decode_basic",
		.func = test_PRL_Encode_decode_basic,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_PRL_Encode_empty_payload",
		.func = test_PRL_Encode_empty_payload,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_device_type_valid",
		.func = test_prl_device_type_valid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_device_type_name",
		.func = test_prl_device_type_name,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_PRL_Encode_all_device_types",
		.func = test_PRL_Encode_all_device_types,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_PRL_Encode_invalid_params",
		.func = test_PRL_Encode_invalid_params,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_PRL_Decode_invalid_params",
		.func = test_PRL_Decode_invalid_params,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_device_crc_verification",
		.func = test_prl_device_crc_verification,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_device_large_payload",
		.func = test_prl_device_large_payload,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "prl_device",
	.module_name = "prl_device",
	.layer_name = "PRL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PRL prl_device tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_prl_device_tests(void)
{
	libutest_register_suite(&test_suite);
}
