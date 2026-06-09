/**
 * @file main.c
 * @brief Unified test runner - Busybox-style main entry point
 *
 * Supports both direct invocation (es-middleware-test) and symlink invocation
 * (osal-test, hal-test, pcl-test, pdl-test)
 */

#include "test_core.h"
#include "osal.h"

/**
 * Extract program name from argv[0] (handles both full path and basename)
 */
static const char *get_program_name(const char *argv0)
{
    const char *name = argv0;
    const char *p = argv0;

    /* Find last '/' character */
    while (*p != '\0') {
        if (*p == '/') {
            name = p + 1;
        }
        p++;
    }

    return name;
}

/**
 * Busybox-style dispatch: check if invoked via symlink
 * Returns layer filter string or NULL for full test suite
 */
static const char *detect_layer_filter(const char *program_name)
{
    if (0 == OSAL_strcmp(program_name, "osal-test")) {
        return "OSAL";
    } else if (0 == OSAL_strcmp(program_name, "hal-test")) {
        return "HAL";
    } else if (0 == OSAL_strcmp(program_name, "pcl-test")) {
        return "PCL";
    } else if (0 == OSAL_strcmp(program_name, "pdl-test")) {
        return "PDL";
    }
    return NULL;  /* es-middleware-test: no filter */
}

/**
 * Print usage information
 */
static void print_usage(const char *program_name)
{
    OSAL_Printf("\nES-Middleware Test Runner (Busybox-style)\n");
    OSAL_Printf("==========================================\n\n");

    if (0 == OSAL_strcmp(program_name, "es-middleware-test")) {
        OSAL_Printf("Usage: %s [options]\n\n", program_name);
        OSAL_Printf("Options:\n");
        OSAL_Printf("  -a, --all              Run all tests\n");
        OSAL_Printf("  -L <layer>             Run tests from specific layer (OSAL, HAL, PCL, PDL)\n");
        OSAL_Printf("  -m <module>            Run tests from specific module\n");
        OSAL_Printf("  -s <suite>             Run specific test suite\n");
        OSAL_Printf("  -l, --list             List all available tests\n");
        OSAL_Printf("  -i, --interactive      Interactive menu (default)\n");
        OSAL_Printf("  -h, --help             Show this help message\n\n");
        OSAL_Printf("Filtering Options:\n");
        OSAL_Printf("  --fast                 Run only fast tests (<100ms)\n");
        OSAL_Printf("  --slow                 Run only slow tests (>1s)\n");
        OSAL_Printf("  --exclude-hardware     Exclude tests requiring hardware\n");
        OSAL_Printf("  --exclude-network      Exclude tests requiring network\n");
        OSAL_Printf("  --category <cat>       Filter by category (unit/performance/stress/system)\n");
        OSAL_Printf("  --tags <tags>          Filter by tags (comma-separated)\n\n");
        OSAL_Printf("Output and Reporting:\n");
        OSAL_Printf("  --format <fmt>         Output format: text (default), junit, json\n");
        OSAL_Printf("  --output <file>        Output file path (for junit/json formats)\n");
        OSAL_Printf("  --no-stats             Disable per-suite statistics and slowest tests\n\n");
        OSAL_Printf("Examples:\n");
        OSAL_Printf("  %s                     # Interactive menu\n", program_name);
        OSAL_Printf("  %s -a                  # Run all tests\n", program_name);
        OSAL_Printf("  %s -a --fast           # Run all fast tests\n", program_name);
        OSAL_Printf("  %s -a --exclude-hardware  # Run tests without hardware requirement\n", program_name);
        OSAL_Printf("  %s -L OSAL             # Run all OSAL tests\n", program_name);
        OSAL_Printf("  %s -m test_osal_mutex  # Run specific module\n", program_name);
        OSAL_Printf("  %s -s osal_mutex       # Run specific suite\n");
        OSAL_Printf("  %s --category unit     # Run only unit tests\n", program_name);
        OSAL_Printf("  %s -a --format junit --output report.xml  # CI integration\n", program_name);
        OSAL_Printf("  %s -a --format json --output results.json # JSON export\n\n", program_name);
        OSAL_Printf("Symlink Usage:\n");
        OSAL_Printf("  osal-test --all        # Run all OSAL tests\n");
        OSAL_Printf("  hal-test --list        # List all HAL tests\n");
        OSAL_Printf("  pdl-test -s pdl_mcu    # Run PDL MCU suite\n\n");
    } else {
        /* Invoked via symlink */
        const char *layer = detect_layer_filter(program_name);
        OSAL_Printf("Usage: %s [options]\n\n", program_name);
        OSAL_Printf("This is a symlink to es-middleware-test, filtered for %s layer tests.\n\n", layer);
        OSAL_Printf("Options:\n");
        OSAL_Printf("  -a, --all              Run all %s tests\n", layer);
        OSAL_Printf("  -s <suite>             Run specific test suite\n");
        OSAL_Printf("  -l, --list             List all %s tests\n", layer);
        OSAL_Printf("  -i, --interactive      Interactive menu\n");
        OSAL_Printf("  -h, --help             Show this help message\n\n");
        OSAL_Printf("Filtering Options:\n");
        OSAL_Printf("  --fast                 Run only fast tests\n");
        OSAL_Printf("  --exclude-hardware     Exclude hardware tests\n\n");
        OSAL_Printf("Output and Reporting:\n");
        OSAL_Printf("  --format <fmt>         Output format: text, junit, json\n");
        OSAL_Printf("  --output <file>        Output file path\n\n");
        OSAL_Printf("Examples:\n");
        OSAL_Printf("  %s                     # Run all %s tests\n", program_name, layer);
        OSAL_Printf("  %s --list              # List %s tests\n", program_name, layer);
        OSAL_Printf("  %s -s <suite>          # Run specific suite\n", program_name);
        OSAL_Printf("  %s --fast              # Run fast %s tests\n", program_name, layer);
        OSAL_Printf("  %s --format junit --output report.xml  # CI integration\n\n", program_name);
    }
}

/**
 * Parse category string to enum
 */
static test_category_t parse_category(const char *str)
{
    if (0 == OSAL_strcmp(str, "unit")) {
        return TEST_CATEGORY_UNIT;
    } else if (0 == OSAL_strcmp(str, "performance") || 0 == OSAL_strcmp(str, "perf")) {
        return TEST_CATEGORY_PERFORMANCE;
    } else if (0 == OSAL_strcmp(str, "stress")) {
        return TEST_CATEGORY_STRESS;
    } else if (0 == OSAL_strcmp(str, "system")) {
        return TEST_CATEGORY_SYSTEM;
    }
    return TEST_CATEGORY_MAX;  /* Invalid */
}

/**
 * Parse tags string (comma-separated) to bitmask
 */
static uint32_t parse_tags(const char *str)
{
    uint32_t tags = 0;
    char buffer[256];
    OSAL_strncpy(buffer, str, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = buffer;
    char *next = buffer;

    while (*next != '\0') {
        /* Find next comma or end */
        while (*next != '\0' && *next != ',') {
            next++;
        }

        /* Null-terminate current token */
        bool has_more = (*next == ',');
        if (has_more) {
            *next = '\0';
            next++;
        }

        /* Trim leading spaces */
        while (*token == ' ') {
            token++;
        }

        /* Trim trailing spaces */
        char *end = token;
        while (*end != '\0') {
            end++;
        }
        while (end > token && *(end - 1) == ' ') {
            end--;
        }
        *end = '\0';

        if (0 == OSAL_strcmp(token, "fast")) {
            tags |= TEST_TAG_FAST;
        } else if (0 == OSAL_strcmp(token, "slow")) {
            tags |= TEST_TAG_SLOW;
        } else if (0 == OSAL_strcmp(token, "hardware")) {
            tags |= TEST_TAG_HARDWARE;
        } else if (0 == OSAL_strcmp(token, "network")) {
            tags |= TEST_TAG_NETWORK;
        } else if (0 == OSAL_strcmp(token, "filesystem")) {
            tags |= TEST_TAG_FILESYSTEM;
        } else if (0 == OSAL_strcmp(token, "memory")) {
            tags |= TEST_TAG_MEMORY_INTENSIVE;
        } else if (0 == OSAL_strcmp(token, "cpu")) {
            tags |= TEST_TAG_CPU_INTENSIVE;
        } else if (0 == OSAL_strcmp(token, "privileged")) {
            tags |= TEST_TAG_PRIVILEGED;
        }

        if (!has_more) break;
        token = next;
    }

    return tags;
}

/**
 * Export test results after running tests
 */
static void export_test_results(const char *format, const char *output_file)
{
    if (!format || !output_file) return;

    if (0 == OSAL_strcmp(format, "junit")) {
        libutest_export_junit_xml(output_file);
    } else if (0 == OSAL_strcmp(format, "json")) {
        libutest_export_json(output_file);
    } else if (0 != OSAL_strcmp(format, "text")) {
        OSAL_Printf("Warning: Unknown output format '%s', using text\n", format);
    }
}

int main(int argc, char *argv[])
{
    const char *program_name = get_program_name(argv[0]);
    const char *layer_filter = detect_layer_filter(program_name);

    /* Initialize filter */
    test_filter_t filter = {
        .category_mask = 0,
        .include_tags = 0,
        .exclude_tags = 0,
        .max_timeout_ms = 0,
        .enabled = false
    };

    /* Output format configuration */
    const char *output_format = "text";  /* Default: text */
    const char *output_file = NULL;
    bool enable_stats = true;  /* Default: show statistics */

    /* Parse filtering options first */
    int i;
    for (i = 1; i < argc; i++) {
        if (0 == OSAL_strcmp(argv[i], "--fast")) {
            filter.include_tags |= TEST_TAG_FAST;
            filter.enabled = true;
        } else if (0 == OSAL_strcmp(argv[i], "--slow")) {
            filter.include_tags |= TEST_TAG_SLOW;
            filter.enabled = true;
        } else if (0 == OSAL_strcmp(argv[i], "--exclude-hardware")) {
            filter.exclude_tags |= TEST_TAG_HARDWARE;
            filter.enabled = true;
        } else if (0 == OSAL_strcmp(argv[i], "--exclude-network")) {
            filter.exclude_tags |= TEST_TAG_NETWORK;
            filter.enabled = true;
        } else if (0 == OSAL_strcmp(argv[i], "--category") && i + 1 < argc) {
            test_category_t cat = parse_category(argv[i + 1]);
            if (cat != TEST_CATEGORY_MAX) {
                filter.category_mask = TEST_CATEGORY_MASK(cat);
                filter.enabled = true;
            } else {
                OSAL_Printf("Error: Invalid category '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;  /* Skip category value */
        } else if (0 == OSAL_strcmp(argv[i], "--tags") && i + 1 < argc) {
            filter.include_tags |= parse_tags(argv[i + 1]);
            filter.enabled = true;
            i++;  /* Skip tags value */
        } else if (0 == OSAL_strcmp(argv[i], "--format") && i + 1 < argc) {
            output_format = argv[i + 1];
            i++;  /* Skip format value */
        } else if (0 == OSAL_strcmp(argv[i], "--output") && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;  /* Skip output file value */
        } else if (0 == OSAL_strcmp(argv[i], "--no-stats")) {
            enable_stats = false;
        }
    }

    /* Parse command line arguments */
    if (1 == argc) {
        int32_t result;

        /* No arguments */
        if (layer_filter) {
            /* Symlink invocation: run all tests for that layer */
            OSAL_Printf("Running all %s tests...\n\n", layer_filter);
            if (filter.enabled) {
                result = libutest_run_layer_filtered(layer_filter, &filter);
            } else {
                result = libutest_run_layer(layer_filter);
            }

            /* Export results if requested */
            export_test_results(output_format, output_file);

            return result;
        } else {
            /* Direct invocation: interactive menu */
            return libutest_interactive_menu_filtered(&filter);
        }
    }

    /* Check for -p flag (print test names) */
    bool print_mode = false;
    if (argc >= 2 && 0 == OSAL_strcmp(argv[argc - 1], "-p")) {
        print_mode = true;
        argc--;  /* Remove -p from argument count */
    }

    /* Run all tests */
    if (0 == OSAL_strcmp(argv[1], "-a") || 0 == OSAL_strcmp(argv[1], "--all")) {
        int32_t result;

        if (print_mode) {
            if (layer_filter) {
                libutest_print_layer(layer_filter);
            } else {
                libutest_print_all();
            }
            return 0;
        }
        if (filter.enabled) {
            if (layer_filter) {
                result = libutest_run_layer_filtered(layer_filter, &filter);
            } else {
                result = libutest_run_all_filtered(&filter);
            }
        } else {
            if (layer_filter) {
                result = libutest_run_layer(layer_filter);
            } else {
                result = libutest_run_all();
            }
        }

        /* Export results if requested */
        export_test_results(output_format, output_file);

        return result;
    }

    /* Run tests by layer (only for ems-test) */
    if (0 == OSAL_strcmp(argv[1], "-L") && argc >= 3) {
        int32_t result;

        if (layer_filter) {
            OSAL_Printf("Warning: -L option ignored when invoked via %s\n", program_name);
            OSAL_Printf("Running %s tests instead.\n\n", layer_filter);
            if (filter.enabled) {
                result = libutest_run_layer_filtered(layer_filter, &filter);
            } else {
                result = libutest_run_layer(layer_filter);
            }
        } else {
            if (print_mode) {
                libutest_print_layer(argv[2]);
                return 0;
            }
            if (filter.enabled) {
                result = libutest_run_layer_filtered(argv[2], &filter);
            } else {
                result = libutest_run_layer(argv[2]);
            }
        }

        /* Export results if requested */
        export_test_results(output_format, output_file);

        return result;
    }

    /* Run tests by module */
    if (0 == OSAL_strcmp(argv[1], "-m") && argc >= 3) {
        int32_t result;

        if (print_mode) {
            libutest_print_module(argv[2]);
            return 0;
        }
        if (filter.enabled) {
            result = libutest_run_module_filtered(argv[2], &filter);
        } else {
            result = libutest_run_module(argv[2]);
        }

        /* Export results if requested */
        export_test_results(output_format, output_file);

        return result;
    }

    /* Run specific test suite */
    if (0 == OSAL_strcmp(argv[1], "-s") && argc >= 3) {
        int32_t result = libutest_run_suite(argv[2]);

        /* Export results if requested */
        export_test_results(output_format, output_file);

        return result;
    }

    /* List all tests */
    if (0 == OSAL_strcmp(argv[1], "-l") || 0 == OSAL_strcmp(argv[1], "--list")) {
        if (layer_filter) {
            libutest_print_layer(layer_filter);
        } else {
            libutest_list_all();
        }
        return 0;
    }

    /* Interactive menu */
    if (0 == OSAL_strcmp(argv[1], "-i") || 0 == OSAL_strcmp(argv[1], "--interactive")) {
        return libutest_interactive_menu_filtered(&filter);
    }

    /* Help */
    if (0 == OSAL_strcmp(argv[1], "-h") || 0 == OSAL_strcmp(argv[1], "--help")) {
        print_usage(program_name);
        return 0;
    }

    /* Unknown option */
    OSAL_Printf("Unknown option: %s\n", argv[1]);
    OSAL_Printf("Use -h or --help for usage information\n");
    return 1;
}
