/**
 * @file main.c
 * @brief Unified test runner - Busybox-style main entry point
 *
 * Supports both direct invocation (ems-test) and symlink invocation
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
    if (0 == OSAL_Strcmp(program_name, "osal-test")) {
        return "OSAL";
    } else if (0 == OSAL_Strcmp(program_name, "hal-test")) {
        return "HAL";
    } else if (0 == OSAL_Strcmp(program_name, "pcl-test")) {
        return "PCL";
    } else if (0 == OSAL_Strcmp(program_name, "pdl-test")) {
        return "PDL";
    }
    return NULL;  /* ems-test: no filter */
}

/**
 * Print usage information
 */
static void print_usage(const char *program_name)
{
    OSAL_Printf("\nEMS Test Runner (Busybox-style)\n");
    OSAL_Printf("================================\n\n");

    if (0 == OSAL_Strcmp(program_name, "ems-test")) {
        OSAL_Printf("Usage: %s [options]\n\n", program_name);
        OSAL_Printf("Options:\n");
        OSAL_Printf("  -a, --all              Run all tests\n");
        OSAL_Printf("  -L <layer>             Run tests from specific layer (OSAL, HAL, PCL, PDL)\n");
        OSAL_Printf("  -m <module>            Run tests from specific module\n");
        OSAL_Printf("  -s <suite>             Run specific test suite\n");
        OSAL_Printf("  -l, --list             List all available tests\n");
        OSAL_Printf("  -i, --interactive      Interactive menu (default)\n");
        OSAL_Printf("  -h, --help             Show this help message\n\n");
        OSAL_Printf("Examples:\n");
        OSAL_Printf("  %s                     # Interactive menu\n", program_name);
        OSAL_Printf("  %s -a                  # Run all tests\n", program_name);
        OSAL_Printf("  %s -L OSAL             # Run all OSAL tests\n", program_name);
        OSAL_Printf("  %s -m test_osal_mutex  # Run specific module\n", program_name);
        OSAL_Printf("  %s -s osal_mutex       # Run specific suite\n\n", program_name);
        OSAL_Printf("Symlink Usage:\n");
        OSAL_Printf("  osal-test --all        # Run all OSAL tests\n");
        OSAL_Printf("  hal-test --list        # List all HAL tests\n");
        OSAL_Printf("  pdl-test -s pdl_mcu    # Run PDL MCU suite\n\n");
    } else {
        /* Invoked via symlink */
        const char *layer = detect_layer_filter(program_name);
        OSAL_Printf("Usage: %s [options]\n\n", program_name);
        OSAL_Printf("This is a symlink to ems-test, filtered for %s layer tests.\n\n", layer);
        OSAL_Printf("Options:\n");
        OSAL_Printf("  -a, --all              Run all %s tests\n", layer);
        OSAL_Printf("  -s <suite>             Run specific test suite\n");
        OSAL_Printf("  -l, --list             List all %s tests\n", layer);
        OSAL_Printf("  -i, --interactive      Interactive menu\n");
        OSAL_Printf("  -h, --help             Show this help message\n\n");
        OSAL_Printf("Examples:\n");
        OSAL_Printf("  %s                     # Run all %s tests\n", program_name, layer);
        OSAL_Printf("  %s --list              # List %s tests\n", program_name, layer);
        OSAL_Printf("  %s -s <suite>          # Run specific suite\n\n", program_name);
    }
}

int main(int argc, char *argv[])
{
    const char *program_name = get_program_name(argv[0]);
    const char *layer_filter = detect_layer_filter(program_name);

    /* Parse command line arguments */
    if (1 == argc) {
        /* No arguments */
        if (layer_filter) {
            /* Symlink invocation: run all tests for that layer */
            OSAL_Printf("Running all %s tests...\n\n", layer_filter);
            return libutest_run_layer(layer_filter);
        } else {
            /* Direct invocation: interactive menu */
            return libutest_interactive_menu();
        }
    }

    /* Check for -p flag (print test names) */
    bool print_mode = false;
    if (argc >= 2 && 0 == OSAL_Strcmp(argv[argc - 1], "-p")) {
        print_mode = true;
        argc--;  /* Remove -p from argument count */
    }

    /* Run all tests */
    if (0 == OSAL_Strcmp(argv[1], "-a") || 0 == OSAL_Strcmp(argv[1], "--all")) {
        if (print_mode) {
            if (layer_filter) {
                libutest_print_layer(layer_filter);
            } else {
                libutest_print_all();
            }
            return 0;
        }
        if (layer_filter) {
            return libutest_run_layer(layer_filter);
        } else {
            return libutest_run_all();
        }
    }

    /* Run tests by layer (only for ems-test) */
    if (0 == OSAL_Strcmp(argv[1], "-L") && argc >= 3) {
        if (layer_filter) {
            OSAL_Printf("Warning: -L option ignored when invoked via %s\n", program_name);
            OSAL_Printf("Running %s tests instead.\n\n", layer_filter);
            return libutest_run_layer(layer_filter);
        }
        if (print_mode) {
            libutest_print_layer(argv[2]);
            return 0;
        }
        return libutest_run_layer(argv[2]);
    }

    /* Run tests by module */
    if (0 == OSAL_Strcmp(argv[1], "-m") && argc >= 3) {
        if (print_mode) {
            libutest_print_module(argv[2]);
            return 0;
        }
        return libutest_run_module(argv[2]);
    }

    /* Run specific test suite */
    if (0 == OSAL_Strcmp(argv[1], "-s") && argc >= 3) {
        return libutest_run_suite(argv[2]);
    }

    /* List all tests */
    if (0 == OSAL_Strcmp(argv[1], "-l") || 0 == OSAL_Strcmp(argv[1], "--list")) {
        if (layer_filter) {
            libutest_print_layer(layer_filter);
        } else {
            libutest_list_all();
        }
        return 0;
    }

    /* Interactive menu */
    if (0 == OSAL_Strcmp(argv[1], "-i") || 0 == OSAL_Strcmp(argv[1], "--interactive")) {
        return libutest_interactive_menu();
    }

    /* Help */
    if (0 == OSAL_Strcmp(argv[1], "-h") || 0 == OSAL_Strcmp(argv[1], "--help")) {
        print_usage(program_name);
        return 0;
    }

    /* Unknown option */
    OSAL_Printf("Unknown option: %s\n", argv[1]);
    OSAL_Printf("Use -h or --help for usage information\n");
    return 1;
}
