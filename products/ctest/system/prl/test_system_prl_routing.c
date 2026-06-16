/**
 * @file test_system_prl_routing.c
 * @brief PRL System Test - Packet Routing and Fast Extraction
 *
 * Tests fast extraction APIs for packet routing and classification
 * without full decode overhead.
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"
#include "prl.h"
#include "prl_mcu.h"
#include "prl_ccm.h"
#include "prl_pmc.h"
#include "prl_gsc.h"

/* Routing table entry */
typedef struct {
	uint8_t dev_type;
	void (*handler)(const uint8_t *packet, size_t len);
	uint32_t packet_count;
} route_entry_t;

/* Mock routing table */
static route_entry_t g_route_table[4];
static int g_route_table_size = 0;

/* Mock handlers */
static void handle_mcu_packet(const uint8_t *packet, size_t len)
{
	(void)packet;
	(void)len;
	/* Mock handler */
}

static void handle_ccm_packet(const uint8_t *packet, size_t len)
{
	(void)packet;
	(void)len;
	/* Mock handler */
}

static void handle_pmc_packet(const uint8_t *packet, size_t len)
{
	(void)packet;
	(void)len;
	/* Mock handler */
}

static void handle_gsc_packet(const uint8_t *packet, size_t len)
{
	(void)packet;
	(void)len;
	/* Mock handler */
}

/**
 * Setup: Initialize routing table
 */
static void setup_routing(void)
{
	OSAL_printf("[ SETUP    ] Initializing routing table\n");

	PRL_Init();

	g_route_table[0].dev_type = PRL_DEV_TYPE_MCU;
	g_route_table[0].handler = handle_mcu_packet;
	g_route_table[0].packet_count = 0;

	g_route_table[1].dev_type = PRL_DEV_TYPE_CCM;
	g_route_table[1].handler = handle_ccm_packet;
	g_route_table[1].packet_count = 0;

	g_route_table[2].dev_type = PRL_DEV_TYPE_PMC;
	g_route_table[2].handler = handle_pmc_packet;
	g_route_table[2].packet_count = 0;

	g_route_table[3].dev_type = PRL_DEV_TYPE_GSC;
	g_route_table[3].handler = handle_gsc_packet;
	g_route_table[3].packet_count = 0;

	g_route_table_size = 4;

	OSAL_printf("[ SETUP OK ] Routing table initialized with %d entries\n",
	           g_route_table_size);
}

/**
 * Teardown: Cleanup routing table
 */
static void teardown_routing(void)
{
	OSAL_printf("[ TEARDOWN ] Cleaning up routing table\n");

	PRL_Deinit();
	OSAL_memset(g_route_table, 0, sizeof(g_route_table));
	g_route_table_size = 0;

	OSAL_printf("[ TEARDOWN OK ] Routing table cleaned up\n");
}

/**
 * Test: Fast device type extraction for routing
 */
static void test_fast_device_type_extraction(void)
{
	OSAL_printf("[ TEST     ] Fast device type extraction\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02};
	int ret;

	/* Create packets for different device types */
	const uint8_t device_types[] = {
		PRL_DEV_TYPE_MCU,
		PRL_DEV_TYPE_CCM,
		PRL_DEV_TYPE_PMC,
		PRL_DEV_TYPE_GSC
	};

	int i;
	for (i = 0; i < (int)(sizeof(device_types) / sizeof(device_types[0])); i++) {
		/* Encode packet */
		ret = PRL_Encode(device_types[i], 0x01,
		                 payload, sizeof(payload),
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Fast extract device type (no CRC verification) */
		uint8_t extracted_dev_type;
		ret = PRL_GetDeviceType(buffer, ret, &extracted_dev_type);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_EQUAL(device_types[i], extracted_dev_type);

		OSAL_printf("[ INFO     ] Extracted device type: %s (0x%02X)\n",
		           PRL_GetDeviceTypeName(extracted_dev_type), extracted_dev_type);
	}

	OSAL_printf("[ PASS     ] Fast device type extraction test passed\n");
}

/**
 * Test: Fast message type extraction for routing
 */
static void test_fast_message_type_extraction(void)
{
	OSAL_printf("[ TEST     ] Fast message type extraction\n");

	uint8_t buffer[256];
	int ret;

	/* Create packets with different message types */
	const uint8_t message_types[] = {
		PRL_MCU_MSG_GET_VERSION,
		PRL_MCU_MSG_HEARTBEAT,
		PRL_MCU_MSG_COMMAND,
		PRL_MCU_MSG_TELEMETRY
	};

	int i;
	for (i = 0; i < (int)(sizeof(message_types) / sizeof(message_types[0])); i++) {
		/* Encode packet */
		ret = PRL_Encode(PRL_DEV_TYPE_MCU, message_types[i],
		                 NULL, 0,
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Fast extract message type */
		uint8_t extracted_msg_type;
		ret = PRL_GetMessageType(buffer, ret, &extracted_msg_type);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
		TEST_ASSERT_EQUAL(message_types[i], extracted_msg_type);

		OSAL_printf("[ INFO     ] Extracted message type: 0x%02X\n",
		           extracted_msg_type);
	}

	OSAL_printf("[ PASS     ] Fast message type extraction test passed\n");
}

/**
 * Test: Sequence-based deduplication
 */
static void test_sequence_deduplication(void)
{
	OSAL_printf("[ TEST     ] Sequence-based deduplication\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0xAA, 0xBB};
	int ret;

	/* Create packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int packet_len = ret;

	/* Extract sequence number (first time) */
	uint32_t seq1;
	ret = PRL_GetSequence(buffer, packet_len, &seq1);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Simulate duplicate packet arrival */
	uint32_t seq2;
	ret = PRL_GetSequence(buffer, packet_len, &seq2);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Verify same sequence number extracted */
	TEST_ASSERT_EQUAL(seq1, seq2);

	/* Simulate new packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);

	uint32_t seq3;
	ret = PRL_GetSequence(buffer, ret, &seq3);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Verify different sequence number */
	TEST_ASSERT_NOT_EQUAL(seq1, seq3);

	OSAL_printf("[ INFO     ] Seq1=%u, Seq2=%u, Seq3=%u\n", seq1, seq2, seq3);
	OSAL_printf("[ PASS     ] Sequence deduplication test passed\n");
}

/**
 * Test: Routing table lookup simulation
 */
static void test_routing_table_lookup(void)
{
	OSAL_printf("[ TEST     ] Routing table lookup simulation\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x11, 0x22};
	int ret;

	/* Send packets to different devices */
	int i;
	for (i = 0; i < g_route_table_size; i++) {
		uint8_t dev_type = g_route_table[i].dev_type;

		/* Encode packet */
		ret = PRL_Encode(dev_type, 0x01,
		                 payload, sizeof(payload),
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Fast validate packet */
		ret = PRL_ValidatePacket(buffer, ret);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Extract device type for routing */
		uint8_t extracted_dev_type;
		ret = PRL_GetDeviceType(buffer, ret, &extracted_dev_type);
		TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

		/* Lookup in routing table */
		int j;
		int found = 0;
		for (j = 0; j < g_route_table_size; j++) {
			if (g_route_table[j].dev_type == extracted_dev_type) {
				g_route_table[j].packet_count++;
				g_route_table[j].handler(buffer, ret);
				found = 1;
				break;
			}
		}
		TEST_ASSERT_TRUE(found);
	}

	/* Verify all routes received packets */
	for (i = 0; i < g_route_table_size; i++) {
		TEST_ASSERT_EQUAL(1, g_route_table[i].packet_count);
		OSAL_printf("[ INFO     ] Route %s: %u packets\n",
		           PRL_GetDeviceTypeName(g_route_table[i].dev_type),
		           g_route_table[i].packet_count);
	}

	OSAL_printf("[ PASS     ] Routing table lookup test passed\n");
}

/**
 * Test: High-frequency packet classification
 */
static void test_high_frequency_classification(void)
{
	OSAL_printf("[ TEST     ] High-frequency packet classification\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01};
	int ret;

	const int num_packets = 1000;
	int classified_count = 0;
	int i;

	uint32_t start_time = OSAL_time_ms();

	for (i = 0; i < num_packets; i++) {
		/* Encode packet (round-robin device types) */
		uint8_t dev_type = (i % 4) + 1; /* MCU, CCM, PMC, GSC */
		ret = PRL_Encode(dev_type, 0x01,
		                 payload, sizeof(payload),
		                 buffer, sizeof(buffer), 0);
		TEST_ASSERT_TRUE(ret > 0);

		/* Fast classify by device type */
		uint8_t extracted_dev_type;
		ret = PRL_GetDeviceType(buffer, ret, &extracted_dev_type);
		if (ret == OSAL_SUCCESS) {
			classified_count++;
		}
	}

	uint32_t elapsed_ms = OSAL_time_ms() - start_time;

	TEST_ASSERT_EQUAL(num_packets, classified_count);

	OSAL_printf("[ INFO     ] Classified %d packets in %u ms\n",
	           classified_count, elapsed_ms);
	OSAL_printf("[ INFO     ] Throughput: %.2f packets/sec\n",
	           (double)classified_count / (elapsed_ms / 1000.0));
	OSAL_printf("[ PASS     ] High-frequency classification test passed\n");
}

/**
 * Test: Fast validation without full decode
 */
static void test_fast_validation(void)
{
	OSAL_printf("[ TEST     ] Fast validation without decode\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0xAA, 0xBB, 0xCC};
	int ret;

	/* Create valid packet */
	ret = PRL_Encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int packet_len = ret;

	/* Fast validate (only checks magic, version, CRC) */
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Corrupt magic number */
	uint8_t corrupted_buffer[256];
	OSAL_memcpy(corrupted_buffer, buffer, packet_len);
	corrupted_buffer[0] = 0xFF; /* Corrupt magic byte */

	ret = PRL_ValidatePacket(corrupted_buffer, packet_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	/* Corrupt CRC */
	OSAL_memcpy(corrupted_buffer, buffer, packet_len);
	prl_header_t *hdr = (prl_header_t *)corrupted_buffer;
	hdr->crc16 ^= 0xFFFF; /* Flip CRC bits */

	ret = PRL_ValidatePacket(corrupted_buffer, packet_len);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ PASS     ] Fast validation test passed\n");
}

/**
 * Test: Combined fast extraction for routing decision
 */
static void test_combined_fast_extraction(void)
{
	OSAL_printf("[ TEST     ] Combined fast extraction for routing\n");

	uint8_t buffer[256];
	uint8_t payload[] = {0x01, 0x02, 0x03};
	int ret;

	/* Create test packet */
	ret = PRL_Encode(PRL_DEV_TYPE_PMC, PRL_PMC_MSG_GET_TELEMETRY,
	                 payload, sizeof(payload),
	                 buffer, sizeof(buffer), 0);
	TEST_ASSERT_TRUE(ret > 0);
	int packet_len = ret;

	/* Step 1: Fast validate */
	ret = PRL_ValidatePacket(buffer, packet_len);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* Step 2: Extract device type */
	uint8_t dev_type;
	ret = PRL_GetDeviceType(buffer, packet_len, &dev_type);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_DEV_TYPE_PMC, dev_type);

	/* Step 3: Extract message type */
	uint8_t msg_type;
	ret = PRL_GetMessageType(buffer, packet_len, &msg_type);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_EQUAL(PRL_PMC_MSG_GET_TELEMETRY, msg_type);

	/* Step 4: Extract sequence for dedup check */
	uint32_t seq;
	ret = PRL_GetSequence(buffer, packet_len, &seq);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	OSAL_printf("[ INFO     ] Routing decision: dev=%s, msg=0x%02X, seq=%u\n",
	           PRL_GetDeviceTypeName(dev_type), msg_type, seq);
	OSAL_printf("[ PASS     ] Combined extraction test passed\n");
}

/* Test case array */
static const test_case_t test_cases[] = {
	{
		.name = "test_fast_device_type_extraction",
		.func = test_fast_device_type_extraction,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_fast_message_type_extraction",
		.func = test_fast_message_type_extraction,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_sequence_deduplication",
		.func = test_sequence_deduplication,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_routing_table_lookup",
		.func = test_routing_table_lookup,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_high_frequency_classification",
		.func = test_high_frequency_classification,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_fast_validation",
		.func = test_fast_validation,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
	{
		.name = "test_combined_fast_extraction",
		.func = test_combined_fast_extraction,
		.setup = setup_routing,
		.teardown = teardown_routing
	},
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_prl_routing",
	.module_name = "prl",
	.layer_name = "PRL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_INTEGRATION | TEST_TAG_PERFORMANCE,
		.timeout_ms = 5000,
		.description = "PRL packet routing and fast extraction"
	}
};

/* Test suite registration */
__attribute__((constructor))
static void register_system_prl_routing_tests(void)
{
	libutest_register_suite(&test_suite);
}
