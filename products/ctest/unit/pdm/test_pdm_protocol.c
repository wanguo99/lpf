/**
 * @file test_pdm_protocol.c
 * @brief PDM internal protocol unit tests
 */

#include <stdint.h>

#include <test_framework/test_framework.h>
#include "osal.h"
#include "pdm_protocol.h"

static void _test_pdm_protocol_lifecycle_and_version(void)
{
	uint8_t major = 0xFF;
	uint8_t minor = 0xFF;

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pdm_protocol_init());
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pdm_protocol_init());

	pdm_protocol_get_version(&major, &minor);
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_VERSION_MAJOR, major);
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_VERSION_MINOR, minor);
	TEST_ASSERT_NOT_NULL(pdm_protocol_get_error_string(OSAL_EINVAL));

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pdm_protocol_deinit());
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pdm_protocol_deinit());
}

static void _test_pdm_protocol_sequence_helpers(void)
{
	pdm_protocol_reset_sequence(10);

	TEST_ASSERT_EQUAL(10, pdm_protocol_get_current_sequence());
	TEST_ASSERT_EQUAL(10, pdm_protocol_get_next_seq());
	TEST_ASSERT_EQUAL(11, pdm_protocol_get_current_sequence());
}

static void _test_pdm_protocol_header_helpers(void)
{
	pdm_protocol_header_t hdr;

	pdm_protocol_reset_sequence(1);
	pdm_protocol_init_header(&hdr, PDM_PROTOCOL_DEV_TYPE_MCU,
				 PDM_PROTOCOL_MCU_MSG_HEARTBEAT, 4, 0);

	TEST_ASSERT_EQUAL(PDM_PROTOCOL_MAGIC, osal_ntohs(hdr.magic));
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_VERSION, hdr.version);
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_DEV_TYPE_MCU, hdr.dev_type);
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_MCU_MSG_HEARTBEAT, hdr.msg_type);
	TEST_ASSERT_EQUAL(4, osal_ntohs(hdr.length));
	TEST_ASSERT_EQUAL(1, osal_ntohl(hdr.seq));
	TEST_ASSERT_EQUAL(OSAL_SUCCESS,
			  pdm_protocol_validate_header(
				  &hdr, PDM_PROTOCOL_MCU_MSG_HEARTBEAT));

	hdr.magic = osal_htons(0x1234);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS,
			      pdm_protocol_validate_header(
				      &hdr, PDM_PROTOCOL_MCU_MSG_HEARTBEAT));
}

static void _test_pdm_protocol_device_type_helpers(void)
{
	TEST_ASSERT_TRUE(
		pdm_protocol_is_device_type_valid(PDM_PROTOCOL_DEV_TYPE_MCU));
	TEST_ASSERT_FALSE(pdm_protocol_is_device_type_valid(0xFF));
	TEST_ASSERT_STRING_EQUAL(
		"MCU", pdm_protocol_get_device_type_name(PDM_PROTOCOL_DEV_TYPE_MCU));
	TEST_ASSERT_STRING_EQUAL("INVALID",
				 pdm_protocol_get_device_type_name(0xFF));
}

static void _test_pdm_protocol_encode_decode_roundtrip(void)
{
	const uint8_t payload[] = { 0x11, 0x22, 0x33, 0x44 };
	uint8_t packet[PDM_PROTOCOL_HEADER_SIZE + OSAL_sizeof(payload)];
	uint8_t decoded[OSAL_sizeof(payload)];
	pdm_protocol_encode_ctx_t enc = {
		.dev_type = PDM_PROTOCOL_DEV_TYPE_MCU,
		.msg_type = PDM_PROTOCOL_MCU_MSG_WRITE_DATA,
		.flags = 0xA5,
		.payload = payload,
		.payload_len = OSAL_sizeof(payload),
		.buffer = packet,
		.buffer_size = OSAL_sizeof(packet),
	};
	pdm_protocol_decode_ctx_t dec = {
		.buffer = packet,
		.buffer_len = OSAL_sizeof(packet),
		.payload = decoded,
		.payload_size = OSAL_sizeof(decoded),
	};

	TEST_ASSERT_EQUAL((int)OSAL_sizeof(packet),
			  pdm_protocol_device_encode(&enc));
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, pdm_protocol_device_decode(&dec));

	TEST_ASSERT_EQUAL(PDM_PROTOCOL_DEV_TYPE_MCU, dec.dev_type);
	TEST_ASSERT_EQUAL(PDM_PROTOCOL_MCU_MSG_WRITE_DATA, dec.msg_type);
	TEST_ASSERT_EQUAL(0xA5, dec.flags);
	TEST_ASSERT_EQUAL(OSAL_sizeof(payload), dec.payload_len);
	TEST_ASSERT_MEMORY_EQUAL(payload, decoded, OSAL_sizeof(payload));
}

static void _test_pdm_protocol_rejects_invalid_inputs(void)
{
	uint8_t packet[PDM_PROTOCOL_HEADER_SIZE + 1];
	uint8_t payload = 0x5A;
	pdm_protocol_encode_ctx_t enc = {
		.dev_type = PDM_PROTOCOL_DEV_TYPE_MCU,
		.msg_type = PDM_PROTOCOL_MCU_MSG_READ_DATA,
		.payload = &payload,
		.payload_len = OSAL_sizeof(payload),
		.buffer = packet,
		.buffer_size = OSAL_sizeof(packet),
	};
	pdm_protocol_decode_ctx_t dec = {
		.buffer = packet,
		.buffer_len = OSAL_sizeof(packet),
	};

	TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM,
			  pdm_protocol_device_encode(NULL));

	enc.dev_type = 0xFF;
	TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_PARAM,
			  pdm_protocol_device_encode(&enc));

	enc.dev_type = PDM_PROTOCOL_DEV_TYPE_MCU;
	enc.buffer_size = PDM_PROTOCOL_HEADER_SIZE;
	TEST_ASSERT_EQUAL(OSAL_ERR_NO_MEMORY, pdm_protocol_device_encode(&enc));

	enc.buffer_size = OSAL_sizeof(packet);
	TEST_ASSERT_EQUAL((int)OSAL_sizeof(packet),
			  pdm_protocol_device_encode(&enc));

	packet[PDM_PROTOCOL_HEADER_SIZE] ^= 0x01;
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, pdm_protocol_device_decode(&dec));
}

static const test_case_t test_cases[] = {
	{ .name = "test_pdm_protocol_lifecycle_and_version",
	  .func = _test_pdm_protocol_lifecycle_and_version,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pdm_protocol_sequence_helpers",
	  .func = _test_pdm_protocol_sequence_helpers,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pdm_protocol_header_helpers",
	  .func = _test_pdm_protocol_header_helpers,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pdm_protocol_device_type_helpers",
	  .func = _test_pdm_protocol_device_type_helpers,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pdm_protocol_encode_decode_roundtrip",
	  .func = _test_pdm_protocol_encode_decode_roundtrip,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_pdm_protocol_rejects_invalid_inputs",
	  .func = _test_pdm_protocol_rejects_invalid_inputs,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "pdm_protocol",
	.module_name = "pdm",
	.layer_name = "PDM",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
		      .tags = TEST_TAG_FAST,
		      .timeout_ms = 100,
		      .description = "PDM internal protocol unit tests" }
};

void register_pdm_protocol_tests(void)
{
	libutest_register_suite(&test_suite);
}
