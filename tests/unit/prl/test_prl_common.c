/************************************************************************
 * PRL测试 - 通用功能测试
 ************************************************************************/

#include "test_framework.h"
#include "prl_common.h"
#include <string.h>

/*===========================================================================
 * CRC16 计算测试
 *===========================================================================*/

TEST_CASE(test_prl_crc16_basic)
{
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data3[] = {0x00, 0x00, 0x00, 0x00};
    uint16_t crc;

    /* 测试不同数据的 CRC 计算 */
    crc = prl_calc_crc16(data1, sizeof(data1));
    TEST_ASSERT_NOT_EQUAL(0, crc);

    crc = prl_calc_crc16(data2, sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(0, crc);

    crc = prl_calc_crc16(data3, sizeof(data3));
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

TEST_CASE(test_prl_crc16_consistency)
{
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint16_t crc1, crc2;

    /* 相同数据应该产生相同的 CRC */
    crc1 = prl_calc_crc16(data, sizeof(data));
    crc2 = prl_calc_crc16(data, sizeof(data));
    TEST_ASSERT_EQUAL(crc1, crc2);
}

TEST_CASE(test_prl_crc16_different_data)
{
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x04};
    uint16_t crc1, crc2;

    /* 不同数据应该产生不同的 CRC */
    crc1 = prl_calc_crc16(data1, sizeof(data1));
    crc2 = prl_calc_crc16(data2, sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/*===========================================================================
 * 序列号测试
 *===========================================================================*/

TEST_CASE(test_prl_seq_number)
{
    uint32_t seq1, seq2, seq3;

    /* 序列号应该递增 */
    seq1 = prl_get_next_seq();
    seq2 = prl_get_next_seq();
    seq3 = prl_get_next_seq();

    TEST_ASSERT_TRUE(seq2 > seq1);
    TEST_ASSERT_TRUE(seq3 > seq2);
    TEST_ASSERT_EQUAL(seq1 + 1, seq2);
    TEST_ASSERT_EQUAL(seq2 + 1, seq3);
}

/*===========================================================================
 * 协议头初始化测试
 *===========================================================================*/

TEST_CASE(test_prl_init_header)
{
    prl_header_t hdr;

    /* 初始化协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 100, 0);

    /* 验证字段 */
    TEST_ASSERT_EQUAL(PRL_MAGIC_NUMBER, hdr.magic);
    TEST_ASSERT_EQUAL(PRL_VERSION, hdr.version);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, hdr.dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_HEARTBEAT, hdr.msg_type);
    TEST_ASSERT_EQUAL(0, hdr.flags);
    TEST_ASSERT_EQUAL(100, hdr.length);
    TEST_ASSERT_TRUE(hdr.seq > 0);
    TEST_ASSERT_TRUE(hdr.timestamp > 0);
}

TEST_CASE(test_prl_init_header_with_flags)
{
    prl_header_t hdr;

    /* 初始化带标志位的协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_CCM, PRL_CCM_MSG_TELEMETRY,
                    200, PRL_FLAG_ACK_REQUIRED);

    /* 验证字段 */
    TEST_ASSERT_EQUAL(PRL_MAGIC_NUMBER, hdr.magic);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_CCM, hdr.dev_type);
    TEST_ASSERT_EQUAL(PRL_CCM_MSG_TELEMETRY, hdr.msg_type);
    TEST_ASSERT_EQUAL(PRL_FLAG_ACK_REQUIRED, hdr.flags);
    TEST_ASSERT_EQUAL(200, hdr.length);
}

/*===========================================================================
 * 协议头验证测试
 *===========================================================================*/

TEST_CASE(test_prl_validate_header_valid)
{
    prl_header_t hdr;
    int ret;

    /* 初始化有效的协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_PMC, PRL_PMC_MSG_COMMAND, 50, 0);

    /* 验证应该成功 */
    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(PRL_OK, ret);

    /* 验证特定消息类型 */
    ret = prl_validate_header(&hdr, PRL_PMC_MSG_COMMAND);
    TEST_ASSERT_EQUAL(PRL_OK, ret);
}

TEST_CASE(test_prl_validate_header_invalid_magic)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 篡改魔数 */
    hdr.magic = 0x1234;

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(PRL_ERR_INVALID_MAGIC, ret);
}

TEST_CASE(test_prl_validate_header_invalid_version)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 篡改版本号（主版本不匹配） */
    hdr.version = 0x20;  /* 主版本 = 2 */

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(PRL_ERR_INVALID_VERSION, ret);
}

TEST_CASE(test_prl_validate_header_invalid_type)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 验证错误的消息类型 */
    ret = prl_validate_header(&hdr, PRL_MCU_MSG_GET_VERSION);
    TEST_ASSERT_EQUAL(PRL_ERR_INVALID_TYPE, ret);
}

TEST_CASE(test_prl_validate_header_invalid_length)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 设置超大长度 */
    hdr.length = PRL_MAX_PAYLOAD_SIZE + 1;

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(PRL_ERR_INVALID_LENGTH, ret);
}

/*===========================================================================
 * 报文 CRC 设置和验证测试
 *===========================================================================*/

TEST_CASE(test_prl_packet_crc)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD};
    size_t total_len;

    /* 构造报文 */
    prl_init_header(hdr, PRL_DEV_TYPE_GSC, PRL_GSC_MSG_TELEMETRY,
                    sizeof(payload), 0);
    memcpy(packet + PRL_HEADER_SIZE, payload, sizeof(payload));
    total_len = PRL_HEADER_SIZE + sizeof(payload);

    /* 设置 CRC */
    prl_set_packet_crc(packet, total_len);
    TEST_ASSERT_NOT_EQUAL(0, hdr->crc16);

    /* 验证 CRC 应该成功 */
    TEST_ASSERT_TRUE(prl_verify_packet_crc(packet, total_len));
}

TEST_CASE(test_prl_packet_crc_tampered)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0x11, 0x22, 0x33, 0x44};
    size_t total_len;

    /* 构造报文并设置 CRC */
    prl_init_header(hdr, PRL_DEV_TYPE_POWER, PRL_POWER_MSG_STATUS_REPORT,
                    sizeof(payload), 0);
    memcpy(packet + PRL_HEADER_SIZE, payload, sizeof(payload));
    total_len = PRL_HEADER_SIZE + sizeof(payload);
    prl_set_packet_crc(packet, total_len);

    /* 篡改负载数据 */
    packet[PRL_HEADER_SIZE] ^= 0xFF;

    /* 验证 CRC 应该失败 */
    TEST_ASSERT_FALSE(prl_verify_packet_crc(packet, total_len));
}

TEST_CASE(test_prl_packet_crc_header_tampered)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0x55, 0x66, 0x77, 0x88};
    size_t total_len;

    /* 构造报文并设置 CRC */
    prl_init_header(hdr, PRL_DEV_TYPE_SATELLITE, PRL_SAT_MSG_ORBIT_DATA,
                    sizeof(payload), 0);
    memcpy(packet + PRL_HEADER_SIZE, payload, sizeof(payload));
    total_len = PRL_HEADER_SIZE + sizeof(payload);
    prl_set_packet_crc(packet, total_len);

    /* 篡改协议头（除了 CRC 字段） */
    hdr->flags ^= 0xFF;

    /* 验证 CRC 应该失败 */
    TEST_ASSERT_FALSE(prl_verify_packet_crc(packet, total_len));
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

TEST_CASE(test_prl_zero_length_payload)
{
    uint8_t packet[PRL_HEADER_SIZE];
    prl_header_t *hdr = (prl_header_t *)packet;

    /* 零长度负载 */
    prl_init_header(hdr, PRL_DEV_TYPE_BMC, PRL_BMC_MSG_GET_SENSOR, 0, 0);
    prl_set_packet_crc(packet, PRL_HEADER_SIZE);

    /* 验证应该成功 */
    TEST_ASSERT_TRUE(prl_verify_packet_crc(packet, PRL_HEADER_SIZE));
}

TEST_CASE(test_prl_max_payload)
{
    uint8_t packet[PRL_MAX_PACKET_SIZE];
    prl_header_t *hdr = (prl_header_t *)packet;
    size_t total_len;

    /* 最大负载 */
    prl_init_header(hdr, PRL_DEV_TYPE_PMC, PRL_PMC_MSG_FIRMWARE_UPDATE,
                    PRL_MAX_PAYLOAD_SIZE, 0);

    /* 填充负载数据 */
    for (size_t i = 0; i < PRL_MAX_PAYLOAD_SIZE; i++) {
        packet[PRL_HEADER_SIZE + i] = (uint8_t)(i & 0xFF);
    }

    total_len = PRL_HEADER_SIZE + PRL_MAX_PAYLOAD_SIZE;
    prl_set_packet_crc(packet, total_len);

    /* 验证应该成功 */
    TEST_ASSERT_TRUE(prl_verify_packet_crc(packet, total_len));
}
