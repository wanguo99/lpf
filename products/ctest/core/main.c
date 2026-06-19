/**
 * @file main.c
 * @brief Unified test runner - Busybox-style main entry point
 *
 * Supports both direct invocation (es-middleware-test) and symlink invocation
 * (osal-test, hal-test, pconfig-test, pdi-test)
 */

#include <test_framework/test_framework.h>
#include "osal.h"

extern void ctest_register_all_suites(void);

/**
 * Extract program name from argv[0] (handles both full path and basename)
 */
static const char *_get_program_name(const char *argv0)
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
static const char *_detect_layer_filter(const char *program_name)
{
	if (0 == osal_strcmp(program_name, "osal-test")) {
		return "OSAL";
	} else if (0 == osal_strcmp(program_name, "hal-test")) {
		return "HAL";
	} else if (0 == osal_strcmp(program_name, "pconfig-test")) {
		return "PCONFIG";
	} else if (0 == osal_strcmp(program_name, "pdi-test")) {
		return "PDI";
	}
	return NULL; /* es-middleware-test: no filter */
}

/**
 * Print usage information
 */
static void _print_usage(const char *program_name)
{
	osal_printf("\nES-Middleware Test Runner\n");
	osal_printf("==========================\n\n");

	if (0 == osal_strcmp(program_name, "es-middleware-test")) {
		osal_printf("USAGE:\n");
		osal_printf("    %s [OPTIONS] [FILTERS]\n\n", program_name);

		osal_printf("RUN OPTIONS:\n");
		osal_printf("    -a, --all                  Run all tests\n");
		osal_printf(
			"    -L, --layer <name>         Run tests from specific layer\n");
		osal_printf(
			"    -m, --module <name>        Run tests from specific module\n");
		osal_printf("    -s, --suite <name>         Run specific test suite\n");
		osal_printf("    -i, --interactive          Interactive menu mode "
					"(default)\n\n");

		osal_printf("LIST OPTIONS:\n");
		osal_printf(
			"    -l, --list                 List all tests with details\n");
		osal_printf(
			"        --list-layers          List all available layers\n");
		osal_printf(
			"        --list-modules         List all available modules\n");
		osal_printf("        --list-suites          List all available test "
					"suites\n\n");

		osal_printf("FILTER OPTIONS:\n");
		osal_printf("    -L, --layer <name>         Filter by layer (OSAL, "
					"HAL, PDI, PDM, etc.)\n");
		osal_printf("    -M, --module <name>        Filter by module name\n");
		osal_printf("        --category <cat>       Filter by category:\n");
		osal_printf("                               unit, performance, stress, "
					"system\n");
		osal_printf(
			"        --fast                 Run only fast tests (<100ms)\n");
		osal_printf(
			"        --slow                 Run only slow tests (>1s)\n");
		osal_printf("        --tags <tags>          Filter by tags "
					"(comma-separated)\n");
		osal_printf("        --exclude-hardware     Exclude tests requiring "
					"hardware\n");
		osal_printf("        --exclude-network      Exclude tests requiring "
					"network\n\n");

		osal_printf("OUTPUT OPTIONS:\n");
		osal_printf("    -v, --verbose              Verbose output (show all "
					"assertions)\n");
		osal_printf(
			"    -q, --quiet                Quiet mode (errors only)\n");
		osal_printf("        --format <fmt>         Output format: text, "
					"junit, json\n");
		osal_printf("        --output <file>        Write output to file\n");
		osal_printf(
			"        --no-stats             Disable statistics summary\n");
		osal_printf("        --color <when>         Colorize output: auto, "
					"always, never\n\n");

		osal_printf("OTHER OPTIONS:\n");
		osal_printf("    -h, --help                 Show this help message\n");
		osal_printf(
			"        --version              Show version information\n\n");

		osal_printf("EXAMPLES:\n");
		osal_printf("    # Run all tests\n");
		osal_printf("    %s --all\n\n", program_name);

		osal_printf("    # Run tests from specific layer\n");
		osal_printf("    %s --layer OSAL\n", program_name);
		osal_printf("    %s -L HAL --fast\n\n", program_name);

		osal_printf("    # Run specific module or suite\n");
		osal_printf("    %s --module osal_mutex\n", program_name);
		osal_printf("    %s --suite osal_mutex\n\n", program_name);

		osal_printf("    # List tests structure\n");
		osal_printf("    %s --list-layers\n", program_name);
		osal_printf("    %s --list-modules --layer OSAL\n", program_name);
		osal_printf("    %s --list-suites --module osal_mutex\n", program_name);
		osal_printf("    %s --list --layer OSAL\n\n", program_name);

		osal_printf("    # Filter tests\n");
		osal_printf("    %s --all --category unit\n", program_name);
		osal_printf("    %s --all --fast --exclude-hardware\n", program_name);
		osal_printf("    %s --all --tags fast,memory\n\n", program_name);

		osal_printf("    # CI/CD integration\n");
		osal_printf("    %s --all --format junit --output report.xml\n",
					program_name);
		osal_printf("    %s --all --format json --output results.json\n\n",
					program_name);

		osal_printf("TEST STRUCTURE:\n");
		osal_printf("    Layer → Module → Suite → Test Case\n");
		osal_printf("    Example: OSAL → osal_mutex → osal_mutex → "
					"test_mutex_init_success\n\n");

		osal_printf("SYMLINKS:\n");
		osal_printf("    osal-test, hal-test, pdi-test\n");
		osal_printf(
			"    These symlinks automatically filter tests by layer.\n\n");

		osal_printf("For more information, see: docs/TEST_FRAMEWORK.md\n\n");
	} else {
		/* Invoked via symlink */
		const char *layer = _detect_layer_filter(program_name);
		osal_printf("USAGE:\n");
		osal_printf("    %s [OPTIONS]\n\n", program_name);
		osal_printf("This symlink filters tests for the %s layer.\n\n", layer);

		osal_printf("OPTIONS:\n");
		osal_printf("    -a, --all              Run all %s tests\n", layer);
		osal_printf("    -s, --suite <name>     Run specific test suite\n");
		osal_printf("    -l, --list             List all %s tests\n", layer);
		osal_printf("        --list-modules     List all %s modules\n", layer);
		osal_printf("        --list-suites      List all %s suites\n", layer);
		osal_printf("    -i, --interactive      Interactive menu\n");
		osal_printf("    -h, --help             Show this help message\n\n");

		osal_printf("FILTER OPTIONS:\n");
		osal_printf("        --fast             Run only fast tests\n");
		osal_printf("        --exclude-hardware Exclude hardware tests\n\n");

		osal_printf("OUTPUT OPTIONS:\n");
		osal_printf(
			"        --format <fmt>     Output format: text, junit, json\n");
		osal_printf("        --output <file>    Write output to file\n\n");

		osal_printf("EXAMPLES:\n");
		osal_printf("    %s --all\n", program_name);
		osal_printf("    %s --list-modules\n", program_name);
		osal_printf("    %s --suite osal_mutex\n", program_name);
		osal_printf("    %s --all --fast\n\n", program_name);
	}
}

/**
 * Parse category string to enum
 */
static test_category_t _parse_category(const char *str)
{
	if (0 == osal_strcmp(str, "unit")) {
		return TEST_CATEGORY_UNIT;
	} else if (0 == osal_strcmp(str, "performance") ||
			   0 == osal_strcmp(str, "perf")) {
		return TEST_CATEGORY_PERFORMANCE;
	} else if (0 == osal_strcmp(str, "stress")) {
		return TEST_CATEGORY_STRESS;
	} else if (0 == osal_strcmp(str, "system")) {
		return TEST_CATEGORY_SYSTEM;
	}
	return TEST_CATEGORY_MAX; /* Invalid */
}

/**
 * Parse tags string (comma-separated) to bitmask
 */
static uint32_t _parse_tags(const char *str)
{
	uint32_t tags = 0;
	char buffer[256];
	osal_strncpy(buffer, str, OSAL_sizeof(buffer));
	buffer[OSAL_sizeof(buffer) - 1] = '\0';

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

		if (0 == osal_strcmp(token, "fast")) {
			tags |= TEST_TAG_FAST;
		} else if (0 == osal_strcmp(token, "slow")) {
			tags |= TEST_TAG_SLOW;
		} else if (0 == osal_strcmp(token, "hardware")) {
			tags |= TEST_TAG_HARDWARE;
		} else if (0 == osal_strcmp(token, "network")) {
			tags |= TEST_TAG_NETWORK;
		} else if (0 == osal_strcmp(token, "filesystem")) {
			tags |= TEST_TAG_FILESYSTEM;
		} else if (0 == osal_strcmp(token, "memory")) {
			tags |= TEST_TAG_MEMORY_INTENSIVE;
		} else if (0 == osal_strcmp(token, "cpu")) {
			tags |= TEST_TAG_CPU_INTENSIVE;
		} else if (0 == osal_strcmp(token, "privileged")) {
			tags |= TEST_TAG_PRIVILEGED;
		}

		if (!has_more)
			break;
		token = next;
	}

	return tags;
}

/**
 * Export test results after running tests
 */
static void _export_test_results(const char *format, const char *output_file)
{
	if (!format || !output_file)
		return;

	if (0 == osal_strcmp(format, "junit")) {
		libutest_export_junit_xml(output_file);
	} else if (0 == osal_strcmp(format, "json")) {
		libutest_export_json(output_file);
	} else if (0 != osal_strcmp(format, "text")) {
		osal_printf("Warning: Unknown output format '%s', using text\n",
					format);
	}
}

int main(int argc, char *argv[])
{
	const char *program_name = _get_program_name(argv[0]);
	const char *layer_filter = _detect_layer_filter(program_name);

	ctest_register_all_suites();

	/* Initialize filter */
	test_filter_t filter = { .category_mask = 0,
							 .include_tags = 0,
							 .exclude_tags = 0,
							 .max_timeout_ms = 0,
							 .enabled = false };

	/* Output format configuration */
	const char *output_format = "text"; /* Default: text */
	const char *output_file = NULL;

	/* Parse filtering options first */
	int i;
	for (i = 1; i < argc; i++) {
		if (0 == osal_strcmp(argv[i], "--fast")) {
			filter.include_tags |= TEST_TAG_FAST;
			filter.enabled = true;
		} else if (0 == osal_strcmp(argv[i], "--slow")) {
			filter.include_tags |= TEST_TAG_SLOW;
			filter.enabled = true;
		} else if (0 == osal_strcmp(argv[i], "--exclude-hardware")) {
			filter.exclude_tags |= TEST_TAG_HARDWARE;
			filter.enabled = true;
		} else if (0 == osal_strcmp(argv[i], "--exclude-network")) {
			filter.exclude_tags |= TEST_TAG_NETWORK;
			filter.enabled = true;
		} else if (0 == osal_strcmp(argv[i], "--category") && i + 1 < argc) {
			test_category_t cat = _parse_category(argv[i + 1]);
			if (cat != TEST_CATEGORY_MAX) {
				filter.category_mask = TEST_CATEGORY_MASK(cat);
				filter.enabled = true;
			} else {
				osal_printf("Error: Invalid category '%s'\n", argv[i + 1]);
				return 1;
			}
			i++; /* Skip category value */
		} else if (0 == osal_strcmp(argv[i], "--tags") && i + 1 < argc) {
			filter.include_tags |= _parse_tags(argv[i + 1]);
			filter.enabled = true;
			i++; /* Skip tags value */
		} else if (0 == osal_strcmp(argv[i], "--format") && i + 1 < argc) {
			output_format = argv[i + 1];
			i++; /* Skip format value */
		} else if (0 == osal_strcmp(argv[i], "--output") && i + 1 < argc) {
			output_file = argv[i + 1];
			i++; /* Skip output file value */
		}
	}

	/* Parse command line arguments */
	if (1 == argc) {
		int32_t result;

		/* No arguments */
		if (layer_filter) {
			/* Symlink invocation: run all tests for that layer */
			osal_printf("Running all %s tests...\n\n", layer_filter);
			if (filter.enabled) {
				result = libutest_run_layer_filtered(layer_filter, &filter);
			} else {
				result = libutest_run_layer(layer_filter);
			}

			/* Export results if requested */
			_export_test_results(output_format, output_file);

			return result;
		} else {
			/* Direct invocation: interactive menu */
			return libutest_interactive_menu_filtered(&filter);
		}
	}

	/* Check for -p flag (print test names) */
	bool print_mode = false;
	if (argc >= 2 && 0 == osal_strcmp(argv[argc - 1], "-p")) {
		print_mode = true;
		argc--; /* Remove -p from argument count */
	}

	/* Run all tests */
	if (0 == osal_strcmp(argv[1], "-a") || 0 == osal_strcmp(argv[1], "--all")) {
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
		_export_test_results(output_format, output_file);

		return result;
	}

	/* Run tests by layer (only for es-middleware-test) */
	if ((0 == osal_strcmp(argv[1], "-L") ||
		 0 == osal_strcmp(argv[1], "--layer")) &&
		argc >= 3) {
		int32_t result;

		if (layer_filter) {
			osal_printf("Warning: --layer option ignored when invoked via %s\n",
						program_name);
			osal_printf("Running %s tests instead.\n\n", layer_filter);
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
		_export_test_results(output_format, output_file);

		return result;
	}

	/* Run tests by module */
	if ((0 == osal_strcmp(argv[1], "-m") ||
		 0 == osal_strcmp(argv[1], "--module")) &&
		argc >= 3) {
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
		_export_test_results(output_format, output_file);

		return result;
	}

	/* Run specific test suite */
	if ((0 == osal_strcmp(argv[1], "-s") ||
		 0 == osal_strcmp(argv[1], "--suite")) &&
		argc >= 3) {
		int32_t result = libutest_run_suite(argv[2]);

		/* Export results if requested */
		_export_test_results(output_format, output_file);

		return result;
	}

	/* List all tests */
	if (0 == osal_strcmp(argv[1], "-l") ||
		0 == osal_strcmp(argv[1], "--list")) {
		/* Check for --layers option */
		if (argc >= 3 && 0 == osal_strcmp(argv[2], "--layers")) {
			libutest_list_layers();
		}
		/* Check for --modules option */
		else if (argc >= 3 && 0 == osal_strcmp(argv[2], "--modules")) {
			/* Check if layer filter follows */
			if (argc >= 5 && 0 == osal_strcmp(argv[3], "-L")) {
				libutest_list_modules(argv[4]);
			} else {
				libutest_list_modules(NULL);
			}
		}
		/* Check for --suites option */
		else if (argc >= 3 && 0 == osal_strcmp(argv[2], "--suites")) {
			const char *layer = NULL;
			const char *module = NULL;
			/* Check for -L or -M filter */
			if (argc >= 5 && 0 == osal_strcmp(argv[3], "-L")) {
				layer = argv[4];
			} else if (argc >= 5 && 0 == osal_strcmp(argv[3], "-M")) {
				module = argv[4];
			}
			libutest_list_suites(layer, module);
		}
		/* Check for -L parameter after --list */
		else if (argc >= 4 && 0 == osal_strcmp(argv[2], "-L")) {
			libutest_list_layer(argv[3]);
		}
		/* Check for -m parameter after --list */
		else if (argc >= 4 && 0 == osal_strcmp(argv[2], "-m")) {
			libutest_list_module(argv[3]);
		} else if (layer_filter) {
			libutest_print_layer(layer_filter);
		} else {
			libutest_list_all();
		}
		return 0;
	}

	/* Standalone --list-layers */
	if (0 == osal_strcmp(argv[1], "--list-layers")) {
		libutest_list_layers();
		return 0;
	}

	/* Standalone --list-modules */
	if (0 == osal_strcmp(argv[1], "--list-modules")) {
		const char *layer = NULL;
		/* Check for --layer filter */
		if (argc >= 4 && (0 == osal_strcmp(argv[2], "-L") ||
						  0 == osal_strcmp(argv[2], "--layer"))) {
			layer = argv[3];
		}
		libutest_list_modules(layer);
		return 0;
	}

	/* Standalone --list-suites */
	if (0 == osal_strcmp(argv[1], "--list-suites")) {
		const char *layer = NULL;
		const char *module = NULL;
		/* Check for --layer or --module filter */
		if (argc >= 4) {
			if (0 == osal_strcmp(argv[2], "-L") ||
				0 == osal_strcmp(argv[2], "--layer")) {
				layer = argv[3];
			} else if (0 == osal_strcmp(argv[2], "-M") ||
					   0 == osal_strcmp(argv[2], "--module")) {
				module = argv[3];
			}
		}
		libutest_list_suites(layer, module);
		return 0;
	}

	/* Interactive menu */
	if (0 == osal_strcmp(argv[1], "-i") ||
		0 == osal_strcmp(argv[1], "--interactive")) {
		return libutest_interactive_menu_filtered(&filter);
	}

	/* Help */
	if (0 == osal_strcmp(argv[1], "-h") ||
		0 == osal_strcmp(argv[1], "--help")) {
		_print_usage(program_name);
		return 0;
	}

	/* Version */
	if (0 == osal_strcmp(argv[1], "--version")) {
		osal_printf("ES-Middleware Test Runner\n");
		osal_printf("Version: %s\n", osal_get_version());
		osal_printf("Build: %s %s\n", __DATE__, __TIME__);
		return 0;
	}

	/* Unknown option */
	osal_printf("Unknown option: %s\n", argv[1]);
	osal_printf("Use -h or --help for usage information\n");
	return 1;
}
