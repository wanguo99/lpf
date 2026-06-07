/**
 * @file test_runner.c
 * @brief Test execution engine
 */

#include "test_core.h"
#include "test_assert.h"
#include "osal.h"
#include <stdarg.h>

#define MAX_SUITES 128
#define LOG_BUF_SIZE 512

/* Global state */
static int32_t g_test_log_fd = -1;
static test_stats_t g_stats = {0};

bool g_test_failed = false;
const char *g_current_test = NULL;

/* External registry functions */
extern const test_suite_t** test_get_all_suites(uint32_t *count);
extern const test_suite_t* test_find_suite(const char *name);
extern uint32_t test_get_suites_by_layer(const char *layer_name, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_module(const char *module_name, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_filtered_suites(const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_layer_filtered(const char *layer_name, const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_module_filtered(const char *module_name, const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);

/*===========================================================================
 * Logging helpers
 *===========================================================================*/

static inline void log_write(const char *msg)
{
    if (g_test_log_fd >= 0) {
        OSAL_write(g_test_log_fd, msg, OSAL_strlen(msg));
    }
}

static void log_printf(const char *fmt, ...)
{
    if (g_test_log_fd >= 0) {
        char buf[LOG_BUF_SIZE];
        va_list args;
        va_start(args, fmt);
        OSAL_vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        OSAL_write(g_test_log_fd, buf, OSAL_strlen(buf));
    }
}

#define LOG_RUN(name)   do { OSAL_Printf("[ RUN      ] %s\n", name); log_printf("[ RUN      ] %s\n", name); } while(0)
#define LOG_PASS(name)  log_printf("[ OK       ] %s\n", name)
#define LOG_FAIL(name)  do { OSAL_Printf("[ FAILED   ] %s\n", name); log_printf("[ FAILED   ] %s\n", name); } while(0)
#define LOG_SEP()       log_write("[----------]\n")
#define LOG_HEADER()    do { OSAL_Printf("[==========]\n"); log_write("[==========]\n"); } while(0)
#define LOG_DETAIL(...) log_printf(__VA_ARGS__)

static void open_test_log(void)
{
    if (g_test_log_fd < 0) {
        g_test_log_fd = OSAL_open("/tmp/ems_test.log", OSAL_O_WRONLY | OSAL_O_CREAT | OSAL_O_TRUNC, 0644);
        if (g_test_log_fd >= 0) {
            OSAL_Printf("Test log: /tmp/ems_test.log\n\n");
        }
    }
}

static void close_test_log(void)
{
    if (g_test_log_fd >= 0) {
        OSAL_close(g_test_log_fd);
        g_test_log_fd = -1;
    }
}

/*===========================================================================
 * Result tracking
 *===========================================================================*/

static void append_to_list(test_result_node_t **head, test_result_node_t **tail, test_result_node_t *node)
{
    if (*head == NULL) {
        *head = *tail = node;
    } else {
        (*tail)->next = node;
        *tail = node;
    }
}

static void add_test_result(const char *suite_name, const char *test_name, test_result_t result, uint32_t elapsed_ms)
{
    test_result_node_t *node = (test_result_node_t *)OSAL_malloc(sizeof(test_result_node_t));
    if (NULL == node) return;

    node->suite_name = suite_name;
    node->test_name = test_name;
    node->elapsed_ms = elapsed_ms;
    node->next = NULL;

    if (result == TEST_RESULT_PASS) {
        append_to_list(&g_stats.passed_list_head, &g_stats.passed_list_tail, node);
    } else {
        append_to_list(&g_stats.failed_list_head, &g_stats.failed_list_tail, node);
    }
}

static void print_result_list(const char *header, uint32_t count, test_result_node_t *head, const char *tag)
{
    const char *current_suite = NULL;
    test_result_node_t *node;

    if (count == 0) return;

    OSAL_Printf("\n%s %u tests\n", header, count);

    for (node = head; NULL != node; node = node->next) {
        if (NULL == current_suite || OSAL_strcmp(current_suite, node->suite_name) != 0) {
            if (current_suite) OSAL_Printf("\n");
            OSAL_Printf("  [%s]\n", node->suite_name);
            current_suite = node->suite_name;
        }
        OSAL_Printf("    [%s] %s (%u ms)\n", tag, node->test_name, node->elapsed_ms);
    }
}

static void print_summary(void)
{
    if (g_stats.failed == 0) return;

    OSAL_Printf("\n");
    LOG_HEADER();
    OSAL_Printf(" Test Result Summary\n");
    LOG_HEADER();
    print_result_list("[  FAILED  ]", g_stats.failed, g_stats.failed_list_head, "FAIL");
    OSAL_Printf("\n");
}

/* External reporter functions */
extern void libutest_print_slowest_tests(uint32_t top_n);
extern void libutest_print_suite_stats(void);

/*===========================================================================
 * Test execution
 *===========================================================================*/

static test_result_t run_test_case(const test_case_t *test, const char *suite_name)
{
    if (NULL == test || NULL == test->func) return TEST_RESULT_FAIL;

    g_test_failed = false;
    g_current_test = test->name;
    LOG_RUN(test->name);

    uint32_t start_time = OSAL_GetTickCount();

    /* Setup */
    if (test->setup) {
        test->setup();
        if (g_test_failed) {
            LOG_DETAIL("[  FAILED  ] Setup failed for %s\n", test->name);
            add_test_result(suite_name, test->name, TEST_RESULT_FAIL, OSAL_GetTickCount() - start_time);
            return TEST_RESULT_FAIL;
        }
    }

    /* Test */
    test->func();

    /* Teardown */
    if (test->teardown) {
        bool failed_before = g_test_failed;
        g_test_failed = false;
        test->teardown();
        if (!g_test_failed) g_test_failed = failed_before;
    }

    uint32_t elapsed = OSAL_GetTickCount() - start_time;

    /* Result */
    test_result_t result = g_test_failed ? TEST_RESULT_FAIL : TEST_RESULT_PASS;

    if (g_test_failed) {
        LOG_FAIL(test->name);
        if (g_stats.failed_test_count < 64) {
            g_stats.failed_tests[g_stats.failed_test_count++] = test->name;
        }
    } else {
        LOG_PASS(test->name);
    }

    LOG_DETAIL("             (elapsed: %u ms)\n\n", elapsed);
    add_test_result(suite_name, test->name, result, elapsed);

    return result;
}

static int32_t run_suite(const test_suite_t *suite)
{
    uint32_t i;
    test_result_t result;

    if (NULL == suite) return OSAL_ERR_GENERIC;

    LOG_SEP();
    LOG_DETAIL(" Running %u tests from %s\n", suite->case_count, suite->suite_name);

    if (suite->suite_setup) suite->suite_setup();

    for (i = 0; i < suite->case_count; i++) {
        result = run_test_case(&suite->cases[i], suite->suite_name);
        g_stats.total++;
        if (result == TEST_RESULT_PASS) {
            g_stats.passed++;
        } else {
            g_stats.failed++;
        }
    }

    if (suite->suite_teardown) suite->suite_teardown();

    LOG_SEP();
    LOG_DETAIL(" %u tests from %s\n\n", suite->case_count, suite->suite_name);

    return OSAL_SUCCESS;
}

static void run_suites_and_report(const test_suite_t **suites, uint32_t count, const char *scope_desc)
{
    uint32_t start_time;
    uint32_t i;

    open_test_log();
    libutest_reset_stats();

    start_time = OSAL_GetTickCount();

    for (i = 0; i < count; i++) {
        run_suite(suites[i]);
    }

    g_stats.total_time_ms = OSAL_GetTickCount() - start_time;
    if (g_stats.total > 0) {
        g_stats.avg_time_ms = g_stats.total_time_ms / g_stats.total;
    }

    print_summary();

    LOG_HEADER();
    OSAL_Printf(" %u tests from %s ran (%llu ms total)\n",
                g_stats.total, scope_desc, (unsigned long long)g_stats.total_time_ms);
    OSAL_Printf("[ PASSED   ] %u tests\n", g_stats.passed);
    if (g_stats.failed > 0) {
        OSAL_Printf("[ FAILED   ] %u tests\n", g_stats.failed);
    }

    /* Print additional statistics if there are tests */
    if (g_stats.total > 0) {
        libutest_print_suite_stats();
        libutest_print_slowest_tests(10);  /* Show top 10 slowest tests */
    }

    close_test_log();
}

/*===========================================================================
 * Public API
 *===========================================================================*/

int32_t libutest_run_all(void)
{
    uint32_t count = 0;
    const test_suite_t **suites = test_get_all_suites(&count);

    LOG_HEADER();
    OSAL_Printf(" Running %u test suites\n", count);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "%u test suites", count);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_all_filtered(const test_filter_t *filter)
{
    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_filtered_suites(filter, suites, MAX_SUITES);

    if (count == 0) {
        OSAL_Printf("No tests match the specified filter\n");
        return OSAL_ERR_GENERIC;
    }

    LOG_HEADER();
    OSAL_Printf(" Running %u filtered test suites\n", count);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "%u filtered test suites", count);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_layer(const char *layer_name)
{
    if (NULL == layer_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_layer(layer_name, suites, MAX_SUITES);

    if (count == 0) {
        OSAL_Printf("No tests found for layer: %s\n", layer_name);
        return OSAL_ERR_GENERIC;
    }

    LOG_HEADER();
    OSAL_Printf(" Running %u test suites from layer %s\n", count, layer_name);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "layer %s", layer_name);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_layer_filtered(const char *layer_name, const test_filter_t *filter)
{
    if (NULL == layer_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_layer_filtered(layer_name, filter, suites, MAX_SUITES);

    if (count == 0) {
        OSAL_Printf("No tests found for layer: %s (with active filters)\n", layer_name);
        return OSAL_ERR_GENERIC;
    }

    LOG_HEADER();
    OSAL_Printf(" Running %u filtered test suites from layer %s\n", count, layer_name);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "filtered layer %s", layer_name);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_module(const char *module_name)
{
    if (NULL == module_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_module(module_name, suites, MAX_SUITES);

    if (count == 0) {
        OSAL_Printf("No tests found for module: %s\n", module_name);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Printf("\n[==========] Running %u test suites from module %s\n", count, module_name);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "%u test suites", count);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_module_filtered(const char *module_name, const test_filter_t *filter)
{
    if (NULL == module_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_module_filtered(module_name, filter, suites, MAX_SUITES);

    if (count == 0) {
        OSAL_Printf("No tests found for module: %s (with active filters)\n", module_name);
        return OSAL_ERR_GENERIC;
    }

    OSAL_Printf("\n[==========] Running %u filtered test suites from module %s\n", count, module_name);

    char desc[64];
    OSAL_snprintf(desc, sizeof(desc), "%u filtered test suites", count);
    run_suites_and_report(suites, count, desc);

    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_suite(const char *suite_name)
{
    if (NULL == suite_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suite = test_find_suite(suite_name);
    if (NULL == suite) {
        OSAL_Printf("Test suite not found: %s\n", suite_name);
        return OSAL_ERR_GENERIC;
    }

    open_test_log();
    libutest_reset_stats();
    run_suite(suite);
    print_summary();

    OSAL_Printf("\n[  PASSED  ] %u tests\n", g_stats.passed);
    if (g_stats.failed > 0) {
        OSAL_Printf("[  FAILED  ] %u tests\n", g_stats.failed);
    }

    close_test_log();
    return (g_stats.failed == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t libutest_run_test(const char *suite_name, const char *test_name)
{
    if (NULL == suite_name || NULL == test_name) return OSAL_ERR_INVALID_POINTER;

    const test_suite_t *suite = test_find_suite(suite_name);
    if (NULL == suite) {
        OSAL_Printf("Test suite not found: %s\n", suite_name);
        return OSAL_ERR_GENERIC;
    }

    const test_case_t *test = NULL;
    uint32_t i;

    for (i = 0; i < suite->case_count; i++) {
        if (0 == OSAL_strcmp(suite->cases[i].name, test_name)) {
            test = &suite->cases[i];
            break;
        }
    }

    if (NULL == test) {
        OSAL_Printf("Test case not found: %s\n", test_name);
        return OSAL_ERR_GENERIC;
    }

    open_test_log();
    libutest_reset_stats();

    test_result_t result = run_test_case(test, suite_name);
    g_stats.total = 1;
    if (result == TEST_RESULT_PASS) {
        g_stats.passed = 1;
    } else {
        g_stats.failed = 1;
    }

    close_test_log();
    return (result == TEST_RESULT_PASS) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

const test_stats_t* libutest_get_stats(void)
{
    return &g_stats;
}

/**
 * @brief 释放测试结果链表
 */
static void free_result_list(test_result_node_t *head)
{
    test_result_node_t *current = head;
    while (current)
    {
        test_result_node_t *next = current->next;
        OSAL_free(current);
        current = next;
    }
}

void libutest_reset_stats(void)
{
    /* 释放链表内存 */
    free_result_list(g_stats.passed_list_head);
    free_result_list(g_stats.failed_list_head);

    /* 清零统计结构 */
    OSAL_memset(&g_stats, 0, sizeof(test_stats_t));
}
