/************************************************************************
 * OSAL测试 - CRC校验测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * CRC16-CCITT 测试
 *===========================================================================*/

static void _test_crc16_empty_data(void)
{
	uint8_t data[1] = { 0 };

	/* 测试空数据 */
	uint16_t crc = osal_crc16_ccitt(data, 0);
	TEST_ASSERT_EQUAL(0xFFFF, crc);
}

static void _test_crc16_single_byte(void)
{
	uint8_t data[] = { 0x00 };

	/* 测试单字节 */
	uint16_t crc = osal_crc16_ccitt(data, 1);
	TEST_ASSERT_NOT_EQUAL(0xFFFF, crc);
}

static void _test_crc16_known_values(void)
{
	/* 测试已知的CRC值 */
	uint8_t data1[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39
	}; /* "123456789" */
	uint16_t crc1 = osal_crc16_ccitt(data1, sizeof(data1));
	TEST_ASSERT_EQUAL(0x29B1, crc1); /* CRC16-CCITT标准测试向量 */
}

static void _test_crc16_same_data_same_result(void)
{
	uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

	/* 多次计算应该得到相同结果 */
	uint16_t crc1 = osal_crc16_ccitt(data, sizeof(data));
	uint16_t crc2 = osal_crc16_ccitt(data, sizeof(data));
	TEST_ASSERT_EQUAL(crc1, crc2);
}

static void _test_crc16_different_data_different_result(void)
{
	uint8_t data1[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	uint8_t data2[] = { 0x01, 0x02, 0x03, 0x04, 0x06 }; /* 最后一个字节不同 */

	uint16_t crc1 = osal_crc16_ccitt(data1, sizeof(data1));
	uint16_t crc2 = osal_crc16_ccitt(data2, sizeof(data2));
	TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

static void _test_crc16_all_zeros(void)
{
	uint8_t data[10] = { 0 };

	uint16_t crc = osal_crc16_ccitt(data, sizeof(data));
	TEST_ASSERT_NOT_EQUAL(0, crc);
	TEST_ASSERT_NOT_EQUAL(0xFFFF, crc);
}

static void _test_crc16_all_ones(void)
{
	uint8_t data[10];
	osal_memset(data, 0xFF, sizeof(data));

	uint16_t crc = osal_crc16_ccitt(data, sizeof(data));
	TEST_ASSERT_NOT_EQUAL(0, crc);
	TEST_ASSERT_NOT_EQUAL(0xFFFF, crc);
}

static void _test_crc16_null_pointer(void)
{
	/* 测试NULL指针（应该返回初始值或处理错误） */
	uint16_t crc = osal_crc16_ccitt(NULL, 10);
	TEST_ASSERT_EQUAL(0xFFFF, crc);
}

/*===========================================================================
 * CRC16-CCITT Update 测试
 *===========================================================================*/

static void _test_crc16_update_incremental(void)
{
	uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

	/* 一次性计算 */
	uint16_t crc_full = osal_crc16_ccitt(data, sizeof(data));

	/* 分段计算 */
	uint16_t crc_part = 0xFFFF;
	crc_part = osal_crc16_ccitt_update(crc_part, &data[0], 3);
	crc_part = osal_crc16_ccitt_update(crc_part, &data[3], 3);

	TEST_ASSERT_EQUAL(crc_full, crc_part);
}

static void _test_crc16_update_single_bytes(void)
{
	uint8_t data[] = { 0xAA, 0xBB, 0xCC, 0xDD };

	/* 一次性计算 */
	uint16_t crc_full = osal_crc16_ccitt(data, sizeof(data));

	/* 逐字节计算 */
	uint16_t crc_byte = 0xFFFF;
	for (size_t i = 0; i < sizeof(data); i++) {
		crc_byte = osal_crc16_ccitt_update(crc_byte, &data[i], 1);
	}

	TEST_ASSERT_EQUAL(crc_full, crc_byte);
}

static void _test_crc16_update_with_skip(void)
{
	/* 模拟跳过CRC字段本身的场景 */
	uint8_t data[] = {
		0x01, 0x02, 0x00, 0x00, 0x05, 0x06
	}; /* offset 2-3 是CRC字段 */
	uint8_t zeros[2] = { 0, 0 };

	uint16_t crc = 0xFFFF;
	crc = osal_crc16_ccitt_update(crc, &data[0], 2); /* 前2字节 */
	crc = osal_crc16_ccitt_update(crc, zeros, 2); /* CRC字段作为0 */
	crc = osal_crc16_ccitt_update(crc, &data[4], 2); /* 后2字节 */

	/* 验证结果非零 */
	TEST_ASSERT_NOT_EQUAL(0, crc);
}

static void _test_crc16_update_empty_data(void)
{
	uint16_t crc = 0xFFFF;
	uint8_t data[1] = { 0 };

	/* Update空数据应该不改变CRC */
	uint16_t crc_new = osal_crc16_ccitt_update(crc, data, 0);
	TEST_ASSERT_EQUAL(crc, crc_new);
}

static void _test_crc16_update_null_pointer(void)
{
	uint16_t crc = 0xFFFF;

	/* 测试NULL指针 */
	uint16_t crc_new = osal_crc16_ccitt_update(crc, NULL, 10);
	TEST_ASSERT_EQUAL(crc, crc_new);
}

/*===========================================================================
 * CRC16 数据完整性测试
 *===========================================================================*/

static void _test_crc16_data_corruption_detection(void)
{
	uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

	/* 计算原始CRC */
	uint16_t crc_original = osal_crc16_ccitt(data, sizeof(data));

	/* 模拟单bit错误 */
	data[2] ^= 0x01; /* 翻转一个bit */
	uint16_t crc_corrupted = osal_crc16_ccitt(data, sizeof(data));

	/* CRC应该不同，表示检测到错误 */
	TEST_ASSERT_NOT_EQUAL(crc_original, crc_corrupted);
}

static void _test_crc16_length_sensitivity(void)
{
	uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

	/* 不同长度应该产生不同的CRC */
	uint16_t crc1 = osal_crc16_ccitt(data, 3);
	uint16_t crc2 = osal_crc16_ccitt(data, 4);
	uint16_t crc3 = osal_crc16_ccitt(data, 5);

	TEST_ASSERT_NOT_EQUAL(crc1, crc2);
	TEST_ASSERT_NOT_EQUAL(crc2, crc3);
	TEST_ASSERT_NOT_EQUAL(crc1, crc3);
}

static void _test_crc16_large_data(void)
{
	/* 测试较大数据块 */
	uint8_t data[1024];
	for (size_t i = 0; i < sizeof(data); i++) {
		data[i] = (uint8_t)(i & 0xFF);
	}

	uint16_t crc = osal_crc16_ccitt(data, sizeof(data));
	TEST_ASSERT_NOT_EQUAL(0, crc);
	TEST_ASSERT_NOT_EQUAL(0xFFFF, crc);
}

/*===========================================================================
 * CRC32 测试（预留功能）
 *===========================================================================*/

static void _test_crc32_empty_data(void)
{
	uint8_t data[1] = { 0 };

	/* CRC32暂未实现，应该返回0 */
	uint32_t crc = osal_crc32(data, 0);
	TEST_ASSERT_EQUAL(0, crc);
}

static void _test_crc32_some_data(void)
{
	uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

	/* 标准 CRC32 测试向量 */
	uint32_t crc = osal_crc32(data, sizeof(data));
	TEST_ASSERT_EQUAL(0x470B99F4, crc);
}

static void _test_crc32_update_returns_input(void)
{
	/* CRC32_Update not implemented in API */
	TEST_SKIP("OSAL_crc32_update not implemented");
}

/*===========================================================================
 * 实际应用场景测试
 *===========================================================================*/

static void _test_crc16_packet_validation(void)
{
	/* 模拟数据包：header + payload + crc */
	struct {
		uint8_t header[4];
		uint8_t payload[10];
		uint16_t crc;
	} packet;

	/* 填充数据 */
	packet.header[0] = 0xAA;
	packet.header[1] = 0x55;
	packet.header[2] = 0x01;
	packet.header[3] = 0x0A; /* payload长度 */

	for (int i = 0; i < 10; i++) {
		packet.payload[i] = (uint8_t)i;
	}

	/* 计算CRC（不包括CRC字段本身） */
	uint16_t crc = osal_crc16_ccitt(
		(uint8_t *)&packet, sizeof(packet.header) + sizeof(packet.payload));
	packet.crc = crc;

	/* 验证：重新计算并比较 */
	uint16_t crc_check = osal_crc16_ccitt(
		(uint8_t *)&packet, sizeof(packet.header) + sizeof(packet.payload));
	TEST_ASSERT_EQUAL(packet.crc, crc_check);
}

static void _test_crc16_incremental_stream(void)
{
	/* 模拟流式数据处理 */
	uint8_t stream_data[100];
	for (size_t i = 0; i < sizeof(stream_data); i++) {
		stream_data[i] = (uint8_t)(i * 7); /* 一些伪随机数据 */
	}

	/* 分多次接收数据并增量计算CRC */
	uint16_t crc = 0xFFFF;
	size_t chunk_size = 10;

	for (size_t offset = 0; offset < sizeof(stream_data);
		 offset += chunk_size) {
		size_t len = (offset + chunk_size > sizeof(stream_data)) ?
						 (sizeof(stream_data) - offset) :
						 chunk_size;
		crc = osal_crc16_ccitt_update(crc, &stream_data[offset], len);
	}

	/* 验证与一次性计算的结果相同 */
	uint16_t crc_full = osal_crc16_ccitt(stream_data, sizeof(stream_data));
	TEST_ASSERT_EQUAL(crc_full, crc);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_crc(void)
{
	/* CRC16-CCITT 基础测试 */
	_test_crc16_empty_data();
	_test_crc16_single_byte();
	_test_crc16_known_values();
	_test_crc16_same_data_same_result();
	_test_crc16_different_data_different_result();
	_test_crc16_all_zeros();
	_test_crc16_all_ones();
	_test_crc16_null_pointer();

	/* CRC16-CCITT Update 测试 */
	_test_crc16_update_incremental();
	_test_crc16_update_single_bytes();
	_test_crc16_update_with_skip();
	_test_crc16_update_empty_data();
	_test_crc16_update_null_pointer();

	/* CRC16 数据完整性测试 */
	_test_crc16_data_corruption_detection();
	_test_crc16_length_sensitivity();
	_test_crc16_large_data();

	/* CRC32 测试（预留） */
	_test_crc32_empty_data();
	_test_crc32_some_data();
	_test_crc32_update_returns_input();

	/* 实际应用场景测试 */
	_test_crc16_packet_validation();
	_test_crc16_incremental_stream();
}

static const test_case_t test_cases[] = {
	{ .name = "test_osal_crc",
	  .func = test_osal_crc,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_crc",
	.module_name = "osal_crc",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 1000,
				  .description = "OSAL CRC tests" }
};

void register_osal_crc_tests(void)
{
	libutest_register_suite(&test_suite);
}
