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
#include "test_framework.h"

/*===========================================================================
 * 基础编解码测试
 *===========================================================================*/

TEST_CASE(test_PRL_Encode_decode_basic)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 编码 MCU 版本查询消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 解码消息 */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
    TEST_ASSERT_EQUAL(sizeof(payload), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_Memcmp(payload, decoded_payload, payload_len));
}

TEST_CASE(test_PRL_Encode_empty_payload)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 编码无负载消息（如心跳） */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            NULL, 0, buffer, sizeof(buffer), 0);
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

TEST_CASE(test_prl_device_type_valid)
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

TEST_CASE(test_prl_device_type_name)
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

TEST_CASE(test_PRL_Encode_all_device_types)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0xAA, 0xBB};
    int ret;

    /* 测试所有设备类型的编码 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_GSC, PRL_GSC_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_CCM, PRL_CCM_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    ret = PRL_Encode(PRL_DEV_TYPE_POWER, PRL_POWER_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);
}

/*===========================================================================
 * 错误处理测试
 *===========================================================================*/

TEST_CASE(test_PRL_Encode_invalid_params)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x01, 0x02};
    int ret;

    /* NULL 缓冲区 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, sizeof(payload), NULL, sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM, ret);

    /* 无效设备类型 */
    ret = PRL_Encode(0xFF, PRL_MCU_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, sizeof(buffer), 0);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 缓冲区太小 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            payload, sizeof(payload), buffer, 10, 0);
    TEST_ASSERT_EQUAL(OSAL_ENOBUFS, ret);
}

TEST_CASE(test_PRL_Decode_invalid_params)
{
    uint8_t buffer[256];
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    /* 先编码一个有效消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                            NULL, 0, buffer, sizeof(buffer), 0);
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

TEST_CASE(test_prl_device_crc_verification)
{
    uint8_t buffer[256];
    uint8_t payload[] = {0x11, 0x22, 0x33};
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret, encoded_len;

    /* 编码消息 */
    encoded_len = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                                     payload, sizeof(payload), buffer, sizeof(buffer), 0);
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

TEST_CASE(test_prl_device_large_payload)
{
    uint8_t buffer[4096 + PRL_HEADER_SIZE];
    uint8_t large_payload[4096];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t payload_len;
    int ret;

    /* 填充大负载数据 */
    for (int i = 0; i < sizeof(large_payload); i++) {
        large_payload[i] = (uint8_t)(i & 0xFF);
    }

    /* 编码大负载消息 */
    ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_TELEMETRY,
                            large_payload, sizeof(large_payload),
                            buffer, sizeof(buffer), 0);
    TEST_ASSERT_TRUE(ret > 0);

    /* 解码并验证 */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                            &decoded_payload, &payload_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_PMC, dev_type);
    TEST_ASSERT_EQUAL(PRL_PMC_MSG_TELEMETRY, msg_type);
    TEST_ASSERT_EQUAL(sizeof(large_payload), payload_len);
    TEST_ASSERT_EQUAL(0, OSAL_Memcmp(large_payload, decoded_payload, payload_len));
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_prl_device, "PRL")
    TEST_CASE_REGISTER(test_PRL_Encode_decode_basic, "Encode/decode basic")
    TEST_CASE_REGISTER(test_PRL_Encode_empty_payload, "Encode empty payload")
    TEST_CASE_REGISTER(test_prl_device_type_valid, "Device type valid")
    TEST_CASE_REGISTER(test_prl_device_type_name, "Device type name")
    TEST_CASE_REGISTER(test_PRL_Encode_all_device_types, "Encode all device types")
    TEST_CASE_REGISTER(test_PRL_Encode_invalid_params, "Encode invalid params")
    TEST_CASE_REGISTER(test_PRL_Decode_invalid_params, "Decode invalid params")
    TEST_CASE_REGISTER(test_prl_device_crc_verification, "CRC verification")
    TEST_CASE_REGISTER(test_prl_device_large_payload, "Large payload")
TEST_MODULE_END(test_prl_device, "PRL")
