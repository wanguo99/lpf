/**
 * @file main.c
 * @brief Unified test runner - main entry point
 *
 * Aggregates all module tests and provides CLI interface
 */

#include "tests_core.h"
#include "osal.h"

int main(int argc, char *argv[])
{
    /* Parse command line arguments */
    if (1 == argc) {
        /* No arguments - interactive menu */
        return libutest_interactive_menu();
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
            libutest_print_all();
            return 0;
        }
        return libutest_run_all();
    }

    /* Run tests by layer */
    if (0 == OSAL_Strcmp(argv[1], "-L") && argc >= 3) {
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
        libutest_list_all();
        return 0;
    }

    /* Interactive menu (default) */
    if (0 == OSAL_Strcmp(argv[1], "-i") || 0 == OSAL_Strcmp(argv[1], "--interactive")) {
        return libutest_interactive_menu();
    }

    /* Help */
    if (0 == OSAL_Strcmp(argv[1], "-h") || 0 == OSAL_Strcmp(argv[1], "--help")) {
        OSAL_Printf("\nEMS Unit Test Runner\n");
        OSAL_Printf("========================\n\n");
        OSAL_Printf("Usage: %s [options]\n\n", argv[0]);
        OSAL_Printf("Options:\n");
        OSAL_Printf("  -a, --all              Run all tests\n");
        OSAL_Printf("  -L <layer>             Run tests from specific layer (OSAL, HAL, PDL)\n");
        OSAL_Printf("  -m <module>            Run tests from specific module\n");
        OSAL_Printf("  -s <suite>             Run specific test suite\n");
        OSAL_Printf("  -l, --list             List all available tests\n");
        OSAL_Printf("  -p                     Print test case names (use with -a, -L, -m)\n");
        OSAL_Printf("  -i, --interactive      Interactive menu (default)\n");
        OSAL_Printf("  -h, --help             Show this help message\n\n");
        OSAL_Printf("Examples:\n");
        OSAL_Printf("  %s                     # Interactive menu\n", argv[0]);
        OSAL_Printf("  %s -a                  # Run all tests\n", argv[0]);
        OSAL_Printf("  %s -L OSAL             # Run all OSAL tests\n", argv[0]);
        OSAL_Printf("  %s -m osal             # Run all osal module tests\n", argv[0]);
        OSAL_Printf("  %s -s osal_task        # Run osal_task test suite\n", argv[0]);
        OSAL_Printf("  %s -l                  # List all tests\n", argv[0]);
        OSAL_Printf("  %s -a -p               # Print all test case names\n", argv[0]);
        OSAL_Printf("  %s -L OSAL -p          # Print OSAL test case names\n", argv[0]);
        OSAL_Printf("  %s -m pdl_mcu -p       # Print pdl_mcu test case names\n\n", argv[0]);
        return 0;
    }

    /* Unknown option */
    OSAL_Printf("Unknown option: %s\n", argv[1]);
    OSAL_Printf("Use -h or --help for usage information\n");
    return 1;
}
