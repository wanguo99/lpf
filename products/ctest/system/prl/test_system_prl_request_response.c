/**
 * @file test_system_prl_request_response.c
 * @brief PRL System Test - Request-Response Communication Pattern
 *
 * Tests end-to-end request-response scenarios simulating realistic
 * device communication patterns.
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"

/* Test environment */
typedef struct {
	int prl_initialized;
	uint32_t sequence_base;
} test_env_t;

static test_env_t g_test_env = {0};

/**
 * Setup: Initialize PRL module
 */
static void setup_prl(void)
{
	OSAL_printf("[ SETUP    ] Initializing PRL module\n");

	int ret = PRL_Init();
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	g_test_env.prl_initialized = 1;
	g_test_env.sequence_base = PRL_GetCurrentSequence();

	OSAL_printf("[ SETUP OK ] PRL initialized, base sequence: %u\n",
	           g_test_env.sequence_base);
}

/**
 * Teardown: Cleanup PRL module
 */
static void teardown_prl(void)
{
	OSAL_printf("[ TEARDOWN ] Cleaning up PRL module\n");

	if (g_test_env.prl_initialized) {
		int ret = PRL_Deinit();
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		g_test_env.prl_initialized = 0;
	}

	OSAL_printf("[ TEARDOWN OK ] PRL cleaned up\n");
}

/**
 * Test: Basic request-response cycle (CCM → MCU)
 */
static void test_basic_request_response(void)
{
	OSAL_printf("[ TEST     ] Basic request-response cycle\n");

	uint8_t req_buffer[256];
	uint8_t rsp_buffer[256];
	uint8_t req_payload[] = {0x01, 0x02, 0x03};
	uint8_t rsp_payload[] = {0xAA, 0xBB, 0xCC, 0xDD};
	int ret;

	/* Step 1: CCM encodes request to MCU */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
	                 req_payload, sizeof(req_payload),
	                 req_buffer, sizeof(req_buffer),
	                 PRL_FLAG_NEED_ACK);
	TEST_ASSERT_TRUE(ret > 0);
	int req_len = ret;

	/* Step 2: Validate request packet */
	ret = PRL_ValidatePacket(req_buffer, req_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Step 3: MCU receives and decodes request */
	uint8_t dev_type, msg_type;
	const uint8_t *payload;
	uint16_t payload_len;

	ret = PRL_Decode(req_buffer, req_len,
	                 &dev_type, &msg_type, &payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
	TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
	TEST_ASSERT_EQUAL(sizeof(req_payload), payload_len);
	TEST_ASSERT_EQUAL(0, OSAL_memcmp(req_payload, payload, payload_len));

	/* Step 4: MCU builds response */
	ret = PRL_BuildResponse(req_buffer, req_len,
	                        rsp_payload, sizeof(rsp_payload),
	                        rsp_buffer, sizeof(rsp_buffer));
	TEST_ASSERT_TRUE(ret > 0);
	int rsp_len = ret;

	/* Step 5: CCM receives and decodes response */
	ret = PRL_Decode(rsp_buffer, rsp_len,
	                 &dev_type, &msg_type, &payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_MCU, dev_type);
	TEST_ASSERT_EQUAL(PRL_MCU_MSG_GET_VERSION, msg_type);
	TEST_ASSERT_EQUAL(sizeof(rsp_payload), payload_len);
	TEST_ASSERT_EQUAL(0, OSAL_memcmp(rsp_payload, payload, payload_len));

	/* Step 6: Verify response has IS_ACK flag */
	prl_header_t *rsp_hdr = (prl_header_t *)rsp_buffer;
	TEST_ASSERT_TRUE(rsp_hdr->flags & PRL_FLAG_IS_ACK);

	/* Step 7: Verify sequence number preserved */
	prl_header_t *req_hdr = (prl_header_t *)req_buffer;
	TEST_ASSERT_EQUAL(req_hdr->seq, rsp_hdr->seq);

	OSAL_printf("[ PASS     ] Basic request-response cycle passed\n");
}

/**
 * Test: Multiple sequential request-response cycles
 */
static void test_sequential_request_response(void)
{
	OSAL_printf("[ TEST     ] Sequential request-response cycles\n");

	const int num_cycles = 10;
	uint8_t req_buffer[256];
	uint8_t rsp_buffer[256];
	uint8_t payload[16];
	int i;

	for (i = 0; i < num_cycles; i++) {
		/* Prepare unique payload */
		OSAL_memset(payload, i, sizeof(payload));

		/* Encode request */
		int ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
		                     payload, sizeof(payload),
		                     req_buffer, sizeof(req_buffer),
		                     PRL_FLAG_NEED_ACK);
		TEST_ASSERT_TRUE(ret > 0);

		/* Validate request */
		ret = PRL_ValidatePacket(req_buffer, ret);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Build response */
		ret = PRL_BuildResponse(req_buffer, ret,
		                        payload, sizeof(payload),
		                        rsp_buffer, sizeof(rsp_buffer));
		TEST_ASSERT_TRUE(ret > 0);

		/* Validate response */
		ret = PRL_ValidatePacket(rsp_buffer, ret);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	}

	OSAL_printf("[ INFO     ] Completed %d request-response cycles\n", num_cycles);
	OSAL_printf("[ PASS     ] Sequential request-response test passed\n");
}

/**
 * Test: PMC telemetry request scenario
 */
static void test_pmc_telemetry_request(void)
{
	OSAL_printf("[ TEST     ] PMC telemetry request scenario\n");

	uint8_t req_buffer[256];
	uint8_t rsp_buffer[256];
	int ret;

	/* CCM requests telemetry from PMC */
	ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_GET_TELEMETRY,
	                 NULL, 0,
	                 req_buffer, sizeof(req_buffer),
	                 PRL_FLAG_NEED_ACK);
	TEST_ASSERT_TRUE(ret > 0);
	int req_len = ret;

	/* PMC receives request */
	uint8_t dev_type, msg_type;
	const uint8_t *payload;
	uint16_t payload_len;

	ret = PRL_Decode(req_buffer, req_len,
	                 &dev_type, &msg_type, &payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_PMC, dev_type);
	TEST_ASSERT_EQUAL(PRL_PMC_MSG_GET_TELEMETRY, msg_type);
	TEST_ASSERT_EQUAL(0, payload_len);

	/* PMC prepares telemetry data */
	typedef struct {
		uint32_t timestamp;
		uint16_t voltage;
		uint16_t current;
		int16_t temperature;
		uint16_t status;
	} __attribute__((packed)) pmc_telemetry_t;

	pmc_telemetry_t telemetry = {
		.timestamp = 1234567890,
		.voltage = 3300,
		.current = 500,
		.temperature = 25,
		.status = 0x01
	};

	/* PMC sends response with telemetry */
	ret = PRL_BuildResponse(req_buffer, req_len,
	                        &telemetry, sizeof(telemetry),
	                        rsp_buffer, sizeof(rsp_buffer));
	TEST_ASSERT_TRUE(ret > 0);

	/* CCM receives telemetry response */
	ret = PRL_Decode(rsp_buffer, ret,
	                 &dev_type, &msg_type, &payload, &payload_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(telemetry), payload_len);

	const pmc_telemetry_t *recv_telemetry = (const pmc_telemetry_t *)payload;
	TEST_ASSERT_EQUAL(telemetry.timestamp, recv_telemetry->timestamp);
	TEST_ASSERT_EQUAL(telemetry.voltage, recv_telemetry->voltage);
	TEST_ASSERT_EQUAL(telemetry.current, recv_telemetry->current);

	OSAL_printf("[ INFO     ] Telemetry: V=%u mV, I=%u mA, T=%d C\n",
	           recv_telemetry->voltage, recv_telemetry->current,
	           recv_telemetry->temperature);
	OSAL_printf("[ PASS     ] PMC telemetry request test passed\n");
}

/**
 * Test: GSC multi-device query scenario
 */
static void test_gsc_multidevice_query(void)
{
	OSAL_printf("[ TEST     ] GSC multi-device query scenario\n");

	uint8_t req_buffer[256];
	uint8_t rsp_buffer[256];
	int ret;

	/* GSC queries multiple device types */
	const uint8_t device_types[] = {
		PRL_DEV_TYPE_MCU,
		PRL_DEV_TYPE_CCM,
		PRL_DEV_TYPE_PMC
	};

	int i;
	for (i = 0; i < (int)(sizeof(device_types) / sizeof(device_types[0])); i++) {
		uint8_t dev_type = device_types[i];

		/* GSC sends status query */
		ret = PRL_Encode(dev_type, 0x01, /* Generic status query */
		                 NULL, 0,
		                 req_buffer, sizeof(req_buffer),
		                 PRL_FLAG_NEED_ACK);
		TEST_ASSERT_TRUE(ret > 0);
		int req_len = ret;

		/* Device responds with status */
		uint8_t status = 0xA0 | i; /* Unique status per device */
		ret = PRL_BuildResponse(req_buffer, req_len,
		                        &status, sizeof(status),
		                        rsp_buffer, sizeof(rsp_buffer));
		TEST_ASSERT_TRUE(ret > 0);

		/* GSC validates response */
		uint8_t recv_dev_type, msg_type;
		const uint8_t *payload;
		uint16_t payload_len;

		ret = PRL_Decode(rsp_buffer, ret,
		                 &recv_dev_type, &msg_type, &payload, &payload_len);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_EQUAL(dev_type, recv_dev_type);
		TEST_ASSERT_EQUAL(1, payload_len);
		TEST_ASSERT_EQUAL(status, *payload);

		OSAL_printf("[ INFO     ] Device %s status: 0x%02X\n",
		           PRL_GetDeviceTypeName(dev_type), status);
	}

	OSAL_printf("[ PASS     ] GSC multi-device query test passed\n");
}

/**
 * Test: Request without response (fire and forget)
 */
static void test_request_no_response(void)
{
	OSAL_printf("[ TEST     ] Request without response\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x11, 0x22, 0x33};
	int ret;

	/* Send command without NEED_ACK flag */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_COMMAND,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer),
	                 PRL_FLAG_NONE);
	TEST_ASSERT_TRUE(ret > 0);

	/* Validate packet structure */
	ret = PRL_ValidatePacket(buffer, ret);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Verify no ACK flag set */
	prl_header_t *hdr = (prl_header_t *)buffer;
	TEST_ASSERT_FALSE(hdr->flags & PRL_FLAG_NEED_ACK);
	TEST_ASSERT_FALSE(hdr->flags & PRL_FLAG_IS_ACK);

	OSAL_printf("[ PASS     ] Fire-and-forget request test passed\n");
}

/**
 * Test: Sequence number correlation in request-response
 */
static void test_sequence_correlation(void)
{
	OSAL_printf("[ TEST     ] Sequence number correlation\n");

	uint8_t req_buffer[256];
	uint8_t rsp_buffer[256];
	uint32_t req_seq, rsp_seq;
	int ret;

	/* Encode request */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 NULL, 0,
	                 req_buffer, sizeof(req_buffer),
	                 PRL_FLAG_NEED_ACK);
	TEST_ASSERT_TRUE(ret > 0);

	/* Extract request sequence number */
	ret = PRL_GetSequence(req_buffer, ret, &req_seq);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Build response */
	ret = PRL_BuildResponse(req_buffer, ret,
	                        NULL, 0,
	                        rsp_buffer, sizeof(rsp_buffer));
	TEST_ASSERT_TRUE(ret > 0);

	/* Extract response sequence number */
	ret = PRL_GetSequence(rsp_buffer, ret, &rsp_seq);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Verify sequence numbers match */
	TEST_ASSERT_EQUAL(req_seq, rsp_seq);

	OSAL_printf("[ INFO     ] Request seq=%u, Response seq=%u\n",
	           req_seq, rsp_seq);
	OSAL_printf("[ PASS     ] Sequence correlation test passed\n");
}

/* Test case array */
static const test_case_t test_cases[] = {
	{
		.name = "test_basic_request_response",
		.func = test_basic_request_response,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_sequential_request_response",
		.func = test_sequential_request_response,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_pmc_telemetry_request",
		.func = test_pmc_telemetry_request,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_gsc_multidevice_query",
		.func = test_gsc_multidevice_query,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_request_no_response",
		.func = test_request_no_response,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
	{
		.name = "test_sequence_correlation",
		.func = test_sequence_correlation,
		.setup = setup_prl,
		.teardown = teardown_prl
	},
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_prl_request_response",
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
		.description = "PRL request-response communication patterns"
	}
};

/* Test suite registration */
__attribute__((constructor))
static void register_system_prl_request_response_tests(void)
{
	libutest_register_suite(&test_suite);
}
