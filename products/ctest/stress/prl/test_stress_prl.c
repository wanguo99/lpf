/**
 * @file test_stress_prl.c
 * @brief PRL Protocol Layer stress tests
 *
 * Tests PRL under extreme conditions:
 * - High-frequency encoding/decoding
 * - Concurrent access from multiple threads
 * - Sequence number rollover
 * - Maximum payload sizes
 * - Long-running stability
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_stress.h>
#include "prl.h"

/* Test constants */
#define TEST_PAYLOAD_SIZE_SMALL     64
#define TEST_PAYLOAD_SIZE_MEDIUM    1024
#define TEST_PAYLOAD_SIZE_LARGE     (PRL_MAX_PAYLOAD_SIZE)
#define TEST_THREAD_COUNT           16
#define TEST_DURATION_SEC           10
#define TEST_ITERATIONS_HIGH        100000

/* Shared data for concurrency tests */
typedef struct {
    osal_atomic_uint32_t encode_count;
    osal_atomic_uint32_t decode_count;
    osal_atomic_uint32_t error_count;
    uint32_t expected_iterations;
} prl_stress_data_t;

/* ========== Worker Functions ========== */

/**
 * High-frequency encode worker
 * Rapidly encodes packets with varying payload sizes
 */
static int32_t encode_stress_worker(void *user_data, uint32_t iteration)
{
    prl_stress_data_t *data = (prl_stress_data_t *)user_data;
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t payload[TEST_PAYLOAD_SIZE_MEDIUM];
    int ret;

    /* Generate test payload */
    OSAL_memset(payload, (uint8_t)(iteration & 0xFF), sizeof(payload));

    /* Encode packet */
    ret = PRL_Encode(PRL_DEV_TYPE_MCU,
                     0x01,
                     payload,
                     sizeof(payload),
                     buffer,
                     sizeof(buffer),
                     PRL_FLAG_NONE);

    if (ret > 0) {
        OSAL_atomic_inc(&data->encode_count);
        return 0;
    } else {
        OSAL_atomic_inc(&data->error_count);
        return ret;
    }
}

/**
 * Encode-decode round-trip worker
 * Tests full encode-decode cycle under stress
 */
static int32_t roundtrip_stress_worker(void *user_data, uint32_t iteration)
{
    prl_stress_data_t *data = (prl_stress_data_t *)user_data;
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t payload[TEST_PAYLOAD_SIZE_SMALL];
    uint8_t dev_type, msg_type;
    const uint8_t *decoded_payload;
    uint16_t decoded_len;
    int ret;

    /* Generate unique test payload */
    for (size_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)((iteration + i) & 0xFF);
    }

    /* Encode */
    ret = PRL_Encode(PRL_DEV_TYPE_CCM,
                     0x10,
                     payload,
                     sizeof(payload),
                     buffer,
                     sizeof(buffer),
                     PRL_FLAG_NEED_ACK);

    if (ret <= 0) {
        OSAL_atomic_inc(&data->error_count);
        return ret;
    }

    OSAL_atomic_inc(&data->encode_count);

    /* Decode */
    ret = PRL_Decode(buffer, ret, &dev_type, &msg_type,
                     &decoded_payload, &decoded_len);

    if (ret != OSAL_SUCCESS) {
        OSAL_atomic_inc(&data->error_count);
        return ret;
    }

    /* Verify */
    if (dev_type != PRL_DEV_TYPE_CCM ||
        msg_type != 0x10 ||
        decoded_len != sizeof(payload) ||
        OSAL_memcmp(decoded_payload, payload, decoded_len) != 0) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    OSAL_atomic_inc(&data->decode_count);
    return 0;
}

/**
 * Large payload stress worker
 * Tests maximum payload size encoding
 */
static int32_t large_payload_worker(void *user_data, uint32_t iteration)
{
    prl_stress_data_t *data = (prl_stress_data_t *)user_data;
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t *payload = OSAL_malloc(TEST_PAYLOAD_SIZE_LARGE);
    int ret;

    if (!payload) {
        OSAL_atomic_inc(&data->error_count);
        return OSAL_ENOMEM;
    }

    /* Fill payload with pattern */
    OSAL_memset(payload, (uint8_t)(iteration & 0xFF), TEST_PAYLOAD_SIZE_LARGE);

    /* Encode large packet */
    ret = PRL_Encode(PRL_DEV_TYPE_PMC,
                     0x20,
                     payload,
                     TEST_PAYLOAD_SIZE_LARGE,
                     buffer,
                     sizeof(buffer),
                     PRL_FLAG_NONE);

    OSAL_free(payload);

    if (ret > 0) {
        OSAL_atomic_inc(&data->encode_count);
        return 0;
    } else {
        OSAL_atomic_inc(&data->error_count);
        return ret;
    }
}

/**
 * Fast extraction API stress worker
 * Tests PRL_GetDeviceType, PRL_GetMessageType, PRL_GetSequence
 */
static int32_t fast_extract_worker(void *user_data, uint32_t iteration)
{
    prl_stress_data_t *data = (prl_stress_data_t *)user_data;
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t payload[32];
    uint8_t dev_type, msg_type;
    uint32_t seq;
    int ret, encoded_len;

    /* Encode a packet */
    OSAL_memset(payload, 0xAA, sizeof(payload));
    encoded_len = PRL_Encode(PRL_DEV_TYPE_GSC,
                             0x30,
                             payload,
                             sizeof(payload),
                             buffer,
                             sizeof(buffer),
                             PRL_FLAG_NONE);

    if (encoded_len <= 0) {
        OSAL_atomic_inc(&data->error_count);
        return encoded_len;
    }

    /* Fast extraction without CRC verification */
    ret = PRL_GetDeviceType(buffer, encoded_len, &dev_type);
    if (ret != OSAL_SUCCESS || dev_type != PRL_DEV_TYPE_GSC) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    ret = PRL_GetMessageType(buffer, encoded_len, &msg_type);
    if (ret != OSAL_SUCCESS || msg_type != 0x30) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    ret = PRL_GetSequence(buffer, encoded_len, &seq);
    if (ret != OSAL_SUCCESS) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    OSAL_atomic_inc(&data->encode_count);
    return 0;
}

/**
 * Response building stress worker
 * Tests PRL_BuildResponse under load
 */
static int32_t response_build_worker(void *user_data, uint32_t iteration)
{
    prl_stress_data_t *data = (prl_stress_data_t *)user_data;
    uint8_t request_buf[PRL_MAX_PACKET_SIZE];
    uint8_t response_buf[PRL_MAX_PACKET_SIZE];
    uint8_t req_payload[64];
    uint8_t rsp_payload[64];
    int ret, req_len;

    /* Build request */
    OSAL_memset(req_payload, 0x11, sizeof(req_payload));
    req_len = PRL_Encode(PRL_DEV_TYPE_POWER,
                         0x40,
                         req_payload,
                         sizeof(req_payload),
                         request_buf,
                         sizeof(request_buf),
                         PRL_FLAG_NEED_ACK);

    if (req_len <= 0) {
        OSAL_atomic_inc(&data->error_count);
        return req_len;
    }

    /* Build response */
    OSAL_memset(rsp_payload, 0x22, sizeof(rsp_payload));
    ret = PRL_BuildResponse(request_buf, req_len,
                            rsp_payload, sizeof(rsp_payload),
                            response_buf, sizeof(response_buf));

    if (ret > 0) {
        OSAL_atomic_inc(&data->encode_count);
        return 0;
    } else {
        OSAL_atomic_inc(&data->error_count);
        return ret;
    }
}

/* ========== Test Cases ========== */

/**
 * Test: High-frequency encoding
 * Verifies PRL can handle rapid encode operations
 */
static void test_stress_encode_high_frequency(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = TEST_THREAD_COUNT;
    const uint32_t duration_sec = TEST_DURATION_SEC;

    /* Initialize */
    PRL_Init();
    PRL_ResetSequence(0);
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("PRL Encode High-Frequency", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running PRL encode stress: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, encode_stress_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    uint32_t total_encodes = OSAL_atomic_load(&data.encode_count);
    OSAL_printf("[ INFO     ] Total successful encodes: %u\n", total_encodes);
    OSAL_printf("[ INFO     ] Throughput: %.2f ops/sec\n",
               (double)total_encodes / duration_sec);

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/**
 * Test: Encode-decode round-trip concurrency
 * Verifies full encode-decode cycle under concurrent load
 */
static void test_stress_roundtrip_concurrency(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = 8;
    const uint32_t iterations = 10000;

    /* Initialize */
    PRL_Init();
    PRL_ResetSequence(0);
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = {
        .type = STRESS_TYPE_CONCURRENCY,
        .thread_count = thread_count,
        .duration_sec = 0,
        .iterations = iterations,
        .ramp_up_sec = 0,
        .stop_on_error = false
    };
    stress_context_t *ctx = stress_context_create("PRL Round-Trip", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running PRL round-trip test: %u threads, %u iterations\n",
               thread_count, iterations);
    TEST_ASSERT_EQUAL(stress_run(ctx, roundtrip_stress_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    uint32_t encode_count = OSAL_atomic_load(&data.encode_count);
    uint32_t decode_count = OSAL_atomic_load(&data.decode_count);
    uint32_t error_count = OSAL_atomic_load(&data.error_count);

    OSAL_printf("[ INFO     ] Encodes: %u, Decodes: %u, Errors: %u\n",
               encode_count, decode_count, error_count);
    TEST_ASSERT_EQUAL(encode_count, thread_count * iterations);
    TEST_ASSERT_EQUAL(decode_count, thread_count * iterations);
    TEST_ASSERT_EQUAL(error_count, 0);

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/**
 * Test: Large payload sustained load
 * Verifies PRL handles maximum payload sizes under continuous load
 */
static void test_stress_large_payload(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = 4;
    const uint32_t iterations = 1000;

    /* Initialize */
    PRL_Init();
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = {
        .type = STRESS_TYPE_CONCURRENCY,
        .thread_count = thread_count,
        .duration_sec = 0,
        .iterations = iterations,
        .ramp_up_sec = 0,
        .stop_on_error = false
    };
    stress_context_t *ctx = stress_context_create("PRL Large Payload", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running large payload test: %u threads, %u iterations, %u bytes/packet\n",
               thread_count, iterations, TEST_PAYLOAD_SIZE_LARGE);
    TEST_ASSERT_EQUAL(stress_run(ctx, large_payload_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    uint32_t encode_count = OSAL_atomic_load(&data.encode_count);
    uint32_t total_bytes = encode_count * (PRL_HEADER_SIZE + TEST_PAYLOAD_SIZE_LARGE);

    OSAL_printf("[ INFO     ] Total packets: %u\n", encode_count);
    OSAL_printf("[ INFO     ] Total data processed: %.2f MB\n",
               (double)total_bytes / (1024 * 1024));

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/**
 * Test: Fast extraction API performance
 * Verifies routing APIs (GetDeviceType, GetMessageType, GetSequence) under load
 */
static void test_stress_fast_extraction(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = TEST_THREAD_COUNT;
    const uint32_t duration_sec = 5;

    /* Initialize */
    PRL_Init();
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("PRL Fast Extraction", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running fast extraction test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, fast_extract_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    uint32_t ops = OSAL_atomic_load(&data.encode_count);
    OSAL_printf("[ INFO     ] Fast extractions: %u (3 ops per iteration)\n", ops * 3);
    OSAL_printf("[ INFO     ] Throughput: %.2f ops/sec\n",
               (double)(ops * 3) / duration_sec);

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/**
 * Test: Response building under load
 * Verifies PRL_BuildResponse API performance
 */
static void test_stress_response_building(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = 8;
    const uint32_t iterations = 10000;

    /* Initialize */
    PRL_Init();
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = {
        .type = STRESS_TYPE_CONCURRENCY,
        .thread_count = thread_count,
        .duration_sec = 0,
        .iterations = iterations,
        .ramp_up_sec = 0,
        .stop_on_error = false
    };
    stress_context_t *ctx = stress_context_create("PRL Response Building", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running response building test: %u threads, %u iterations\n",
               thread_count, iterations);
    TEST_ASSERT_EQUAL(stress_run(ctx, response_build_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    uint32_t responses = OSAL_atomic_load(&data.encode_count);

    OSAL_printf("[ INFO     ] Total responses built: %u\n", responses);
    TEST_ASSERT_EQUAL(responses, thread_count * iterations);

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/**
 * Test: Sequence number rollover
 * Verifies sequence number handling at UINT32_MAX boundary
 */
static void test_stress_sequence_rollover(void)
{
    uint8_t buffer[PRL_MAX_PACKET_SIZE];
    uint8_t payload[64];
    uint32_t seq;
    int ret;

    /* Initialize */
    PRL_Init();

    /* Set sequence to near maximum */
    const uint32_t near_max = UINT32_MAX - 100;
    PRL_ResetSequence(near_max);

    OSAL_printf("[ INFO     ] Testing sequence rollover from %u\n", near_max);

    /* Encode packets across rollover boundary */
    for (uint32_t i = 0; i < 200; i++) {
        OSAL_memset(payload, (uint8_t)i, sizeof(payload));

        ret = PRL_Encode(PRL_DEV_TYPE_MCU, 0x50,
                         payload, sizeof(payload),
                         buffer, sizeof(buffer),
                         PRL_FLAG_NONE);
        TEST_ASSERT_GREATER_THAN(0, ret);

        /* Extract and verify sequence */
        ret = PRL_GetSequence(buffer, ret, &seq);
        TEST_ASSERT_EQUAL(ret, OSAL_SUCCESS);

        uint32_t expected_seq = near_max + i;
        TEST_ASSERT_EQUAL(seq, expected_seq);

        /* Check rollover occurred */
        if (i == 100) {
            OSAL_printf("[ INFO     ] Sequence rolled over at iteration %u (seq=%u)\n",
                       i, seq);
        }
    }

    OSAL_printf("[ INFO     ] Sequence rollover test completed successfully\n");

    /* Cleanup */
    PRL_Deinit();
}

/**
 * Test: Long-running stability
 * Verifies PRL stability over extended operation
 */
static void test_stress_long_running(void)
{
    prl_stress_data_t data;
    const uint32_t thread_count = 4;
    const uint32_t duration_sec = 30;  /* 30 seconds sustained test */

    /* Initialize */
    PRL_Init();
    OSAL_atomic_init(&data.encode_count, 0);
    OSAL_atomic_init(&data.decode_count, 0);
    OSAL_atomic_init(&data.error_count, 0);

    /* Create stress test context */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("PRL Long-Running Stability", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* Run stress test */
    OSAL_printf("[ INFO     ] Running long-running stability test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    OSAL_printf("[ INFO     ] This will take a while...\n");
    TEST_ASSERT_EQUAL(stress_run(ctx, roundtrip_stress_worker, &data), 0);

    /* Print report */
    stress_print_report(ctx);

    /* Verify results */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    uint32_t encode_count = OSAL_atomic_load(&data.encode_count);
    uint32_t decode_count = OSAL_atomic_load(&data.decode_count);
    uint32_t error_count = OSAL_atomic_load(&data.error_count);

    OSAL_printf("[ INFO     ] Operations: Encode=%u, Decode=%u, Errors=%u\n",
               encode_count, decode_count, error_count);
    OSAL_printf("[ INFO     ] Average throughput: %.2f ops/sec\n",
               (double)(encode_count + decode_count) / duration_sec);

    /* Cleanup */
    stress_context_destroy(ctx);
    PRL_Deinit();
}

/* ========== Test Registration ========== */

/* Test case array */
static const test_case_t test_cases[] = {
    {
        .name = "test_stress_encode_high_frequency",
        .func = test_stress_encode_high_frequency,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_roundtrip_concurrency",
        .func = test_stress_roundtrip_concurrency,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_large_payload",
        .func = test_stress_large_payload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_fast_extraction",
        .func = test_stress_fast_extraction,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_response_building",
        .func = test_stress_response_building,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_sequence_rollover",
        .func = test_stress_sequence_rollover,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_stress_long_running",
        .func = test_stress_long_running,
        .setup = NULL,
        .teardown = NULL
    },
};

/* Test suite definition */
static const test_suite_t test_suite = {
    .suite_name = "stress_prl",
    .module_name = "stress_prl",
    .layer_name = "PRL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_STRESS,
        .tags = TEST_TAG_SLOW,
        .timeout_ms = 60000,  /* 60 seconds timeout */
        .description = "PRL stress tests"
    }
};

/* Test suite registration function */
__attribute__((constructor))
static void register_stress_prl_tests(void)
{
    libutest_register_suite(&test_suite);
}
