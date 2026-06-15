/************************************************************************
 * PRL测试 - 通用功能测试
 *
 * 注意: 本测试文件测试PRL内部函数（prl_*开头）
 * 这些是白盒测试，用于验证内部实现细节
 * 对于黑盒测试，请参考 test_prl_device.c
 ************************************************************************/

#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"
#include "prl_power.h"
#include <test/test_framework.h>

/*===========================================================================
 * CRC16 计算测试
 *===========================================================================*/

static void test_prl_crc16_basic(void)
{
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data3[] = {0x00, 0x00, 0x00, 0x00};
    uint16_t crc;

    /* 测试不同数据的 CRC 计算（使用 OSAL CRC 函数） */
    crc = OSAL_CRC16_CCITT(data1, OSAL_sizeof(data1));
    TEST_ASSERT_NOT_EQUAL(0, crc);

    crc = OSAL_CRC16_CCITT(data2, OSAL_sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(0, crc);

    crc = OSAL_CRC16_CCITT(data3, OSAL_sizeof(data3));
    TEST_ASSERT_NOT_EQUAL(0, crc);
}

static void test_prl_crc16_consistency(void)
{
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint16_t crc1, crc2;

    /* 相同数据应该产生相同的 CRC */
    crc1 = OSAL_CRC16_CCITT(data, OSAL_sizeof(data));
    crc2 = OSAL_CRC16_CCITT(data, OSAL_sizeof(data));
    TEST_ASSERT_EQUAL(crc1, crc2);
}

static void test_prl_crc16_different_data(void)
{
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x04};
    uint16_t crc1, crc2;

    /* 不同数据应该产生不同的 CRC */
    crc1 = OSAL_CRC16_CCITT(data1, OSAL_sizeof(data1));
    crc2 = OSAL_CRC16_CCITT(data2, OSAL_sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/*===========================================================================
 * 序列号测试
 *===========================================================================*/

/* 注意: 此测试直接调用内部函数 prl_get_next_seq() 验证序列号递增逻辑 */
static void test_prl_seq_number(void)
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

static void test_prl_init_header(void)
{
    prl_header_t hdr;

    /* 初始化协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 100, 0);

    /* 验证字段（注意：多字节字段使用网络字节序存储） */
    TEST_ASSERT_EQUAL(OSAL_htons(PRL_MAGIC), hdr.magic);
    TEST_ASSERT_EQUAL(PRL_VERSION, hdr.version);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, hdr.dev_type);
    TEST_ASSERT_EQUAL(PRL_MCU_MSG_HEARTBEAT, hdr.msg_type);
    TEST_ASSERT_EQUAL(0, hdr.flags);
    TEST_ASSERT_EQUAL(OSAL_htons(100), hdr.length);
    TEST_ASSERT_TRUE(OSAL_ntohl(hdr.seq) > 0);
    TEST_ASSERT_TRUE(OSAL_ntohl(hdr.timestamp) > 0);
}

static void test_prl_init_header_with_flags(void)
{
    prl_header_t hdr;

    /* 初始化带标志位的协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_CCM, PRL_CCM_MSG_TELEMETRY,
                    200, PRL_FLAG_NEED_ACK);

    /* 验证字段（注意：多字节字段使用网络字节序存储） */
    TEST_ASSERT_EQUAL(OSAL_htons(PRL_MAGIC), hdr.magic);
    TEST_ASSERT_EQUAL(PRL_DEV_TYPE_CCM, hdr.dev_type);
    TEST_ASSERT_EQUAL(PRL_CCM_MSG_TELEMETRY, hdr.msg_type);
    TEST_ASSERT_EQUAL(PRL_FLAG_NEED_ACK, hdr.flags);
    TEST_ASSERT_EQUAL(OSAL_htons(200), hdr.length);
}

/*===========================================================================
 * 协议头验证测试
 *===========================================================================*/

static void test_prl_validate_header_valid(void)
{
    prl_header_t hdr;
    int ret;

    /* 初始化有效的协议头 */
    prl_init_header(&hdr, PRL_DEV_TYPE_PMC, PRL_PMC_MSG_COMMAND, 50, 0);

    /* 验证应该成功 */
    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证特定消息类型 */
    ret = prl_validate_header(&hdr, PRL_PMC_MSG_COMMAND);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_prl_validate_header_invalid_magic(void)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 篡改魔数 */
    hdr.magic = 0x1234;

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(OSAL_EPROTO, ret);
}

static void test_prl_validate_header_invalid_version(void)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 篡改版本号（主版本不匹配） */
    hdr.version = 0x20;  /* 主版本 = 2 */

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(OSAL_EPROTO, ret);
}

static void test_prl_validate_header_invalid_type(void)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 验证错误的消息类型 */
    ret = prl_validate_header(&hdr, PRL_MCU_MSG_GET_VERSION);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

static void test_prl_validate_header_invalid_length(void)
{
    prl_header_t hdr;
    int ret;

    prl_init_header(&hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT, 10, 0);

    /* 设置超大长度（需要转换为网络字节序） */
    hdr.length = OSAL_htons(PRL_MAX_PAYLOAD_SIZE + 1);

    ret = prl_validate_header(&hdr, 0);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);
}

/*===========================================================================
 * 报文 CRC 设置和验证测试
 *===========================================================================*/

static void test_prl_packet_crc(void)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD};
    size_t total_len;

    /* 构造报文 */
    prl_init_header(hdr, PRL_DEV_TYPE_GSC, PRL_GSC_MSG_TELEMETRY,
                    OSAL_sizeof(payload), 0);
    OSAL_memcpy(packet + PRL_HEADER_SIZE, payload, OSAL_sizeof(payload));
    total_len = PRL_HEADER_SIZE + OSAL_sizeof(payload);

    /* 设置 CRC */
    prl_set_packet_crc(packet, total_len);
    TEST_ASSERT_NOT_EQUAL(0, hdr->crc16);

    /* 验证 CRC 应该成功 */
    TEST_ASSERT_TRUE(prl_verify_packet_crc(packet, total_len));
}

static void test_prl_packet_crc_tampered(void)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0x11, 0x22, 0x33, 0x44};
    size_t total_len;

    /* 构造报文并设置 CRC */
    prl_init_header(hdr, PRL_DEV_TYPE_POWER, PRL_POWER_MSG_STATUS_REPORT,
                    OSAL_sizeof(payload), 0);
    OSAL_memcpy(packet + PRL_HEADER_SIZE, payload, OSAL_sizeof(payload));
    total_len = PRL_HEADER_SIZE + OSAL_sizeof(payload);
    prl_set_packet_crc(packet, total_len);

    /* 篡改负载数据 */
    packet[PRL_HEADER_SIZE] ^= 0xFF;

    /* 验证 CRC 应该失败 */
    TEST_ASSERT_FALSE(prl_verify_packet_crc(packet, total_len));
}

static void test_prl_packet_crc_header_tampered(void)
{
    uint8_t packet[256];
    prl_header_t *hdr = (prl_header_t *)packet;
    uint8_t payload[] = {0x55, 0x66, 0x77, 0x88};
    size_t total_len;

    /* 构造报文并设置 CRC */
    prl_init_header(hdr, PRL_DEV_TYPE_CCM, PRL_CCM_MSG_ORBIT_DATA,
                    OSAL_sizeof(payload), 0);
    OSAL_memcpy(packet + PRL_HEADER_SIZE, payload, OSAL_sizeof(payload));
    total_len = PRL_HEADER_SIZE + OSAL_sizeof(payload);
    prl_set_packet_crc(packet, total_len);

    /* 篡改协议头（除了 CRC 字段） */
    hdr->flags ^= 0xFF;

    /* 验证 CRC 应该失败 */
    TEST_ASSERT_FALSE(prl_verify_packet_crc(packet, total_len));
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

static void test_prl_zero_length_payload(void)
{
    uint8_t packet[PRL_HEADER_SIZE];
    prl_header_t *hdr = (prl_header_t *)packet;

    /* 零长度负载 */
    prl_init_header(hdr, PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION, 0, 0);
    prl_set_packet_crc(packet, PRL_HEADER_SIZE);

    /* 验证应该成功 */
    TEST_ASSERT_TRUE(prl_verify_packet_crc(packet, PRL_HEADER_SIZE));
}

static void test_prl_max_payload(void)
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

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_prl_crc16_basic",
		.func = test_prl_crc16_basic,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_crc16_consistency",
		.func = test_prl_crc16_consistency,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_crc16_different_data",
		.func = test_prl_crc16_different_data,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_seq_number",
		.func = test_prl_seq_number,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_init_header",
		.func = test_prl_init_header,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_init_header_with_flags",
		.func = test_prl_init_header_with_flags,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_validate_header_valid",
		.func = test_prl_validate_header_valid,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_validate_header_invalid_magic",
		.func = test_prl_validate_header_invalid_magic,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_validate_header_invalid_version",
		.func = test_prl_validate_header_invalid_version,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_validate_header_invalid_type",
		.func = test_prl_validate_header_invalid_type,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_validate_header_invalid_length",
		.func = test_prl_validate_header_invalid_length,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_packet_crc",
		.func = test_prl_packet_crc,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_packet_crc_tampered",
		.func = test_prl_packet_crc_tampered,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_packet_crc_header_tampered",
		.func = test_prl_packet_crc_header_tampered,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_zero_length_payload",
		.func = test_prl_zero_length_payload,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_prl_max_payload",
		.func = test_prl_max_payload,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "prl_common",
	.module_name = "prl_common",
	.layer_name = "PRL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PRL prl_common tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_prl_common_tests(void)
{
	libutest_register_suite(&test_suite);
}
