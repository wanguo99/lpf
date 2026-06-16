/**
 * @file test_core.h
 * @brief 通用单元测试框架核心
 *
 * 提供通用的、平台无关的测试框架，支持：
 * - 自动测试注册
 * - 层级化组织
 * - 交互式菜单和命令行模式
 * - 测试元数据（分类、标签、超时）
 * - 动态过滤
 *
 * 依赖：仅依赖OSAL（保证可移植性）
 */

#ifndef TEST_CORE_H
#define TEST_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "test_metadata.h"

/* 注意：源文件需要包含 osal.h 以使用 OSAL API */

/* Test function signatures */
typedef void (*test_func_t)(void);
typedef void (*fixture_func_t)(void);

/* Test case structure */
typedef struct {
    const char *name;
    test_func_t func;
    fixture_func_t setup;
    fixture_func_t teardown;
} test_case_t;

/**
 * Auto-collected test case structure (for linker section-based collection)
 * This allows TEST_CASE() to automatically register without explicit TEST_CASE_REF()
 */
typedef struct {
    const char *section_name;  /**< Section name to identify which suite this belongs to */
    test_case_t case_info;     /**< Test case information */
} auto_test_case_t;

/* Test suite structure with metadata support */
typedef struct {
    const char *suite_name;
    const char *module_name;
    const char *layer_name;
    const test_case_t *cases;
    uint32_t case_count;
    fixture_func_t suite_setup;
    fixture_func_t suite_teardown;
    test_metadata_t metadata;  /**< Test suite metadata (category, tags, timeout, description) */
} test_suite_t;

/* Test result codes */
typedef enum {
    TEST_RESULT_PASS = 0,
    TEST_RESULT_FAIL = 1
} test_result_t;

/* Test result record (for detailed summary) */
typedef struct test_result_node {
    const char *suite_name;
    const char *test_name;
    uint32_t elapsed_ms;
    struct test_result_node *next;
} test_result_node_t;

/* Test statistics */
typedef struct {
    uint32_t total;
    uint32_t passed;
    uint32_t failed;
    uint64_t total_time_ms;      /* Total execution time in milliseconds */
    uint64_t avg_time_ms;        /* Average execution time per test */
    const char *failed_tests[64]; /* List of failed test names (legacy) */
    uint32_t failed_test_count;  /* Number of failed tests in the list (legacy) */

    /* Separate linked lists for each result type */
    test_result_node_t *passed_list_head;
    test_result_node_t *passed_list_tail;
    test_result_node_t *failed_list_head;
    test_result_node_t *failed_list_tail;
} test_stats_t;

/* Core API - Test Registration */
void libutest_register_suite(const test_suite_t *suite);

/* Core API - Test Execution */
int32_t libutest_run_all(void);
int32_t libutest_run_all_filtered(const test_filter_t *filter);
int32_t libutest_run_layer(const char *layer_name);
int32_t libutest_run_layer_filtered(const char *layer_name, const test_filter_t *filter);
int32_t libutest_run_module(const char *module_name);
int32_t libutest_run_module_filtered(const char *module_name, const test_filter_t *filter);
int32_t libutest_run_suite(const char *suite_name);
int32_t libutest_run_test(const char *suite_name, const char *test_name);

/* Core API - Test Discovery */
void libutest_list_all(void);
void libutest_list_layer(const char *layer_name);
void libutest_list_module(const char *module_name);
void libutest_list_layers(void);
void libutest_list_modules(const char *layer_name);
void libutest_list_suites(const char *layer_name, const char *module_name);

/* Core API - Test Case Printing */
void libutest_print_all(void);
void libutest_print_layer(const char *layer_name);
void libutest_print_module(const char *module_name);

/* Core API - Interactive Mode */
int32_t libutest_interactive_menu(void);
int32_t libutest_interactive_menu_filtered(const test_filter_t *filter);

/* Core API - Statistics */
const test_stats_t* libutest_get_stats(void);
void libutest_reset_stats(void);

/* Core API - Reporting and Export */
int32_t libutest_export_junit_xml(const char *output_path);
int32_t libutest_export_json(const char *output_path);
void libutest_print_slowest_tests(uint32_t top_n);
void libutest_print_suite_stats(void);

#endif /* TEST_CORE_H */
