/**
 * @file test_template.c
 * @brief Test Template - Copy this file to start a new test
 *
 * INSTRUCTIONS:
 * 1. Copy this file to the appropriate test directory:
 *    - Unit tests: products/tests/unit/<layer>/test_<layer>_<module>.c
 *    - Performance: products/tests/performance/<layer>/test_<layer>_<module>.c
 *    - Stress: products/tests/stress/<layer>/test_<layer>_<module>.c
 *    - System: products/tests/system/<layer>/test_<layer>_<module>.c
 *
 * 2. Replace the following placeholders:
 *    - MODULE_NAME: Name of the module being tested (e.g., "mutex", "timer")
 *    - LAYER: Layer name (e.g., "OSAL", "HAL", "PDL", "PRL", "PCONFIG", "ACONFIG")
 *    - DESCRIPTION: Brief description of what this test suite tests
 *    - TEST_CATEGORY: Test category (TEST_CATEGORY_UNIT, TEST_CATEGORY_PERFORMANCE, etc.)
 *    - TEST_TAGS: Test tags (TEST_TAG_FAST, TEST_TAG_SLOW, TEST_TAG_HARDWARE, etc.)
 *    - TIMEOUT_MS: Expected maximum execution time in milliseconds
 *
 * 3. Add your test cases using TEST_CASE() macro
 *
 * 4. List all test cases in the test_case_t array
 *
 * 5. Add corresponding Kconfig option in products/tests/<category>/<layer>/Kconfig:
 *    config TEST_<LAYER>_<MODULE>
 *        bool "Test <layer> <module>"
 *        depends on CONFIG_<LAYER>
 *        default y
 *        help
 *          Test <layer> <module> functionality.
 *          Runtime: <100ms
 *          Hardware: None
 *
 * 6. Compile and run:
 *    python3 build.py config tests_x86_full_defconfig
 *    python3 build.py build
 *    ./_build/bin/ems-test -m <layer>_<module>
 */

#include "test_framework.h"
#include "<module_header>.h"  /* Replace with actual module header */

/* ============================================================================
 * Test Cases
 * ============================================================================ */

/**
 * Test case 1: Basic functionality
 *
 * This test verifies basic functionality of the module.
 */
TEST_CASE(test_MODULE_NAME_basic)
{
    /* Setup */
    /* ... initialize resources ... */

    /* Execute */
    /* ... call the function being tested ... */

    /* Verify */
    TEST_ASSERT_EQUAL(expected_value, actual_value);

    /* Cleanup */
    /* ... release resources ... */
}

/**
 * Test case 2: Error handling - null pointer
 *
 * This test verifies that the module handles null pointers correctly.
 */
TEST_CASE(test_MODULE_NAME_null_pointer)
{
    /* Call function with null pointer */
    int32_t ret = MODULE_Function(NULL);

    /* Verify error code */
    TEST_ASSERT_EQUAL(MODULE_ERR_INVALID_POINTER, ret);
}

/**
 * Test case 3: Error handling - invalid parameters
 *
 * This test verifies that the module handles invalid parameters correctly.
 */
TEST_CASE(test_MODULE_NAME_invalid_params)
{
    /* Call function with invalid parameters */
    int32_t ret = MODULE_Function(invalid_param);

    /* Verify error code */
    TEST_ASSERT_EQUAL(MODULE_ERR_INVALID_PARAM, ret);
}

/**
 * Test case 4: Edge case
 *
 * This test verifies behavior at boundary conditions.
 */
TEST_CASE(test_MODULE_NAME_edge_case)
{
    /* Test boundary conditions */
    /* ... */

    TEST_ASSERT_TRUE(condition);
}

/* ============================================================================
 * Optional: Test Fixture (Setup/Teardown)
 * ============================================================================
 *
 * Use fixtures when multiple test cases need the same initialization.
 * Uncomment and modify the following if needed:
 */

#if 0  /* Enable if using fixtures */

/* Global test context */
static void *g_test_resource;

/**
 * Setup function - called before each test case
 */
static void setup(void)
{
    /* Initialize resources */
    g_test_resource = MODULE_Create();
    /* ... */
}

/**
 * Teardown function - called after each test case
 */
static void teardown(void)
{
    /* Cleanup resources */
    if (g_test_resource) {
        MODULE_Destroy(g_test_resource);
        g_test_resource = NULL;
    }
    /* ... */
}

#endif  /* Enable if using fixtures */

/* ============================================================================
 * Test Suite Registration
 * ============================================================================ */

/**
 * Define test case array
 *
 * List all test cases using TEST_CASE_ENTRY() macro.
 * If using fixtures, use TEST_CASE_ENTRY_WITH_FIXTURE() instead.
 */
static const test_case_t LAYER_MODULE_NAME_cases[] = {
    /* Basic test cases */
    TEST_CASE_ENTRY(test_MODULE_NAME_basic),

    /* Error handling test cases */
    TEST_CASE_ENTRY(test_MODULE_NAME_null_pointer),
    TEST_CASE_ENTRY(test_MODULE_NAME_invalid_params),

    /* Edge case test cases */
    TEST_CASE_ENTRY(test_MODULE_NAME_edge_case),

    /* If using fixtures, use this format instead:
    TEST_CASE_ENTRY_WITH_FIXTURE(test_MODULE_NAME_basic, setup, teardown),
    TEST_CASE_ENTRY_WITH_FIXTURE(test_MODULE_NAME_null_pointer, setup, teardown),
    */
};

/**
 * Register test suite
 *
 * Parameters:
 * 1. suite_id: Identifier for this test suite (LAYER_MODULE_NAME, e.g., osal_mutex)
 * 2. layer_name: Layer name string ("OSAL", "HAL", "PDL", "PRL", "PCONFIG", "ACONFIG")
 * 3. cases_array: Test case array defined above
 * 4. category: Test category
 *    - TEST_CATEGORY_UNIT: Unit tests (fast, isolated)
 *    - TEST_CATEGORY_PERFORMANCE: Performance tests (timing, throughput)
 *    - TEST_CATEGORY_STRESS: Stress tests (load, concurrency)
 *    - TEST_CATEGORY_SYSTEM: System tests (integration, end-to-end)
 * 5. tags: Test tags (can be combined with | operator)
 *    - TEST_TAG_FAST: Fast test (<100ms)
 *    - TEST_TAG_SLOW: Slow test (>1s)
 *    - TEST_TAG_HARDWARE: Requires hardware
 *    - TEST_TAG_NETWORK: Requires network
 *    - TEST_TAG_FILESYSTEM: Requires filesystem
 *    - TEST_TAG_MEMORY_INTENSIVE: High memory usage
 *    - TEST_TAG_CPU_INTENSIVE: High CPU usage
 * 6. timeout_ms: Expected maximum execution time in milliseconds (0 = no limit)
 * 7. description: Human-readable description of what this suite tests
 *
 * Examples:
 *
 * Unit test (fast, no hardware):
 *   TEST_SUITE_REGISTER(osal_mutex, "OSAL", osal_mutex_cases,
 *                       TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100,
 *                       "OSAL mutex operations (create, lock, unlock, destroy)");
 *
 * Performance test (slow):
 *   TEST_SUITE_REGISTER(osal_mutex_perf, "OSAL", osal_mutex_perf_cases,
 *                       TEST_CATEGORY_PERFORMANCE, TEST_TAG_SLOW, 5000,
 *                       "OSAL mutex performance benchmarks");
 *
 * Hardware test (requires hardware):
 *   TEST_SUITE_REGISTER(hal_can, "HAL", hal_can_cases,
 *                       TEST_CATEGORY_UNIT, TEST_TAG_FAST | TEST_TAG_HARDWARE, 100,
 *                       "HAL CAN driver operations");
 *
 * Stress test (slow, CPU intensive):
 *   TEST_SUITE_REGISTER(osal_thread_stress, "OSAL", osal_thread_stress_cases,
 *                       TEST_CATEGORY_STRESS, TEST_TAG_SLOW | TEST_TAG_CPU_INTENSIVE, 10000,
 *                       "OSAL thread concurrency stress test");
 */
TEST_SUITE_REGISTER(LAYER_MODULE_NAME,        /* Replace: suite_id (e.g., osal_mutex) */
                    "LAYER",                   /* Replace: layer name (e.g., "OSAL") */
                    LAYER_MODULE_NAME_cases,   /* Test case array */
                    TEST_CATEGORY_UNIT,        /* Replace: test category */
                    TEST_TAG_FAST,             /* Replace: test tags */
                    100,                       /* Replace: timeout in ms */
                    "DESCRIPTION");            /* Replace: description */

/* ============================================================================
 * Tips and Best Practices
 * ============================================================================
 *
 * 1. Test Independence
 *    - Each test should be able to run independently
 *    - Don't rely on execution order
 *    - Create and cleanup resources within each test
 *
 * 2. Clear Naming
 *    - Test names should clearly describe what they test
 *    - Use format: test_<feature>_<scenario>
 *    - Examples: test_mutex_create_success, test_mutex_lock_timeout
 *
 * 3. Resource Cleanup
 *    - Always cleanup resources created in tests
 *    - Use fixtures for shared resource management
 *    - Cleanup even if test fails
 *
 * 4. Error Path Testing
 *    - Don't only test success cases
 *    - Test error handling (null pointers, invalid params, etc.)
 *    - Test boundary conditions
 *
 * 5. Assertion Granularity
 *    - Use multiple small assertions instead of one large assertion
 *    - Each assertion should verify one specific condition
 *    - Makes it easier to identify what failed
 *
 * 6. Documentation
 *    - Add comments explaining what each test verifies
 *    - Document any special setup or assumptions
 *    - Keep file header up to date
 *
 * 7. Metadata
 *    - Set appropriate category (UNIT, PERFORMANCE, STRESS, SYSTEM)
 *    - Tag tests correctly (FAST, SLOW, HARDWARE, etc.)
 *    - Set realistic timeout values
 *    - Write clear descriptions
 *
 * ============================================================================
 * Available Assertions
 * ============================================================================
 *
 * Equality:
 *   TEST_ASSERT_EQUAL(expected, actual)
 *   TEST_ASSERT_NOT_EQUAL(expected, actual)
 *
 * Boolean:
 *   TEST_ASSERT_TRUE(condition)
 *   TEST_ASSERT_FALSE(condition)
 *
 * Pointers:
 *   TEST_ASSERT_NULL(ptr)
 *   TEST_ASSERT_NOT_NULL(ptr)
 *
 * Strings:
 *   TEST_ASSERT_STR_EQUAL(expected, actual)
 *   TEST_ASSERT_STR_NOT_EQUAL(expected, actual)
 *
 * Memory:
 *   TEST_ASSERT_MEM_EQUAL(expected, actual, size)
 *
 * ============================================================================
 */
