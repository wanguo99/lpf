/**
 * @file test_menu.c
 * @brief Interactive test menu and listing
 *
 * Provides interactive menu for test selection and execution,
 * as well as test listing functionality.
 */

#include "test_core.h"
#include "osal.h"

#define MAX_LAYERS 16
#define MAX_MODULES 64
#define MAX_SUITES 128

/* External registry functions */
extern const test_suite_t** test_get_all_suites(uint32_t *count);
extern uint32_t test_get_layers(const char **layers, uint32_t max_layers);
extern uint32_t test_get_modules(const char **modules, uint32_t max_modules);
extern uint32_t test_get_suites_by_layer(const char *layer_name, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_layer_filtered(const char *layer_name, const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_module(const char *module_name, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_suites_by_module_filtered(const char *module_name, const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);
extern uint32_t test_get_filtered_suites(const test_filter_t *filter, const test_suite_t **suites, uint32_t max_suites);

/**
 * Read user input
 */
static int32_t read_choice(void)
{
    char buffer[16];
    if (OSAL_Fgets(buffer, sizeof(buffer), OSAL_stdin) == NULL) {
        return -1;
    }

    int32_t choice;
    if (OSAL_Sscanf(buffer, "%d", &choice) != 1) {
        return -1;
    }

    return choice;
}

/**
 * Format timeout in human-readable form
 */
static void format_timeout(uint32_t timeout_ms, char *buf, uint32_t buf_size)
{
    if (timeout_ms == 0) {
        OSAL_strncpy(buf, "none", buf_size);
    } else if (timeout_ms < 1000) {
        OSAL_snprintf(buf, buf_size, "%ums", timeout_ms);
    } else if (timeout_ms < 60000) {
        OSAL_snprintf(buf, buf_size, "%.1fs", timeout_ms / 1000.0);
    } else {
        OSAL_snprintf(buf, buf_size, "%.1fm", timeout_ms / 60000.0);
    }
    buf[buf_size - 1] = '\0';
}

/**
 * Display suite metadata in compact form
 */
static void display_suite_metadata(const test_suite_t *suite)
{
    char tags_buf[128];
    char timeout_buf[32];

    test_tags_to_string(suite->metadata.tags, tags_buf, sizeof(tags_buf));
    format_timeout(suite->metadata.timeout_ms, timeout_buf, sizeof(timeout_buf));

    OSAL_Printf("    Category: %s | Tags: %s | Timeout: %s\n",
                test_category_name(suite->metadata.category),
                suite->metadata.tags ? tags_buf : "none",
                timeout_buf);

    if (suite->metadata.description && suite->metadata.description[0] != '\0') {
        OSAL_Printf("    Description: %s\n", suite->metadata.description);
    }
}

/**
 * Count total tests in suite list
 */
static uint32_t count_total_tests(const test_suite_t **suites, uint32_t suite_count)
{
    uint32_t total = 0;
    uint32_t i;

    for (i = 0; i < suite_count; i++) {
        total += suites[i]->case_count;
    }

    return total;
}

/**
 * Display filter status
 */
static void display_filter_status(const test_filter_t *filter)
{
    if (!filter || !filter->enabled) {
        OSAL_Printf("  Filter: None (showing all tests)\n");
        return;
    }

    OSAL_Printf("  Active Filters:\n");

    if (filter->category_mask != 0) {
        OSAL_Printf("    - Category: ");
        bool first = true;
        uint32_t i;
        for (i = 0; i < TEST_CATEGORY_MAX; i++) {
            if (filter->category_mask & (1u << i)) {
                if (!first) {
                    OSAL_Printf(", ");
                }
                OSAL_Printf("%s", test_category_name((test_category_t)i));
                first = false;
            }
        }
        OSAL_Printf("\n");
    }

    if (filter->include_tags != 0) {
        char tags_buf[128];
        test_tags_to_string(filter->include_tags, tags_buf, sizeof(tags_buf));
        OSAL_Printf("    - Include tags: %s\n", tags_buf);
    }

    if (filter->exclude_tags != 0) {
        char tags_buf[128];
        test_tags_to_string(filter->exclude_tags, tags_buf, sizeof(tags_buf));
        OSAL_Printf("    - Exclude tags: %s\n", tags_buf);
    }

    if (filter->max_timeout_ms > 0) {
        char timeout_buf[32];
        format_timeout(filter->max_timeout_ms, timeout_buf, sizeof(timeout_buf));
        OSAL_Printf("    - Max timeout: %s\n", timeout_buf);
    }
}

/**
 * List all tests
 */
void libutest_list_all(void)
{
    uint32_t suite_count = 0;
    const test_suite_t **suites = test_get_all_suites(&suite_count);

    OSAL_Printf("\nRegistered Test Suites (%u total):\n", suite_count);
    OSAL_Printf("=====================================\n");

    uint32_t i;


    for (i = 0; i < suite_count; i++) {
        const test_suite_t *suite = suites[i];
        OSAL_Printf("\n[%s/%s] %s (%u tests)\n",
                   suite->layer_name, suite->module_name, suite->suite_name, suite->case_count);

        uint32_t j;


        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("  - %s\n", suite->cases[j].name);
        }
    }

    OSAL_Printf("\n");
}

/**
 * List tests from a specific layer
 */
void libutest_list_layer(const char *layer_name)
{
    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_layer(layer_name, suites, MAX_SUITES);

    OSAL_Printf("\nTest Suites in layer %s (%u total):\n", layer_name, count);
    OSAL_Printf("=====================================\n");

    uint32_t i;


    for (i = 0; i < count; i++) {
        const test_suite_t *suite = suites[i];
        OSAL_Printf("\n[%s] %s (%u tests)\n",
                   suite->module_name, suite->suite_name, suite->case_count);

        uint32_t j;


        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("  - %s\n", suite->cases[j].name);
        }
    }

    OSAL_Printf("\n");
}

/**
 * List tests from a specific module
 */
void libutest_list_module(const char *module_name)
{
    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_module(module_name, suites, MAX_SUITES);

    OSAL_Printf("\nTest Suites in module %s (%u total):\n", module_name, count);
    OSAL_Printf("=====================================\n");

    uint32_t i;


    for (i = 0; i < count; i++) {
        const test_suite_t *suite = suites[i];
        OSAL_Printf("\n%s (%u tests)\n", suite->suite_name, suite->case_count);

        uint32_t j;


        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("  - %s\n", suite->cases[j].name);
        }
    }

    OSAL_Printf("\n");
}

/**
 * Print all test case names
 */
void libutest_print_all(void)
{
    uint32_t suite_count = 0;
    const test_suite_t **suites = test_get_all_suites(&suite_count);

    uint32_t i;


    for (i = 0; i < suite_count; i++) {
        const test_suite_t *suite = suites[i];
        uint32_t j;

        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("%s\n", suite->cases[j].name);
        }
    }
}

/**
 * Print test case names from a specific layer
 */
void libutest_print_layer(const char *layer_name)
{
    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_layer(layer_name, suites, MAX_SUITES);

    uint32_t i;


    for (i = 0; i < count; i++) {
        const test_suite_t *suite = suites[i];
        uint32_t j;

        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("%s\n", suite->cases[j].name);
        }
    }
}

/**
 * Print test case names from a specific module
 */
void libutest_print_module(const char *module_name)
{
    const test_suite_t *suites[MAX_SUITES];
    uint32_t count = test_get_suites_by_module(module_name, suites, MAX_SUITES);

    uint32_t i;


    for (i = 0; i < count; i++) {
        const test_suite_t *suite = suites[i];
        uint32_t j;

        for (j = 0; j < suite->case_count; j++) {
            OSAL_Printf("%s\n", suite->cases[j].name);
        }
    }
}

/**
 * Interactive menu - select test case
 */
static int32_t menu_select_test(const test_suite_t *suite)
{
    while (1) {
        OSAL_Printf("\n=== Test Cases in %s ===\n", suite->suite_name);
        OSAL_Printf("0. Run all tests in this suite\n");

        uint32_t i;


        for (i = 0; i < suite->case_count; i++) {
            OSAL_Printf("%u. %s\n", i + 1, suite->cases[i].name);
        }

        OSAL_Printf("%u. Back to suite selection\n", suite->case_count + 1);
        OSAL_Printf("%u. Exit\n", suite->case_count + 2);
        OSAL_Printf("\nEnter your choice: ");

        int32_t choice = read_choice();

        if (0 == choice) {
            return libutest_run_suite(suite->suite_name);
        } else if (choice > 0 && choice <= (int32_t)suite->case_count) {
            return libutest_run_test(suite->suite_name, suite->cases[choice - 1].name);
        } else if (choice == (int32_t)(suite->case_count + 1)) {
            return OSAL_SUCCESS;
        } else if (choice == (int32_t)(suite->case_count + 2)) {
            OSAL_Printf("\nExiting...\n");
            OSAL_Exit(0);
        } else {
            OSAL_Printf("Invalid choice. Please try again.\n");
        }
    }
}

/**
 * Interactive menu - select suite
 */
static int32_t menu_select_suite(const test_suite_t **suites, uint32_t count, const char *context)
{
    while (1) {
        uint32_t total_tests = count_total_tests(suites, count);
        OSAL_Printf("\n=== Test Suites %s ===\n", context);
        OSAL_Printf("Total: %u suites, %u tests\n", count, total_tests);
        OSAL_Printf("0. Run all suites\n");

        uint32_t i;


        for (i = 0; i < count; i++) {
            OSAL_Printf("%u. %s (%u tests)\n", i + 1, suites[i]->suite_name, suites[i]->case_count);
            display_suite_metadata(suites[i]);
        }

        OSAL_Printf("%u. Back\n", count + 1);
        OSAL_Printf("%u. Exit\n", count + 2);
        OSAL_Printf("\nEnter your choice: ");

        int32_t choice = read_choice();

        if (0 == choice) {
            /* Run all suites */
            libutest_reset_stats();
            uint32_t i;

            for (i = 0; i < count; i++) {
                libutest_run_suite(suites[i]->suite_name);
            }
            return OSAL_SUCCESS;
        } else if (choice > 0 && choice <= (int32_t)count) {
            menu_select_test(suites[choice - 1]);
        } else if (choice == (int32_t)(count + 1)) {
            return OSAL_SUCCESS;
        } else if (choice == (int32_t)(count + 2)) {
            OSAL_Printf("\nExiting...\n");
            OSAL_Exit(0);
        } else {
            OSAL_Printf("Invalid choice. Please try again.\n");
        }
    }
}

/**
 * Interactive menu - select module
 */
static int32_t menu_select_module(const test_filter_t *filter)
{
    const char *modules[MAX_MODULES];
    uint32_t module_count = test_get_modules(modules, MAX_MODULES);

    while (1) {
        OSAL_Printf("\n=== Select Module ===\n");

        uint32_t i;


        for (i = 0; i < module_count; i++) {
            /* Count tests in this module (with filter if active) */
            const test_suite_t *suites[MAX_SUITES];
            uint32_t count;
            if (filter && filter->enabled) {
                count = test_get_suites_by_module_filtered(modules[i], filter, suites, MAX_SUITES);
            } else {
                count = test_get_suites_by_module(modules[i], suites, MAX_SUITES);
            }
            uint32_t total_tests = count_total_tests(suites, count);
            OSAL_Printf("%u. %s (%u suites, %u tests)\n", i + 1, modules[i], count, total_tests);
        }

        OSAL_Printf("%u. Back to main menu\n", module_count + 1);
        OSAL_Printf("%u. Exit\n", module_count + 2);
        OSAL_Printf("\nEnter your choice: ");

        int32_t choice = read_choice();

        if (choice > 0 && choice <= (int32_t)module_count) {
            const test_suite_t *suites[MAX_SUITES];
            uint32_t count;
            if (filter && filter->enabled) {
                count = test_get_suites_by_module_filtered(modules[choice - 1], filter, suites, MAX_SUITES);
            } else {
                count = test_get_suites_by_module(modules[choice - 1], suites, MAX_SUITES);
            }

            char context[128];
            OSAL_snprintf(context, sizeof(context), "in module %s", modules[choice - 1]);
            menu_select_suite(suites, count, context);
        } else if (choice == (int32_t)(module_count + 1)) {
            return OSAL_SUCCESS;
        } else if (choice == (int32_t)(module_count + 2)) {
            OSAL_Printf("\nExiting...\n");
            OSAL_Exit(0);
        } else {
            OSAL_Printf("Invalid choice. Please try again.\n");
        }
    }
}

/**
 * Interactive menu - select layer
 */
static int32_t menu_select_layer(const test_filter_t *filter)
{
    const char *layers[MAX_LAYERS];
    uint32_t layer_count = test_get_layers(layers, MAX_LAYERS);

    while (1) {
        OSAL_Printf("\n=== Select Layer ===\n");

        uint32_t i;


        for (i = 0; i < layer_count; i++) {
            /* Count tests in this layer (with filter if active) */
            const test_suite_t *suites[MAX_SUITES];
            uint32_t count;
            if (filter && filter->enabled) {
                count = test_get_suites_by_layer_filtered(layers[i], filter, suites, MAX_SUITES);
            } else {
                count = test_get_suites_by_layer(layers[i], suites, MAX_SUITES);
            }
            uint32_t total_tests = count_total_tests(suites, count);
            OSAL_Printf("%u. %s (%u suites, %u tests)\n", i + 1, layers[i], count, total_tests);
        }

        OSAL_Printf("%u. Back to main menu\n", layer_count + 1);
        OSAL_Printf("%u. Exit\n", layer_count + 2);
        OSAL_Printf("\nEnter your choice: ");

        int32_t choice = read_choice();

        if (choice > 0 && choice <= (int32_t)layer_count) {
            const test_suite_t *suites[MAX_SUITES];
            uint32_t count;
            if (filter && filter->enabled) {
                count = test_get_suites_by_layer_filtered(layers[choice - 1], filter, suites, MAX_SUITES);
            } else {
                count = test_get_suites_by_layer(layers[choice - 1], suites, MAX_SUITES);
            }

            char context[128];
            OSAL_snprintf(context, sizeof(context), "in layer %s", layers[choice - 1]);
            menu_select_suite(suites, count, context);
        } else if (choice == (int32_t)(layer_count + 1)) {
            return OSAL_SUCCESS;
        } else if (choice == (int32_t)(layer_count + 2)) {
            OSAL_Printf("\nExiting...\n");
            OSAL_Exit(0);
        } else {
            OSAL_Printf("Invalid choice. Please try again.\n");
        }
    }
}

/**
 * Interactive menu - main (with optional filter)
 */
int32_t libutest_interactive_menu_filtered(const test_filter_t *filter)
{
    uint32_t suite_count = 0;
    const test_suite_t **all_suites = test_get_all_suites(&suite_count);

    /* Count filtered suites if filter is active */
    uint32_t visible_suite_count = suite_count;
    uint32_t visible_test_count = 0;

    if (filter && filter->enabled) {
        const test_suite_t *filtered_suites[MAX_SUITES];
        visible_suite_count = test_get_filtered_suites(filter, filtered_suites, MAX_SUITES);
        visible_test_count = count_total_tests(filtered_suites, visible_suite_count);
    } else {
        visible_test_count = count_total_tests(all_suites, suite_count);
    }

    OSAL_Printf("\n========================================\n");
    OSAL_Printf("  EMS Unit Test Framework\n");
    OSAL_Printf("  Total: %u suites, %u tests\n", suite_count, count_total_tests(all_suites, suite_count));
    if (filter && filter->enabled) {
        OSAL_Printf("  Filtered: %u suites, %u tests\n", visible_suite_count, visible_test_count);
    }
    OSAL_Printf("========================================\n");

    if (filter && filter->enabled) {
        display_filter_status(filter);
    }

    while (1) {
        OSAL_Printf("\n=== Main Menu ===\n");
        OSAL_Printf("1. Run all tests\n");
        OSAL_Printf("2. Select by layer\n");
        OSAL_Printf("3. Select by module\n");
        OSAL_Printf("4. List all tests\n");
        OSAL_Printf("5. Exit\n");
        OSAL_Printf("\nEnter your choice: ");

        int32_t choice = read_choice();

        switch (choice) {
            case 1:
                if (filter && filter->enabled) {
                    libutest_run_all_filtered(filter);
                } else {
                    libutest_run_all();
                }
                break;

            case 2:
                menu_select_layer(filter);
                break;

            case 3:
                menu_select_module(filter);
                break;

            case 4:
                libutest_list_all();
                break;

            case 5:
                OSAL_Printf("\nExiting...\n");
                return OSAL_SUCCESS;

            default:
                OSAL_Printf("Invalid choice. Please try again.\n");
                break;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * Interactive menu - main (backward compatibility)
 */
int32_t libutest_interactive_menu(void)
{
    return libutest_interactive_menu_filtered(NULL);
}
