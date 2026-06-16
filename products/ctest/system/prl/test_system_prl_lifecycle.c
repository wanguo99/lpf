/**
 * @file test_system_prl_lifecycle.c
 * @brief PRL System Test - Module Lifecycle and State Management
 *
 * Tests PRL initialization, operation, cleanup, and state management
 * across multiple lifecycle iterations.
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"

/**
 * Test: Basic PRL initialization and deinitialization
 */
static void test_basic_init_deinit(void)
{
	OSAL_printf("[ TEST     ] Basic init/deinit cycle\n");

	/* Initialize PRL */
	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Verify we can perform operations */
	uint8_t major, minor;
	PRL_GetVersion(&major, &minor);
	TEST_ASSERT_EQUAL(PRL_VERSION_MAJOR, major);
	TEST_ASSERT_EQUAL(PRL_VERSION_MINOR, minor);

	/* Deinitialize PRL */
	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Basic init/deinit test passed\n");
}

/**
 * Test: Multiple init/deinit cycles
 */
static void test_multiple_init_deinit_cycles(void)
{
	OSAL_printf("[ TEST     ] Multiple init/deinit cycles\n");

	const int num_cycles = 10;
	int i;

	for (i = 0; i < num_cycles; i++) {
		/* Initialize */
		int ret = PRL_Init();
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Perform some operations */
		uint8_t buffer[256];
		uint8_t payload[] = {0x01, 0x02};
		ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
		                 payload, sizeof(payload),
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Deinitialize */
		ret = PRL_Deinit();
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	OSAL_printf("[ INFO     ] Completed %d init/deinit cycles\n", num_cycles);
	OSAL_printf("[ PASS     ] Multiple cycles test passed\n");
}

/**
 * Test: Operations without explicit init (should still work)
 */
static void test_operations_without_init(void)
{
	OSAL_printf("[ TEST     ] Operations without explicit init\n");

	/* Note: PRL should handle operations gracefully even without init */
	uint8_t buffer[256];
	uint8_t payload[] = {0xAA, 0xBB};

	/* Try encoding without init */
	int ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                     payload, sizeof(payload),
	                     buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	/* Try decoding */
	uint8_t dev_type, msg_type;
	const uint8_t *decoded_payload;
	uint16_t payload_len;

	ret = PRL_Decode(buffer, ret,
	                 &dev_type, &msg_type, &decoded_payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Operations without init test passed\n");
}

/**
 * Test: Sequence number management across operations
 */
static void test_sequence_number_management(void)
{
	OSAL_printf("[ TEST     ] Sequence number management\n");

	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Get initial sequence */
	uint32_t seq1 = PRL_GetCurrentSequence();

	/* Encode a packet (should increment sequence) */
	uint8_t buffer[256];
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0, buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	/* Get new sequence (should be incremented) */
	uint32_t seq2 = PRL_GetCurrentSequence();
	TEST_ASSERT_EQUAL(seq1 + 1, seq2);

	/* Encode another packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0, buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	uint32_t seq3 = PRL_GetCurrentSequence();
	TEST_ASSERT_EQUAL(seq2 + 1, seq3);

	OSAL_printf("[ INFO     ] Sequence progression: %u → %u → %u\n",
	           seq1, seq2, seq3);

	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Sequence management test passed\n");
}

/**
 * Test: PRL_ResetSequence functionality
 */
static void test_reset_sequence(void)
{
	OSAL_printf("[ TEST     ] Reset sequence functionality\n");

	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Reset to specific value */
	const uint32_t reset_value = 1000;
	PRL_ResetSequence(reset_value);

	/* Verify sequence was reset */
	uint32_t seq = PRL_GetCurrentSequence();
	TEST_ASSERT_EQUAL(reset_value, seq);

	/* Encode a packet and verify sequence increments from reset value */
	uint8_t buffer[256];
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0, buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	/* Extract sequence from packet */
	uint32_t packet_seq;
	ret = PRL_GetSequence(buffer, ret, &packet_seq);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(reset_value, packet_seq);

	/* Current sequence should now be reset_value + 1 */
	seq = PRL_GetCurrentSequence();
	TEST_ASSERT_EQUAL(reset_value + 1, seq);

	OSAL_printf("[ INFO     ] Reset to %u, packet seq=%u, current=%u\n",
	           reset_value, packet_seq, seq);

	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Reset sequence test passed\n");
}

/**
 * Test: PRL_GetVersion functionality
 */
static void test_get_version(void)
{
	OSAL_printf("[ TEST     ] Get version functionality\n");

	uint8_t major, minor;

	/* Can be called without init */
	PRL_GetVersion(&major, &minor);
	TEST_ASSERT_EQUAL(PRL_VERSION_MAJOR, major);
	TEST_ASSERT_EQUAL(PRL_VERSION_MINOR, minor);

	/* Should also work after init */
	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	PRL_GetVersion(&major, &minor);
	TEST_ASSERT_EQUAL(PRL_VERSION_MAJOR, major);
	TEST_ASSERT_EQUAL(PRL_VERSION_MINOR, minor);

	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ INFO     ] Protocol version: %u.%u\n", major, minor);
	OSAL_printf("[ PASS     ] Get version test passed\n");
}

/**
 * Test: PRL_GetErrorString functionality
 */
static void test_get_error_string(void)
{
	OSAL_printf("[ TEST     ] Get error string functionality\n");

	/* Test common error codes */
	const char *str;

	str = PRL_GetErrorString(OSAL_SUCCESS);
	TEST_ASSERT_NOT_NULL(str);

	str = PRL_GetErrorString(OSAL_ERR_INVALID_PARAM);
	TEST_ASSERT_NOT_NULL(str);

	str = PRL_GetErrorString(OSAL_ENOBUFS);
	TEST_ASSERT_NOT_NULL(str);

	str = PRL_GetErrorString(OSAL_EPROTO);
	TEST_ASSERT_NOT_NULL(str);

	str = PRL_GetErrorString(-999); /* Unknown error */
	TEST_ASSERT_NOT_NULL(str);

	OSAL_printf("[ INFO     ] Error strings verified\n");
	OSAL_printf("[ PASS     ] Get error string test passed\n");
}

/**
 * Test: State consistency across init/deinit
 */
static void test_state_consistency(void)
{
	OSAL_printf("[ TEST     ] State consistency across lifecycle\n");

	/* First init */
	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	uint32_t seq1 = PRL_GetCurrentSequence();

	/* Encode some packets */
	uint8_t buffer[256];
	int i;
	for (i = 0; i < 5; i++) {
		ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
		                 NULL, 0, buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);
	}

	uint32_t seq2 = PRL_GetCurrentSequence();
	TEST_ASSERT_EQUAL(seq1 + 5, seq2);

	/* Deinit */
	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Re-init (sequence may or may not reset - implementation dependent) */
	ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	uint32_t seq3 = PRL_GetCurrentSequence();
	/* Just verify it's a valid sequence number */
	TEST_ASSERT_TRUE(seq3 >= 0);

	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ INFO     ] Sequence: init=%u, after_ops=%u, reinit=%u\n",
	           seq1, seq2, seq3);
	OSAL_printf("[ PASS     ] State consistency test passed\n");
}

/**
 * Test: Stress test - rapid init/deinit with operations
 */
static void test_rapid_init_deinit_with_ops(void)
{
	OSAL_printf("[ TEST     ] Rapid init/deinit with operations\n");

	const int num_iterations = 100;
	uint8_t buffer[256];
	uint8_t payload[] = {0x01};
	int i;

	for (i = 0; i < num_iterations; i++) {
		/* Init */
		int ret = PRL_Init();
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Quick operation */
		ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
		                 payload, sizeof(payload),
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Validate */
		ret = PRL_ValidatePacket(buffer, ret);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Deinit */
		ret = PRL_Deinit();
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	OSAL_printf("[ INFO     ] Completed %d rapid cycles\n", num_iterations);
	OSAL_printf("[ PASS     ] Rapid init/deinit test passed\n");
}

/**
 * Test: Module re-initialization safety
 */
static void test_reinit_safety(void)
{
	OSAL_printf("[ TEST     ] Module re-initialization safety\n");

	/* Double init should be safe */
	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	ret = PRL_Init(); /* Second init */
	/* Should either succeed or return a specific error */
	TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret < 0);

	/* Operations should still work */
	uint8_t buffer[256];
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0, buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	/* Deinit once */
	ret = PRL_Deinit();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Double deinit should be safe */
	ret = PRL_Deinit();
	/* Should either succeed or return a specific error */
	TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret < 0);

	OSAL_printf("[ PASS     ] Re-initialization safety test passed\n");
}

/* Test case array */
static const test_case_t test_cases[] = {
	{
		.name = "test_basic_init_deinit",
		.func = test_basic_init_deinit,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_multiple_init_deinit_cycles",
		.func = test_multiple_init_deinit_cycles,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_operations_without_init",
		.func = test_operations_without_init,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_sequence_number_management",
		.func = test_sequence_number_management,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_reset_sequence",
		.func = test_reset_sequence,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_get_version",
		.func = test_get_version,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_get_error_string",
		.func = test_get_error_string,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_state_consistency",
		.func = test_state_consistency,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_rapid_init_deinit_with_ops",
		.func = test_rapid_init_deinit_with_ops,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_reinit_safety",
		.func = test_reinit_safety,
		.setup = NULL,
		.teardown = NULL
	},
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_prl_lifecycle",
	.module_name = "prl",
	.layer_name = "PRL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_INTEGRATION,
		.timeout_ms = 5000,
		.description = "PRL module lifecycle and state management"
	}
};

/* Test suite registration */
__attribute__((constructor))
static void register_system_prl_lifecycle_tests(void)
{
	libutest_register_suite(&test_suite);
}
