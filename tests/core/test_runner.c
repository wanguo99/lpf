/**
 * @file test_runner.c
 * @brief Test execution engine
 */

#include "tests_core.h"
#include "test_assert.h"
#include "osal.h"

#define MAX_SUITES 128
#define LOG_BUF_SIZE 512

/* Test log file handle */
static int32_t g_test_log_fd = -1;

/* Global state */
bool g_test_failed = false;
const char *g_current_test = NULL;
static test_stats_t g_stats = {0};

/* External registry functions */
extern const test_suite_t** test_get_all_suites(uint32_t *count);
extern const test_suite_t* test_find_suite(const char *name);
extern uint32_t test_get_suites_by_layer(const char *layer_name, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_module(const char *module_name, const test_suite_t **suites, uint32_t max_suites);

/* Helper: Write to log file */
static inline void log_write(const char *msg)
{
    if (g_test_log_fd >= 0) {
        OSAL_write(g_test_log_fd, msg, OSAL_Strlen(msg));
    }
}

/* Helper: Write formatted string to log file */
static void log_printf(const char *fmt, ...)
{
    if (g_test_log_fd >= 0) {
        char buf[LOG_BUF_SIZE];
        OSAL_Snprintf(buf, sizeof(buf), fmt);
        OSAL_write(g_test_log_fd, buf, OSAL_Strlen(buf));
    }
}

/* Unified output macros */
#define TEST_LOG_RUN(name)       do { OSAL_Printf("[ RUN      ] %s\n", name); log_printf("[ RUN      ] %s\n", name); } while(0)
#define TEST_LOG_PASS(name)      log_printf("[ OK       ] %s\n", name)
#define TEST_LOG_FAIL(name)      do { OSAL_Printf("[ FAILED   ] %s\n", name); log_printf("[ FAILED   ] %s\n", name); } while(0)
#define TEST_LOG_SKIP(name)      log_printf("[ SKIPPED  ] %s\n", name)
#define TEST_LOG_SEPARATOR()     log_write("[----------]\n")
#define TEST_LOG_HEADER()        do { OSAL_Printf("[==========]\n"); log_write("[==========]\n"); } while(0)
#define TEST_LOG_DETAIL(...)     log_printf(__VA_ARGS__)

/**
 * Open test log file
 */
static void open_test_log(void)
{
    if (g_test_log_fd < 0) {
        g_test_log_fd = OSAL_open("/tmp/ems_test.log", OSAL_O_WRONLY | OSAL_O_CREAT | OSAL_O_TRUNC, 0644);
        if (g_test_log_fd >= 0) {
            OSAL_Printf("Test log: /tmp/ems_test.log\n\n");
        }
    }
}

/**
 * Close test log file
 */
static void close_test_log(void)
{
    if (g_test_log_fd >= 0) {
        OSAL_close(g_test_log_fd);
        g_test_log_fd = -1;
    }
}

/**
 * Helper: Append node to list
 */
static void append_to_list(test_result_node_t **head, test_result_node_t **tail, test_result_node_t *node)
{
    if (*head == NULL) {
        *head = *tail = node;
    } else {
        (*tail)->next = node;
        *tail = node;
    }
}

/**
 * Add test result to appropriate linked list
 */
static void add_test_result(const char *suite_name, const char *test_name, test_result_t result, uint32_t elapsed_ms)
{
    test_result_node_t *node = (test_result_node_t *)OSAL_Malloc(sizeof(test_result_node_t));
    if (NULL == node) return;

    node->suite_name = suite_name;
    node->test_name = test_name;
    node->elapsed_ms = elapsed_ms;
    node->next = NULL;

    switch (result) {
        case TEST_RESULT_PASS:
            append_to_list(&g_stats.passed_list_head, &g_stats.passed_list_tail, node);
            break;
        case TEST_RESULT_FAIL:
            append_to_list(&g_stats.failed_list_head, &g_stats.failed_list_tail, node);
            break;
        case TEST_RESULT_SKIP:
            append_to_list(&g_stats.skipped_list_head, &g_stats.skipped_list_tail, node);
            break;
    }
}

/**
 * Helper: Print test result list
 */
static void print_result_list(const char *header, uint32_t count, test_result_node_t *head, const char *tag)
{
    if (count == 0) return;

    OSAL_Printf("\n%s %u tests\n", header, count);

    const char *current_suite = NULL;
    for (test_result_node_t *node = head; node != NULL; node = node->next) {
        if (current_suite == NULL || OSAL_Strcmp(current_suite, node->suite_name) != 0) {
            if (current_suite != NULL) OSAL_Printf("\n");
            OSAL_Printf("  [%s]\n", node->suite_name);
            current_suite = node->suite_name;
        }
        OSAL_Printf("    [%s] %s (%u ms)\n", tag, node->test_name, node->elapsed_ms);
    }
}

/**
 * Print detailed test results summary
 */
static void print_test_results_summary(void)
{
    OSAL_Printf("\n");
    TEST_LOG_HEADER();
    OSAL_Printf(" Test Result Summary\n");
    TEST_LOG_HEADER();

    print_result_list("[  SKIPPED ]", g_stats.skipped, g_stats.skipped_list_head, "SKIP");
    print_result_list("[  FAILED  ]", g_stats.failed, g_stats.failed_list_head, "FAIL");

    OSAL_Printf("\n");
}

/**
 * Run a single test case
 */
static test_result_t run_test_case(const test_case_t *test, const char *suite_name)
{
    if (!test || !test->func) return TEST_RESULT_FAIL;

    g_test_failed = false;
    g_current_test = test->name;
    TEST_LOG_RUN(test->name);

    uint32_t start_time = OSAL_GetTickCount();

    /* Run setup */
    if (test->setup) {
        test->setup();
        if (g_test_failed) {
            TEST_LOG_DETAIL("[  FAILED  ] Setup failed for %s\n", test->name);
            add_test_result(suite_name, test->name, TEST_RESULT_FAIL, OSAL_GetTickCount() - start_time);
            return TEST_RESULT_FAIL;
        }
    }

    /* Run test */
    test->func();

    /* Run teardown */
    if (test->teardown) {
        bool failed_before = g_test_failed;
        g_test_failed = false;
        test->teardown();
        if (!g_test_failed) g_test_failed = failed_before;
    }

    uint32_t elapsed = OSAL_GetTickCount() - start_time;

    /* Determine result */
    test_result_t result;
    if (g_current_test == NULL) {
        result = TEST_RESULT_SKIP;
    } else if (g_test_failed) {
        TEST_LOG_FAIL(test->name);
        if (g_stats.failed_test_count < 64) {
            g_stats.failed_tests[g_stats.failed_test_count++] = test->name;
        }
        result = TEST_RESULT_FAIL;
    } else {
        TEST_LOG_PASS(test->name);
        result = TEST_RESULT_PASS;
    }

    TEST_LOG_DETAIL("             (elapsed: %u ms)\n\n", elapsed);
    add_test_result(suite_name, test->name, result, elapsed);

    return result;
}

/**
 * Run all tests in a suite
 */
static int32_t run_suite(const test_suite_t *suite)
{
    if (!suite) return OSAL_ERR_GENERIC;

    TEST_LOG_SEPARATOR();
    TEST_LOG_DETAIL(" Running %u tests from %s\n", suite->case_count, suite->suite_name);

    if (suite->suite_setup) suite->suite_setup();

    for (uint32_t i = 0; i < suite->case_count; i++) {
        test_result_t result = run_test_case(&suite->cases[i], suite->suite_name);
        g_stats.total++;

        if (result == TEST_RESULT_PASS) g_stats.passed++;
        else if (result == TEST_RESULT_FAIL) g_stats.failed++;
        else g_stats.skipped++;
    }

    if (suite->suite_teardown) suite->suite_teardown();

    TEST_LOG_SEPARATOR();
    TEST_LOG_DETAIL(" %u tests from %s\n\n", suite->case_count, suite->suite_name);

    return OSAL_SUCCESS;
}

/**
 * Run all registered tests
 */
int32_t libutest_run_all(void)
{
    uint32_t suite_count = 0;
    const test_suite_t **suites = test_get_all_suites(&suite_count);

    /* Open log file */
    open_test_log();

    TEST_LOG_HEADER();
    OSAL_Printf(" Running %u test suites\n", suite_count);

    libutest_reset_stats();

    uint32_t start_time = OSAL_GetTickCount();

    for (uint32_t i = 0; i < suite_count; i++) {
        run_suite(suites[i]);
    }

    uint32_t end_time = OSAL_GetTickCount();
    g_stats.total_time_ms = end_time - start_time;
    if (g_stats.total > 0) {
        g_stats.avg_time_ms = g_stats.total_time_ms / g_stats.total;
    }

    /* Print detailed results summary */
    print_test_results_summary();

    TEST_LOG_HEADER();
    OSAL_Printf(" %u tests from %u test suites ran (%u ms total)\n",
                g_stats.total, suite_count, g_stats.total_time_ms);
    OSAL_Printf("[ PASSED   ] %u tests\n", g_stats.passed);

    if (g_stats.failed > 0) {
        OSAL_Printf("[ FAILED   ] %u tests\n", g_stats.failed);
    }

    if (g_stats.skipped > 0) {
        OSAL_Printf("[ SKIPPED  ] %u tests\n", g_stats.skipped);
    }

    /* Close log file */
    close_test_log();

    return (0 == g_stats.failed) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * Run tests from a specific layer
 */
int32_t libutest_run_layer(const char *layer_name)
{
    if (NULL == layer_name) {
        return OSAL_ERR_INVALID_POINTER;
    }

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_layer(layer_name, suites, MAX_SUITES);

    if (0 == count) {
        OSAL_Printf("No tests found for layer: %s\n", layer_name);
        return OSAL_ERR_GENERIC;
    }

    /* Open log file */
    open_test_log();

    TEST_LOG_HEADER();
    OSAL_Printf(" Running %u test suites from layer %s\n", count, layer_name);

    libutest_reset_stats();

    uint32_t start_time = OSAL_GetTickCount();

    for (uint32_t i = 0; i < count; i++) {
        run_suite(suites[i]);
    }

    uint32_t end_time = OSAL_GetTickCount();
    g_stats.total_time_ms = end_time - start_time;
    if (g_stats.total > 0) {
        g_stats.avg_time_ms = g_stats.total_time_ms / g_stats.total;
    }

    /* Print detailed results summary */
    print_test_results_summary();

    TEST_LOG_HEADER();
    OSAL_Printf(" %u tests from layer %s ran (%llu ms total)\n",
                g_stats.total, layer_name, (unsigned long long)g_stats.total_time_ms);
    OSAL_Printf("[ PASSED   ] %u tests\n", g_stats.passed);

    if (g_stats.failed > 0) {
        OSAL_Printf("[ FAILED   ] %u tests\n", g_stats.failed);
    }

    if (g_stats.skipped > 0) {
        OSAL_Printf("[ SKIPPED  ] %u tests\n", g_stats.skipped);
    }

    /* Close log file */
    close_test_log();

    return (0 == g_stats.failed) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * Run tests from a specific module
 */
int32_t libutest_run_module(const char *module_name)
{
    if (NULL == module_name) {
        return OSAL_ERR_INVALID_POINTER;
    }

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_module(module_name, suites, MAX_SUITES);

    if (0 == count) {
        OSAL_Printf("No tests found for module: %s\n", module_name);
        return OSAL_ERR_GENERIC;
    }

    /* Open log file */
    open_test_log();

    OSAL_Printf("\n[==========] Running %u test suites from module %s\n", count, module_name);

    libutest_reset_stats();

    for (uint32_t i = 0; i < count; i++) {
        run_suite(suites[i]);
    }

    /* Print detailed results summary */
    print_test_results_summary();

    OSAL_Printf("\n[==========] %u tests from %u test suites ran\n", g_stats.total, count);
    OSAL_Printf("[  PASSED  ] %u tests\n", g_stats.passed);

    if (g_stats.failed > 0) {
        OSAL_Printf("[  FAILED  ] %u tests\n", g_stats.failed);
    }

    if (g_stats.skipped > 0) {
        OSAL_Printf("[  SKIPPED ] %u tests\n", g_stats.skipped);
    }

    /* Close log file */
    close_test_log();

    return (0 == g_stats.failed) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * Run a specific test suite
 */
int32_t libutest_run_suite(const char *suite_name)
{
    if (NULL == suite_name) {
        return OSAL_ERR_INVALID_POINTER;
    }

    const test_suite_t *suite = test_find_suite(suite_name);
    if (NULL == suite) {
        OSAL_Printf("Test suite not found: %s\n", suite_name);
        return OSAL_ERR_GENERIC;
    }

    /* Open log file */
    open_test_log();

    libutest_reset_stats();
    run_suite(suite);

    /* Print detailed results summary */
    print_test_results_summary();

    OSAL_Printf("\n[  PASSED  ] %u tests\n", g_stats.passed);

    if (g_stats.failed > 0) {
        OSAL_Printf("[  FAILED  ] %u tests\n", g_stats.failed);
    }

    if (g_stats.skipped > 0) {
        OSAL_Printf("[  SKIPPED ] %u tests\n", g_stats.skipped);
    }

    /* Close log file */
    close_test_log();

    return (0 == g_stats.failed) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * Run a specific test case
 */
int32_t libutest_run_test(const char *suite_name, const char *test_name)
{
    if (NULL == suite_name || NULL == test_name) {
        return OSAL_ERR_INVALID_POINTER;
    }

    const test_suite_t *suite = test_find_suite(suite_name);
    if (NULL == suite) {
        OSAL_Printf("Test suite not found: %s\n", suite_name);
        return OSAL_ERR_GENERIC;
    }

    /* Find test case */
    const test_case_t *test = NULL;
    for (uint32_t i = 0; i < suite->case_count; i++) {
        if (0 == OSAL_Strcmp(suite->cases[i].name, test_name)) {
            test = &suite->cases[i];
            break;
        }
    }

    if (NULL == test) {
        OSAL_Printf("Test case not found: %s\n", test_name);
        return OSAL_ERR_GENERIC;
    }

    /* Open log file */
    open_test_log();

    libutest_reset_stats();

    test_result_t result = run_test_case(test, suite_name);

    g_stats.total = 1;
    if (result == TEST_RESULT_PASS) {
        g_stats.passed = 1;
    } else if (result == TEST_RESULT_FAIL) {
        g_stats.failed = 1;
    } else {
        g_stats.skipped = 1;
    }

    /* Close log file */
    close_test_log();

    return (result == TEST_RESULT_PASS) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * Get test statistics
 */
const test_stats_t* libutest_get_stats(void)
{
    return &g_stats;
}

/**
 * Reset test statistics
 */
void libutest_reset_stats(void)
{
    OSAL_Memset(&g_stats, 0, sizeof(test_stats_t));
}
