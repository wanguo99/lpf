/**
 * @file test_example_metadata.c
 * @brief Example demonstrating test metadata usage
 *
 * This example shows how to use test metadata for categorization and filtering:
 * - Test categories (UNIT, PERFORMANCE, STRESS, SYSTEM)
 * - Test tags (FAST, SLOW, HARDWARE, NETWORK, etc.)
 * - Timeout settings
 * - Descriptive documentation
 *
 * Metadata enables:
 * - Runtime filtering (e.g., run only fast tests, exclude hardware tests)
 * - Better test organization and discovery
 * - CI/CD integration (e.g., run only unit tests in pull requests)
 * - Performance tracking and regression detection
 */

#include "test_framework.h"
#include "osal.h"

/* ============================================================================
 * Example 1: Unit Test with Fast Tag
 * ============================================================================
 *
 * Typical unit test characteristics:
 * - Category: TEST_CATEGORY_UNIT
 * - Tags: TEST_TAG_FAST (execution time < 100ms)
 * - Timeout: 100ms
 * - No hardware dependencies
 */

TEST_CASE(test_unit_fast_example)
{
    /* Simple, fast unit test */
    int32_t result = 2 + 2;
    TEST_ASSERT_EQUAL(4, result);
}

TEST_CASE(test_unit_string_operations)
{
    const char *str = "Hello";
    TEST_ASSERT_EQUAL(5, OSAL_Strlen(str));
    TEST_ASSERT_TRUE(OSAL_Strcmp(str, "Hello") == 0);
}

static const test_case_t unit_fast_cases[] = {
    TEST_CASE_ENTRY(test_unit_fast_example),
    TEST_CASE_ENTRY(test_unit_string_operations),
};

TEST_SUITE_REGISTER(metadata_unit_fast, "OSAL", unit_fast_cases,
                    TEST_CATEGORY_UNIT,       /* Unit test */
                    TEST_TAG_FAST,            /* Fast execution */
                    100,                      /* 100ms timeout */
                    "Fast unit test example demonstrating TEST_TAG_FAST");

/* ============================================================================
 * Example 2: Performance Test with Slow Tag
 * ============================================================================
 *
 * Performance test characteristics:
 * - Category: TEST_CATEGORY_PERFORMANCE
 * - Tags: TEST_TAG_SLOW (execution time > 1s)
 * - Timeout: 5000ms (5 seconds)
 * - Measures timing, throughput, or latency
 */

TEST_CASE(test_performance_timing)
{
    uint64_t start_time = OSAL_GetTimeMs();

    /* Simulate performance measurement */
    OSAL_Sleep(100);  /* Sleep for 100ms */

    uint64_t end_time = OSAL_GetTimeMs();
    uint64_t elapsed = end_time - start_time;

    /* Verify timing is within expected range (90-150ms) */
    TEST_ASSERT_TRUE(elapsed >= 90 && elapsed <= 150);
}

TEST_CASE(test_performance_throughput)
{
    /* Simulate throughput test */
    uint32_t operations = 0;
    uint64_t start_time = OSAL_GetTimeMs();

    while (OSAL_GetTimeMs() - start_time < 1000) {
        operations++;
    }

    /* Verify minimum throughput */
    TEST_ASSERT_TRUE(operations > 100000);
}

static const test_case_t performance_cases[] = {
    TEST_CASE_ENTRY(test_performance_timing),
    TEST_CASE_ENTRY(test_performance_throughput),
};

TEST_SUITE_REGISTER(metadata_performance, "OSAL", performance_cases,
                    TEST_CATEGORY_PERFORMANCE,  /* Performance test */
                    TEST_TAG_SLOW,              /* Slow execution (>1s) */
                    5000,                       /* 5 second timeout */
                    "Performance test example demonstrating TEST_TAG_SLOW and timing measurements");

/* ============================================================================
 * Example 3: Hardware-Dependent Test
 * ============================================================================
 *
 * Hardware test characteristics:
 * - Category: TEST_CATEGORY_UNIT
 * - Tags: TEST_TAG_FAST | TEST_TAG_HARDWARE
 * - Timeout: 100ms
 * - Requires real hardware (CAN bus, GPIO, etc.)
 */

TEST_CASE(test_hardware_can_init)
{
    /* This test would require real CAN hardware */
    /* For this example, we just demonstrate the metadata */
    TEST_ASSERT_TRUE(true);  /* Placeholder */
}

TEST_CASE(test_hardware_gpio_read)
{
    /* This test would require real GPIO pins */
    TEST_ASSERT_TRUE(true);  /* Placeholder */
}

static const test_case_t hardware_cases[] = {
    TEST_CASE_ENTRY(test_hardware_can_init),
    TEST_CASE_ENTRY(test_hardware_gpio_read),
};

TEST_SUITE_REGISTER(metadata_hardware, "HAL", hardware_cases,
                    TEST_CATEGORY_UNIT,
                    TEST_TAG_FAST | TEST_TAG_HARDWARE,  /* Combine tags with | */
                    100,
                    "Hardware test example demonstrating TEST_TAG_HARDWARE for tests requiring real devices");

/* ============================================================================
 * Example 4: Stress Test with Multiple Tags
 * ============================================================================
 *
 * Stress test characteristics:
 * - Category: TEST_CATEGORY_STRESS
 * - Tags: TEST_TAG_SLOW | TEST_TAG_CPU_INTENSIVE | TEST_TAG_MEMORY_INTENSIVE
 * - Timeout: 10000ms (10 seconds)
 * - Tests system under high load
 */

TEST_CASE(test_stress_thread_creation)
{
    /* Simulate stress test with many thread creations */
    const uint32_t num_threads = 100;

    for (uint32_t i = 0; i < num_threads; i++) {
        /* Would create/destroy threads in real test */
    }

    TEST_ASSERT_TRUE(true);  /* Placeholder */
}

TEST_CASE(test_stress_memory_allocation)
{
    /* Simulate stress test with many memory allocations */
    const uint32_t num_allocs = 1000;

    for (uint32_t i = 0; i < num_allocs; i++) {
        void *ptr = OSAL_Malloc(1024);
        if (ptr) {
            OSAL_Free(ptr);
        }
    }

    TEST_ASSERT_TRUE(true);
}

static const test_case_t stress_cases[] = {
    TEST_CASE_ENTRY(test_stress_thread_creation),
    TEST_CASE_ENTRY(test_stress_memory_allocation),
};

TEST_SUITE_REGISTER(metadata_stress, "OSAL", stress_cases,
                    TEST_CATEGORY_STRESS,
                    TEST_TAG_SLOW | TEST_TAG_CPU_INTENSIVE | TEST_TAG_MEMORY_INTENSIVE,  /* Multiple tags */
                    10000,  /* 10 second timeout */
                    "Stress test example with multiple tags (SLOW, CPU_INTENSIVE, MEMORY_INTENSIVE)");

/* ============================================================================
 * Example 5: System Integration Test
 * ============================================================================
 *
 * System test characteristics:
 * - Category: TEST_CATEGORY_SYSTEM
 * - Tags: TEST_TAG_SLOW
 * - Timeout: 5000ms
 * - Tests multiple modules/layers together
 */

TEST_CASE(test_system_integration)
{
    /* Simulate system integration test involving multiple layers */

    /* Step 1: Initialize OSAL resources */
    osal_mutex_t *mutex = NULL;
    int32_t ret = OSAL_MutexCreate(&mutex);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Step 2: Use HAL layer (simulated) */
    /* ... */

    /* Step 3: Test PDL layer (simulated) */
    /* ... */

    /* Step 4: Verify end-to-end behavior */
    TEST_ASSERT_NOT_NULL(mutex);

    /* Cleanup */
    OSAL_MutexDelete(mutex);
}

static const test_case_t system_cases[] = {
    TEST_CASE_ENTRY(test_system_integration),
};

TEST_SUITE_REGISTER(metadata_system, "SYSTEM", system_cases,
                    TEST_CATEGORY_SYSTEM,  /* System/integration test */
                    TEST_TAG_SLOW,
                    5000,
                    "System integration test example demonstrating TEST_CATEGORY_SYSTEM");

/* ============================================================================
 * Example 6: Network-Dependent Test
 * ============================================================================
 *
 * Network test characteristics:
 * - Category: TEST_CATEGORY_UNIT
 * - Tags: TEST_TAG_SLOW | TEST_TAG_NETWORK
 * - Timeout: 5000ms
 * - Requires network connectivity
 */

TEST_CASE(test_network_connectivity)
{
    /* This test would require network connectivity */
    TEST_ASSERT_TRUE(true);  /* Placeholder */
}

static const test_case_t network_cases[] = {
    TEST_CASE_ENTRY(test_network_connectivity),
};

TEST_SUITE_REGISTER(metadata_network, "OSAL", network_cases,
                    TEST_CATEGORY_UNIT,
                    TEST_TAG_SLOW | TEST_TAG_NETWORK,  /* Network dependency */
                    5000,
                    "Network test example demonstrating TEST_TAG_NETWORK for tests requiring connectivity");

/* ============================================================================
 * Metadata Usage Summary
 * ============================================================================
 *
 * Categories:
 *   TEST_CATEGORY_UNIT        - Isolated module testing
 *   TEST_CATEGORY_PERFORMANCE - Timing, throughput, latency measurements
 *   TEST_CATEGORY_STRESS      - Load, concurrency, resource limits
 *   TEST_CATEGORY_SYSTEM      - Integration, end-to-end scenarios
 *
 * Tags (can be combined with | operator):
 *   TEST_TAG_FAST             - Fast test (<100ms)
 *   TEST_TAG_SLOW             - Slow test (>1s)
 *   TEST_TAG_HARDWARE         - Requires real hardware
 *   TEST_TAG_NETWORK          - Requires network connectivity
 *   TEST_TAG_FILESYSTEM       - Requires filesystem access
 *   TEST_TAG_MEMORY_INTENSIVE - High memory usage
 *   TEST_TAG_CPU_INTENSIVE    - High CPU usage
 *   TEST_TAG_PRIVILEGED       - Requires elevated privileges
 *   TEST_TAG_UNSTABLE         - Known to be flaky/unstable
 *   TEST_TAG_DEPRECATED       - Deprecated, to be removed
 *
 * Filtering Examples (command line):
 *   ./_build/bin/ems-test --fast                 # Run only fast tests
 *   ./_build/bin/ems-test --exclude-hardware     # Skip hardware tests
 *   ./_build/bin/ems-test --category unit        # Run only unit tests
 *   ./_build/bin/ems-test --category performance # Run only performance tests
 *   ./_build/bin/ems-test --tags slow            # Run only slow tests
 *
 * CI/CD Integration:
 *   - Pull requests: Run only FAST tests (quick feedback)
 *   - Nightly builds: Run SLOW tests (comprehensive coverage)
 *   - Pre-release: Run ALL tests including HARDWARE
 *   - Performance tracking: Run PERFORMANCE tests and compare results
 *
 * Best Practices:
 *   1. Always set appropriate category
 *   2. Tag FAST tests (<100ms) and SLOW tests (>1s)
 *   3. Tag HARDWARE tests that require real devices
 *   4. Set realistic timeout values
 *   5. Write descriptive suite descriptions
 *   6. Use multiple tags when appropriate (e.g., SLOW | HARDWARE)
 *   7. Keep unit tests FAST to encourage frequent running
 *   8. Separate performance benchmarks from functional tests
 *
 * ============================================================================
 */
