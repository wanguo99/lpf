/**
 * @file test_reporter.c
 * @brief Test result export and reporting
 *
 * Supports multiple output formats:
 * - JUnit XML (for CI integration with Jenkins, GitLab CI, etc.)
 * - JSON (for automation and custom tooling)
 * - Text (human-readable summary)
 */

#include "test_core.h"
#include "osal.h"

/* Forward declarations */
extern const test_stats_t* libutest_get_stats(void);

/*===========================================================================
 * XML Escaping (for JUnit XML format)
 *===========================================================================*/

/**
 * Escape special XML characters: & < > " '
 * Returns number of bytes written (excluding null terminator)
 */
static uint32_t xml_escape(const char *input, char *output, uint32_t output_size)
{
    uint32_t out_pos = 0;
    const char *p = input;

    if (!input || !output || output_size == 0) return 0;

    while (*p && out_pos + 6 < output_size) {  /* Reserve 6 bytes for longest escape (&quot;) */
        switch (*p) {
            case '&':
                OSAL_memcpy(output + out_pos, "&amp;", 5);
                out_pos += 5;
                break;
            case '<':
                OSAL_memcpy(output + out_pos, "&lt;", 4);
                out_pos += 4;
                break;
            case '>':
                OSAL_memcpy(output + out_pos, "&gt;", 4);
                out_pos += 4;
                break;
            case '"':
                OSAL_memcpy(output + out_pos, "&quot;", 6);
                out_pos += 6;
                break;
            case '\'':
                OSAL_memcpy(output + out_pos, "&apos;", 6);
                out_pos += 6;
                break;
            default:
                output[out_pos++] = *p;
                break;
        }
        p++;
    }

    output[out_pos] = '\0';
    return out_pos;
}

/*===========================================================================
 * JUnit XML Export
 *===========================================================================*/

/**
 * Export test results in JUnit XML format
 * Compatible with Jenkins, GitLab CI, GitHub Actions, etc.
 *
 * Format specification:
 * https://github.com/testmoapp/junitxml
 */
int32_t libutest_export_junit_xml(const char *output_path)
{
    const test_stats_t *stats = libutest_get_stats();
    int32_t fd;
    char buf[1024];
    uint32_t len;
    test_result_node_t *node;
    const char *current_suite = NULL;
    uint32_t suite_tests = 0;
    uint32_t suite_failures = 0;
    uint64_t suite_time_ms = 0;
    char escaped_name[256];

    if (!output_path) return OSAL_ERR_INVALID_POINTER;

    /* Open output file */
    fd = OSAL_open(output_path, OSAL_O_WRONLY | OSAL_O_CREAT | OSAL_O_TRUNC, 0644);
    if (fd < 0) {
        OSAL_Printf("Error: Failed to create JUnit XML file: %s\n", output_path);
        return OSAL_ERR_GENERIC;
    }

    /* XML header */
    len = OSAL_snprintf(buf, sizeof(buf), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    OSAL_write(fd, buf, len);

    /* Root testsuites element */
    len = OSAL_snprintf(buf, sizeof(buf),
                        "<testsuites tests=\"%u\" failures=\"%u\" time=\"%.3f\">\n",
                        stats->total,
                        stats->failed,
                        stats->total_time_ms / 1000.0);
    OSAL_write(fd, buf, len);

    /* Helper macro to write test cases */
#define WRITE_TESTSUITE_HEADER() do { \
    if (current_suite) { \
        xml_escape(current_suite, escaped_name, sizeof(escaped_name)); \
        len = OSAL_snprintf(buf, sizeof(buf), \
                            "  <testsuite name=\"%s\" tests=\"%u\" failures=\"%u\" time=\"%.3f\">\n", \
                            escaped_name, suite_tests, suite_failures, suite_time_ms / 1000.0); \
        OSAL_write(fd, buf, len); \
    } \
} while(0)

#define WRITE_TESTSUITE_FOOTER() do { \
    if (current_suite) { \
        len = OSAL_snprintf(buf, sizeof(buf), "  </testsuite>\n"); \
        OSAL_write(fd, buf, len); \
    } \
} while(0)

    /* Process passed tests */
    for (node = stats->passed_list_head; node != NULL; node = node->next) {
        /* New suite? Close previous, open new */
        if (!current_suite || OSAL_strcmp(current_suite, node->suite_name) != 0) {
            WRITE_TESTSUITE_FOOTER();
            current_suite = node->suite_name;
            suite_tests = 0;
            suite_failures = 0;
            suite_time_ms = 0;
            WRITE_TESTSUITE_HEADER();
        }

        /* Write test case */
        xml_escape(node->test_name, escaped_name, sizeof(escaped_name));
        len = OSAL_snprintf(buf, sizeof(buf),
                            "    <testcase name=\"%s\" time=\"%.3f\"/>\n",
                            escaped_name, node->elapsed_ms / 1000.0);
        OSAL_write(fd, buf, len);

        suite_tests++;
        suite_time_ms += node->elapsed_ms;
    }

    /* Process failed tests */
    for (node = stats->failed_list_head; node != NULL; node = node->next) {
        /* New suite? Close previous, open new */
        if (!current_suite || OSAL_strcmp(current_suite, node->suite_name) != 0) {
            WRITE_TESTSUITE_FOOTER();
            current_suite = node->suite_name;
            suite_tests = 0;
            suite_failures = 0;
            suite_time_ms = 0;
            WRITE_TESTSUITE_HEADER();
        }

        /* Write test case with failure */
        xml_escape(node->test_name, escaped_name, sizeof(escaped_name));
        len = OSAL_snprintf(buf, sizeof(buf),
                            "    <testcase name=\"%s\" time=\"%.3f\">\n",
                            escaped_name, node->elapsed_ms / 1000.0);
        OSAL_write(fd, buf, len);

        len = OSAL_snprintf(buf, sizeof(buf),
                            "      <failure message=\"Test assertion failed\"/>\n");
        OSAL_write(fd, buf, len);

        len = OSAL_snprintf(buf, sizeof(buf), "    </testcase>\n");
        OSAL_write(fd, buf, len);

        suite_tests++;
        suite_failures++;
        suite_time_ms += node->elapsed_ms;
    }

    /* Close last suite */
    WRITE_TESTSUITE_FOOTER();

#undef WRITE_TESTSUITE_HEADER
#undef WRITE_TESTSUITE_FOOTER

    /* Close root element */
    len = OSAL_snprintf(buf, sizeof(buf), "</testsuites>\n");
    OSAL_write(fd, buf, len);

    OSAL_close(fd);
    OSAL_Printf("JUnit XML report: %s\n", output_path);

    return OSAL_SUCCESS;
}

/*===========================================================================
 * JSON Export
 *===========================================================================*/

/**
 * Export test results in JSON format
 * Useful for automation and custom tooling
 */
int32_t libutest_export_json(const char *output_path)
{
    const test_stats_t *stats = libutest_get_stats();
    int32_t fd;
    char buf[1024];
    uint32_t len;
    test_result_node_t *node;
    bool first;

    if (!output_path) return OSAL_ERR_INVALID_POINTER;

    /* Open output file */
    fd = OSAL_open(output_path, OSAL_O_WRONLY | OSAL_O_CREAT | OSAL_O_TRUNC, 0644);
    if (fd < 0) {
        OSAL_Printf("Error: Failed to create JSON file: %s\n", output_path);
        return OSAL_ERR_GENERIC;
    }

    /* Root object */
    len = OSAL_snprintf(buf, sizeof(buf), "{\n");
    OSAL_write(fd, buf, len);

    /* Summary statistics */
    len = OSAL_snprintf(buf, sizeof(buf),
                        "  \"summary\": {\n"
                        "    \"total\": %u,\n"
                        "    \"passed\": %u,\n"
                        "    \"failed\": %u,\n"
                        "    \"total_time_ms\": %llu,\n"
                        "    \"avg_time_ms\": %llu\n"
                        "  },\n",
                        stats->total,
                        stats->passed,
                        stats->failed,
                        (unsigned long long)stats->total_time_ms,
                        (unsigned long long)stats->avg_time_ms);
    OSAL_write(fd, buf, len);

    /* Passed tests array */
    len = OSAL_snprintf(buf, sizeof(buf), "  \"passed\": [\n");
    OSAL_write(fd, buf, len);

    first = true;
    for (node = stats->passed_list_head; node != NULL; node = node->next) {
        if (!first) {
            len = OSAL_snprintf(buf, sizeof(buf), ",\n");
            OSAL_write(fd, buf, len);
        }
        first = false;

        len = OSAL_snprintf(buf, sizeof(buf),
                            "    {\"suite\": \"%s\", \"test\": \"%s\", \"elapsed_ms\": %u}",
                            node->suite_name, node->test_name, node->elapsed_ms);
        OSAL_write(fd, buf, len);
    }

    len = OSAL_snprintf(buf, sizeof(buf), "\n  ],\n");
    OSAL_write(fd, buf, len);

    /* Failed tests array */
    len = OSAL_snprintf(buf, sizeof(buf), "  \"failed\": [\n");
    OSAL_write(fd, buf, len);

    first = true;
    for (node = stats->failed_list_head; node != NULL; node = node->next) {
        if (!first) {
            len = OSAL_snprintf(buf, sizeof(buf), ",\n");
            OSAL_write(fd, buf, len);
        }
        first = false;

        len = OSAL_snprintf(buf, sizeof(buf),
                            "    {\"suite\": \"%s\", \"test\": \"%s\", \"elapsed_ms\": %u}",
                            node->suite_name, node->test_name, node->elapsed_ms);
        OSAL_write(fd, buf, len);
    }

    len = OSAL_snprintf(buf, sizeof(buf), "\n  ]\n");
    OSAL_write(fd, buf, len);

    /* Close root object */
    len = OSAL_snprintf(buf, sizeof(buf), "}\n");
    OSAL_write(fd, buf, len);

    OSAL_close(fd);
    OSAL_Printf("JSON report: %s\n", output_path);

    return OSAL_SUCCESS;
}

/*===========================================================================
 * Slowest Tests Report
 *===========================================================================*/

/**
 * Compare function for sorting test results by elapsed time (descending)
 */
static int compare_elapsed_desc(const void *a, const void *b)
{
    const test_result_node_t *node_a = *(const test_result_node_t **)a;
    const test_result_node_t *node_b = *(const test_result_node_t **)b;

    if (node_a->elapsed_ms > node_b->elapsed_ms) return -1;
    if (node_a->elapsed_ms < node_b->elapsed_ms) return 1;
    return 0;
}

/**
 * Simple insertion sort for test results (replaces qsort)
 */
static void sort_tests_by_time(test_result_node_t **all_tests, uint32_t count)
{
    uint32_t i, j;
    test_result_node_t *key;

    for (i = 1; i < count; i++) {
        key = all_tests[i];
        j = i;

        /* Move elements that are slower than key to one position ahead */
        while (j > 0 && compare_elapsed_desc(&all_tests[j - 1], &key) > 0) {
            all_tests[j] = all_tests[j - 1];
            j--;
        }
        all_tests[j] = key;
    }
}

/**
 * Print the N slowest tests
 */
void libutest_print_slowest_tests(uint32_t top_n)
{
    const test_stats_t *stats = libutest_get_stats();
    test_result_node_t *node;
    test_result_node_t **all_tests;
    uint32_t count = 0;
    uint32_t i;

    if (stats->total == 0) return;

    /* Allocate array for all test pointers */
    all_tests = (test_result_node_t **)OSAL_malloc(stats->total * sizeof(test_result_node_t *));
    if (!all_tests) {
        OSAL_Printf("Error: Failed to allocate memory for slowest tests report\n");
        return;
    }

    /* Collect all tests (passed + failed) */
    for (node = stats->passed_list_head; node != NULL; node = node->next) {
        all_tests[count++] = node;
    }
    for (node = stats->failed_list_head; node != NULL; node = node->next) {
        all_tests[count++] = node;
    }

    /* Sort by elapsed time (descending) */
    sort_tests_by_time(all_tests, count);

    /* Print top N */
    if (top_n > count) top_n = count;

    OSAL_Printf("\n[==========]\n");
    OSAL_Printf(" Top %u Slowest Tests\n", top_n);
    OSAL_Printf("[==========]\n");

    for (i = 0; i < top_n; i++) {
        node = all_tests[i];
        OSAL_Printf("  %4u ms  [%s] %s\n",
                    node->elapsed_ms,
                    node->suite_name,
                    node->test_name);
    }

    OSAL_free(all_tests);
}

/*===========================================================================
 * Per-Suite Statistics
 *===========================================================================*/

/**
 * Per-suite statistics structure
 */
typedef struct {
    const char *suite_name;
    uint32_t total;
    uint32_t passed;
    uint32_t failed;
    uint64_t total_time_ms;
} suite_stats_t;

/**
 * Print per-suite statistics
 */
void libutest_print_suite_stats(void)
{
    const test_stats_t *stats = libutest_get_stats();
    test_result_node_t *node;
    suite_stats_t *suite_stats;
    uint32_t suite_count = 0;
    uint32_t max_suites = 64;
    uint32_t i, j;
    bool found;

    if (stats->total == 0) return;

    /* Allocate suite statistics array */
    suite_stats = (suite_stats_t *)OSAL_malloc(max_suites * sizeof(suite_stats_t));
    if (!suite_stats) {
        OSAL_Printf("Error: Failed to allocate memory for suite statistics\n");
        return;
    }
    OSAL_memset(suite_stats, 0, max_suites * sizeof(suite_stats_t));

    /* Process passed tests */
    for (node = stats->passed_list_head; node != NULL; node = node->next) {
        found = false;
        for (i = 0; i < suite_count; i++) {
            if (OSAL_strcmp(suite_stats[i].suite_name, node->suite_name) == 0) {
                suite_stats[i].total++;
                suite_stats[i].passed++;
                suite_stats[i].total_time_ms += node->elapsed_ms;
                found = true;
                break;
            }
        }
        if (!found && suite_count < max_suites) {
            suite_stats[suite_count].suite_name = node->suite_name;
            suite_stats[suite_count].total = 1;
            suite_stats[suite_count].passed = 1;
            suite_stats[suite_count].failed = 0;
            suite_stats[suite_count].total_time_ms = node->elapsed_ms;
            suite_count++;
        }
    }

    /* Process failed tests */
    for (node = stats->failed_list_head; node != NULL; node = node->next) {
        found = false;
        for (i = 0; i < suite_count; i++) {
            if (OSAL_strcmp(suite_stats[i].suite_name, node->suite_name) == 0) {
                suite_stats[i].total++;
                suite_stats[i].failed++;
                suite_stats[i].total_time_ms += node->elapsed_ms;
                found = true;
                break;
            }
        }
        if (!found && suite_count < max_suites) {
            suite_stats[suite_count].suite_name = node->suite_name;
            suite_stats[suite_count].total = 1;
            suite_stats[suite_count].passed = 0;
            suite_stats[suite_count].failed = 1;
            suite_stats[suite_count].total_time_ms = node->elapsed_ms;
            suite_count++;
        }
    }

    /* Print per-suite statistics */
    OSAL_Printf("\n[==========]\n");
    OSAL_Printf(" Per-Suite Statistics\n");
    OSAL_Printf("[==========]\n");
    OSAL_Printf("  %-30s %6s %6s %6s %10s\n",
                "Suite", "Total", "Pass", "Fail", "Time(ms)");
    OSAL_Printf("  %s\n", "-----------------------------------------------------------------------");

    for (i = 0; i < suite_count; i++) {
        OSAL_Printf("  %-30s %6u %6u %6u %10llu\n",
                    suite_stats[i].suite_name,
                    suite_stats[i].total,
                    suite_stats[i].passed,
                    suite_stats[i].failed,
                    (unsigned long long)suite_stats[i].total_time_ms);
    }

    OSAL_free(suite_stats);
}
