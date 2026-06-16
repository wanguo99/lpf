/**
 * @file test_system_prl_error_recovery.c
 * @brief PRL System Test - Error Handling and Recovery
 *
 * Tests error injection, detection, and recovery scenarios to verify
 * system robustness and stability under adverse conditions.
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"

/**
 * Setup: Initialize PRL
 */
static void setup_prl(void)
{
	PRL_Init();
}

/**
 * Teardown: Cleanup PRL
 */
static void teardown_prl(void)
{
	PRL_Deinit();
}

/**
 * Test: Corrupted packet detection
 */
static void test_corrupted_packet_detection(void)
{
	OSAL_printf("[ TEST     ] Corrupted packet detection\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02, 0x03};
	int ret;

	/* Create valid packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int packet_len = ret;

	/* Verify it's valid */
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Corrupt magic number */
	buffer[0] = 0xFF;
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Corrupted magic detected: %s\n",
	           PRL_GetErrorString(ret));

	/* Restore and corrupt version */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	buffer[2] = 0xFF; /* Invalid version */
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Invalid version detected: %s\n",
	           PRL_GetErrorString(ret));

	/* Restore and corrupt CRC */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	prl_header_t *hdr = (prl_header_t *)buffer;
	hdr->crc16 ^= 0xFFFF;
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] CRC mismatch detected: %s\n",
	           PRL_GetErrorString(ret));

	OSAL_printf("[ PASS     ] Corrupted packet detection test passed\n");
}

/**
 * Test: Invalid device type rejection
 */
static void test_invalid_device_type(void)
{
	OSAL_printf("[ TEST     ] Invalid device type rejection\n");

	uint8_t buffer[256];

	/* Try to encode with invalid device type */
	int ret = PRL_Encode(0xFF, 0x01, NULL, 0,
	                     buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret < 0);
	OSAL_printf("[ INFO     ] Invalid device type encoding rejected: %s\n",
	           PRL_GetErrorString(ret));

	/* Verify device type validation */
	TEST_ASSERT_FALSE(PRL_IsDeviceTypeValid(0x00));
	TEST_ASSERT_FALSE(PRL_IsDeviceTypeValid(0xFF));
	TEST_ASSERT_FALSE(PRL_IsDeviceTypeValid(0x10));

	/* Verify valid types still work */
	TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_MCU));
	TEST_ASSERT_TRUE(PRL_IsDeviceTypeValid(PRL_DEV_TYPE_CCM));

	OSAL_printf("[ PASS     ] Invalid device type test passed\n");
}

/**
 * Test: Buffer overflow protection
 */
static void test_buffer_overflow_protection(void)
{
	OSAL_printf("[ TEST     ] Buffer overflow protection\n");

	uint8_t small_buffer[32];
	uint8_t large_payload[4096];
	int ret;

	/* Try to encode large payload into small buffer */
	OSAL_memset(large_payload, 0xAA, sizeof(large_payload));
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 large_payload, sizeof(large_payload),
	                 small_buffer, sizeof(small_buffer), 0);
	TEST_ASSERT_TRUE(ret < 0);
	OSAL_printf("[ INFO     ] Buffer overflow prevented: %s\n",
	           PRL_GetErrorString(ret));

	/* Try to encode payload exceeding max size */
	uint8_t buffer[8192];
	uint8_t huge_payload[PRL_MAX_PAYLOAD_SIZE + 1];
	OSAL_memset(huge_payload, 0xBB, sizeof(huge_payload));

	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 huge_payload, sizeof(huge_payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret < 0);
	OSAL_printf("[ INFO     ] Oversized payload rejected: %s\n",
	           PRL_GetErrorString(ret));

	OSAL_printf("[ PASS     ] Buffer overflow protection test passed\n");
}

/**
 * Test: Partial packet handling
 */
static void test_partial_packet_handling(void)
{
	OSAL_printf("[ TEST     ] Partial packet handling\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02, 0x03};
	int ret;

	/* Create valid packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int full_len = ret;

	/* Try to decode with truncated header */
	uint8_t dev_type, msg_type;
	const uint8_t *decoded_payload;
	uint16_t payload_len;

	ret = PRL_Decode(buffer, PRL_HEADER_SIZE - 1,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Truncated header rejected: %s\n",
	           PRL_GetErrorString(ret));

	/* Try to decode with truncated payload */
	ret = PRL_Decode(buffer, PRL_HEADER_SIZE + 1,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Truncated payload rejected: %s\n",
	           PRL_GetErrorString(ret));

	/* Verify full packet decodes successfully */
	ret = PRL_Decode(buffer, full_len,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Partial packet handling test passed\n");
}

/**
 * Test: Null pointer handling
 */
static void test_null_pointer_handling(void)
{
	OSAL_printf("[ TEST     ] Null pointer handling\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02};
	int ret;

	/* PRL_Encode with NULL buffer */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 NULL, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret < 0);

	/* PRL_Decode with NULL outputs */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	ret = PRL_Decode(buffer, ret, NULL, NULL, NULL, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	/* PRL_GetDeviceType with NULL output */
	ret = PRL_GetDeviceType(buffer, ret, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	/* PRL_GetVersion with NULL outputs should be safe */
	PRL_GetVersion(NULL, NULL); /* Should not crash */

	OSAL_printf("[ PASS     ] Null pointer handling test passed\n");
}

/**
 * Test: Protocol version mismatch handling
 */
static void test_protocol_version_mismatch(void)
{
	OSAL_printf("[ TEST     ] Protocol version mismatch\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02};
	int ret;

	/* Create packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	/* Modify version to simulate old protocol version */
	prl_header_t *hdr = (prl_header_t *)buffer;
	uint8_t original_version = hdr->version;
	hdr->version = 0xFF; /* Invalid/future version */

	/* Recalculate CRC for modified header */
	prl_set_packet_crc(buffer, ret);

	/* Try to validate/decode */
	int validate_ret = PRL_ValidatePacket(buffer, ret);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, validate_ret);
	OSAL_printf("[ INFO     ] Version mismatch detected: %s\n",
	           PRL_GetErrorString(validate_ret));

	/* Restore version */
	hdr->version = original_version;
	prl_set_packet_crc(buffer, ret);

	validate_ret = PRL_ValidatePacket(buffer, ret);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, validate_ret);

	OSAL_printf("[ PASS     ] Protocol version mismatch test passed\n");
}

/**
 * Test: Recovery after decode errors
 */
static void test_recovery_after_decode_errors(void)
{
	OSAL_printf("[ TEST     ] Recovery after decode errors\n");

	uint8_t buffer[256];
	uint8_t corrupted[256];
	uint8_t payload[] = {0x01, 0x02, 0x03};
	int ret;

	/* Create valid packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int packet_len = ret;

	/* Create corrupted packet */
	OSAL_memcpy(corrupted, buffer, packet_len);
	corrupted[0] = 0xFF;

	/* Try to decode corrupted packet */
	uint8_t dev_type, msg_type;
	const uint8_t *decoded_payload;
	uint16_t payload_len;

	ret = PRL_Decode(corrupted, packet_len,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
	OSAL_printf("[ INFO     ] Error detected: %s\n", PRL_GetErrorString(ret));

	/* Verify system can still decode valid packets */
	ret = PRL_Decode(buffer, packet_len,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
	TEST_ASSERT_EQUAL(PRL_MCU_MSG_COMMAND, msg_type);

	/* Verify we can still encode */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0, buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	OSAL_printf("[ PASS     ] Recovery after decode errors test passed\n");
}

/**
 * Test: Multiple sequential errors
 */
static void test_multiple_sequential_errors(void)
{
	OSAL_printf("[ TEST     ] Multiple sequential errors\n");

	uint8_t buffer[256];
	int i;
	int error_count = 0;

	/* Generate multiple error conditions */
	for (i = 0; i < 10; i++) {
		/* Invalid device type */
		int ret = PRL_Encode(0xFF, 0x01, NULL, 0,
		                     buffer, sizeof(buffer), 0);
		if (ret < 0) {
			error_count++;
		}

		/* Buffer too small */
		uint8_t small[16];
		uint8_t large_payload[256];
		OSAL_memset(large_payload, i, sizeof(large_payload));

		ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
		                 large_payload, sizeof(large_payload),
		                 small, sizeof(small), 0);
		if (ret < 0) {
			error_count++;
		}
	}

	TEST_ASSERT_EQUAL(20, error_count); /* All should fail */

	/* Verify system still works after errors */
	uint8_t payload[] = {0xAA};
	int ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                     payload, sizeof(payload),
	                     buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	OSAL_printf("[ INFO     ] Handled %d sequential errors successfully\n",
	           error_count);
	OSAL_printf("[ PASS     ] Multiple sequential errors test passed\n");
}

/**
 * Test: System stability under continuous errors
 */
static void test_stability_under_errors(void)
{
	OSAL_printf("[ TEST     ] System stability under continuous errors\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02};
	int valid_count = 0;
	int error_count = 0;
	int i;

	/* Mix valid and invalid operations */
	for (i = 0; i < 100; i++) {
		if (i % 3 == 0) {
			/* Valid operation */
			int ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
			                     payload, sizeof(payload),
			                     buffer, sizeof(buffer), 0);
			if (ret > 0) {
				valid_count++;
			}
		} else {
			/* Invalid operation */
			int ret = PRL_Encode(0xFF, 0x01, NULL, 0,
			                     buffer, sizeof(buffer), 0);
			if (ret < 0) {
				error_count++;
			}
		}
	}

	/* Verify expected counts */
	TEST_ASSERT_TRUE(valid_count > 30);
	TEST_ASSERT_TRUE(error_count > 60);

	OSAL_printf("[ INFO     ] Valid ops: %d, Errors: %d\n",
	           valid_count, error_count);
	OSAL_printf("[ PASS     ] Stability under errors test passed\n");
}

/**
 * Test: Error string coverage
 */
static void test_error_string_coverage(void)
{
	OSAL_printf("[ TEST     ] Error string coverage\n");

	/* Test all common error codes have strings */
	const int error_codes[] = {
		OSAL_SUCCESS,
		OSAL_ERR_INVALID_PARAM,
		OSAL_EINVAL,
		OSAL_ENOBUFS,
		OSAL_EPROTO,
		OSAL_EBADMSG,
		-999 /* Unknown error */
	};

	int i;
	for (i = 0; i < (int)(sizeof(error_codes) / sizeof(error_codes[0])); i++) {
		const char *str = PRL_GetErrorString(error_codes[i]);
		TEST_ASSERT_NOT_NULL(str);
		OSAL_printf("[ INFO     ] Error %d: %s\n", error_codes[i], str);
	}

	OSAL_printf("[ PASS     ] Error string coverage test passed\n");
}

/* Test case array */
static const test_case_t test_cases[] = {
	{
		.name = "test_corrupted_packet_detection",
		.func = test_corrupted_packet_detection,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_invalid_device_type",
		.func = test_invalid_device_type,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_buffer_overflow_protection",
		.func = test_buffer_overflow_protection,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_partial_packet_handling",
		.func = test_partial_packet_handling,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_null_pointer_handling",
		.func = test_null_pointer_handling,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_protocol_version_mismatch",
		.func = test_protocol_version_mismatch,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_recovery_after_decode_errors",
		.func = test_recovery_after_decode_errors,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_multiple_sequential_errors",
		.func = test_multiple_sequential_errors,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_stability_under_errors",
		.func = test_stability_under_errors,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_error_string_coverage",
		.func = test_error_string_coverage,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_prl_error_recovery",
	.module_name = "prl",
	.layer_name = "PRL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_INTEGRATION | TEST_TAG_ROBUSTNESS,
		.timeout_ms = 5000,
		.description = "PRL error handling and recovery"
	}
};

/* Test suite registration */
__attribute__((constructor))
static void register_system_prl_error_recovery_tests(void)
{
	libutest_register_suite(&test_suite);
}
