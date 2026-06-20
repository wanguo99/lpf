// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef SDK_SOURCE_DIR
#define SDK_SOURCE_DIR "."
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef struct {
	const char *name;
	const char *path;
} source_file_t;

static char *read_source_file(const char *relative_path)
{
	char path[1024];
	char *buffer;
	FILE *file;
	long size;
	size_t read_size;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/%s", SDK_SOURCE_DIR,
		       relative_path);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return NULL;

	file = fopen(path, "rb");
	if (!file)
		return NULL;

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}

	size = ftell(file);
	if (size < 0) {
		fclose(file);
		return NULL;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}

	buffer = malloc((size_t)size + 1U);
	if (!buffer) {
		fclose(file);
		return NULL;
	}

	read_size = fread(buffer, 1, (size_t)size, file);
	fclose(file);
	if (read_size != (size_t)size) {
		free(buffer);
		return NULL;
	}

	buffer[size] = '\0';
	return buffer;
}

static int expect_contains(const char *file_name, const char *content,
			   const char *needle)
{
	if (strstr(content, needle))
		return 0;

	fprintf(stderr, "%s: missing required token: %s\n", file_name,
		needle);
	return 1;
}

static int expect_not_contains(const char *file_name, const char *content,
			       const char *needle)
{
	if (!strstr(content, needle))
		return 0;

	fprintf(stderr, "%s: forbidden token present: %s\n", file_name,
		needle);
	return 1;
}

static int expect_ordered_contains(const char *file_name, const char *content,
				   const char *first, const char *second)
{
	const char *first_pos;
	const char *second_pos;

	first_pos = strstr(content, first);
	second_pos = strstr(content, second);
	if (first_pos && second_pos && first_pos < second_pos)
		return 0;

	fprintf(stderr, "%s: expected %s before %s\n", file_name, first,
		second);
	return 1;
}

static int expect_path_absent(const char *relative_path)
{
	char path[1024];
	struct stat st;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/%s", SDK_SOURCE_DIR,
		       relative_path);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return 1;

	if (stat(path, &st) != 0)
		return errno == ENOENT ? 0 : 1;

	fprintf(stderr, "forbidden path present: %s\n", relative_path);
	return 1;
}

static int expect_path_present(const char *relative_path)
{
	char path[1024];
	struct stat st;
	int ret;

	ret = snprintf(path, sizeof(path), "%s/%s", SDK_SOURCE_DIR,
		       relative_path);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return 1;

	if (stat(path, &st) == 0)
		return 0;

	fprintf(stderr, "required path missing: %s\n", relative_path);
	return 1;
}

static int expect_range_not_contains(const char *file_name,
				     const char *content,
				     const char *start_token,
				     const char *end_token,
				     const char *needle)
{
	const char *start;
	const char *end;
	const char *found;

	start = strstr(content, start_token);
	if (!start) {
		fprintf(stderr, "%s: missing range start: %s\n", file_name,
			start_token);
		return 1;
	}

	end = strstr(start, end_token);
	if (!end) {
		fprintf(stderr, "%s: missing range end: %s\n", file_name,
			end_token);
		return 1;
	}

	found = strstr(start, needle);
	if (!found || found >= end)
		return 0;

	fprintf(stderr, "%s: forbidden token present in range %s: %s\n",
		file_name, start_token, needle);
	return 1;
}

static int test_service_context_registries(void)
{
	char *mcu_service;
	char *led_service;
	char *mcu_chrdev;
	char *led_chrdev;
	int failures = 0;

	mcu_service = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_service.c");
	led_service = read_source_file(
		"kernel/lpf-runtime/peripheral/led/lpf_led_service.c");
	mcu_chrdev = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_chrdev.c");
	led_chrdev = read_source_file(
		"kernel/lpf-runtime/peripheral/led/lpf_led_chrdev.c");

	if (!mcu_service || !led_service || !mcu_chrdev || !led_chrdev) {
		fprintf(stderr, "failed to read peripheral service sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_contains(
		"lpf_mcu_service.c", mcu_service,
		"struct lpf_mcu_context *next;");
	failures += expect_contains(
		"lpf_mcu_service.c", mcu_service,
		"static lpf_mcu_context_t *g_mcu_context_list;");
	failures += expect_contains("lpf_mcu_service.c", mcu_service,
				    "lpf_mcu_add_context_locked");
	failures += expect_contains("lpf_mcu_service.c", mcu_service,
				    "lpf_mcu_remove_context_locked");
	failures += expect_not_contains("lpf_mcu_service.c", mcu_service,
					"[LPF_MCU_MAX_DEVICES]");
	failures += expect_not_contains("lpf_mcu_service.c", mcu_service,
					"LPF_MCU_MAX_DEVICES");

	failures += expect_contains(
		"lpf_led_service.c", led_service,
		"struct lpf_led_context *next;");
	failures += expect_contains(
		"lpf_led_service.c", led_service,
		"static lpf_led_context_t *g_led_context_list;");
	failures += expect_contains("lpf_led_service.c", led_service,
				    "lpf_led_add_context_locked");
	failures += expect_contains("lpf_led_service.c", led_service,
				    "lpf_led_remove_context_locked");
	failures += expect_not_contains("lpf_led_service.c", led_service,
					"[LPF_LED_MAX_DEVICES]");
	failures += expect_not_contains("lpf_led_service.c", led_service,
					"LPF_LED_MAX_DEVICES");

	failures += expect_contains(
		"lpf_mcu_chrdev.c", mcu_chrdev,
		"static lpf_chrdev_t g_lpf_mcu_chrdevs[LPF_MCU_MAX_DEVICES];");
	failures += expect_contains(
		"lpf_led_chrdev.c", led_chrdev,
		"static lpf_chrdev_t g_lpf_led_chrdevs[LPF_LED_MAX_DEVICES];");

out:
	free(led_chrdev);
	free(mcu_chrdev);
	free(led_service);
	free(mcu_service);
	return failures ? 1 : 0;
}

static int test_mcu_transport_is_service_owned(void)
{
	char *mcu_internal;
	char *mcu_makefile;
	int failures = 0;

	mcu_internal = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_internal.h");
	mcu_makefile = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/Makefile");

	if (!mcu_internal || !mcu_makefile) {
		fprintf(stderr, "failed to read MCU transport ownership sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_path_absent("kernel/lpf-runtime/transport/mcu");
	failures += expect_path_absent("kernel/include/lpf/transport/mcu");
	failures += expect_contains("lpf_mcu_internal.h", mcu_internal,
				    "#include \"lpf_mcu_transport.h\"");
	failures += expect_not_contains("lpf_mcu_internal.h", mcu_internal,
					"lpf/transport/mcu");
	failures += expect_contains("mcu/Makefile", mcu_makefile,
				    "lpf-runtime/peripheral/mcu/lpf_mcu_transport.o");
	failures += expect_contains(
		"mcu/Makefile", mcu_makefile,
		"lpf-runtime/peripheral/mcu/lpf_mcu_transport_can.o");
	failures += expect_contains(
		"mcu/Makefile", mcu_makefile,
		"lpf-runtime/peripheral/mcu/lpf_mcu_transport_uart.o");

out:
	free(mcu_makefile);
	free(mcu_internal);
	return failures ? 1 : 0;
}

static int test_hw_sources_are_capability_grouped(void)
{
	char *hw_makefile;
	char *hw_core;
	char *hw_gpio;
	int failures = 0;

	hw_makefile = read_source_file("kernel/lpf-runtime/hw/Makefile");
	hw_core = read_source_file("kernel/lpf-runtime/hw/core/lpf_hw.c");
	hw_gpio = read_source_file("kernel/lpf-runtime/hw/gpio/lpf_hw_gpio.c");

	if (!hw_makefile || !hw_core || !hw_gpio) {
		fprintf(stderr, "failed to read HW grouping sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_path_present("kernel/lpf-runtime/include/lpf_hw_internal.h");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw_internal.h");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw.c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw_gpio.c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw_pwm.c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw_bus_i2c.c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/lpf_hw_bus_spi.c");
	failures += expect_path_absent(
		"kernel/lpf-runtime/hw/lpf_hw_transport_can.c");
	failures += expect_path_absent(
		"kernel/lpf-runtime/hw/lpf_hw_transport_uart.c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/bus/i2c");
	failures += expect_path_absent("kernel/lpf-runtime/hw/bus/spi");
	failures += expect_path_absent("kernel/lpf-runtime/hw/transport/can");
	failures += expect_path_absent("kernel/lpf-runtime/hw/transport/uart");

	failures += expect_contains("hw/Makefile", hw_makefile,
				    "lpf-runtime/hw/core/lpf_hw.o");
	failures += expect_contains("hw/Makefile", hw_makefile,
				    "lpf-runtime/hw/gpio/lpf_hw_gpio.o");
	failures += expect_contains("hw/Makefile", hw_makefile,
				    "lpf-runtime/hw/pwm/lpf_hw_pwm.o");
	failures += expect_contains("hw/Makefile", hw_makefile,
				    "lpf-runtime/hw/i2c/lpf_hw_bus_i2c.o");
	failures += expect_contains("hw/Makefile", hw_makefile,
				    "lpf-runtime/hw/spi/lpf_hw_bus_spi.o");
	failures += expect_contains(
		"hw/Makefile", hw_makefile,
		"lpf-runtime/hw/can/lpf_hw_transport_can.o");
	failures += expect_contains(
		"hw/Makefile", hw_makefile,
		"lpf-runtime/hw/uart/lpf_hw_transport_uart.o");

	failures += expect_contains("lpf_hw.c", hw_core,
				    "#include \"lpf_hw_internal.h\"");
	failures += expect_contains("lpf_hw_gpio.c", hw_gpio,
				    "#include \"lpf_hw_internal.h\"");

out:
	free(hw_gpio);
	free(hw_core);
	free(hw_makefile);
	return failures ? 1 : 0;
}

static int test_static_config_sources_are_version_named(void)
{
	char *config_makefile;
	char *config_header;
	char *config_backend;
	char *config_static_header;
	char *static_backend;
	char *static_table;
	char *x86_config;
	char *mock_config;
	int failures = 0;

	config_makefile = read_source_file("kernel/lpf-runtime/config/Makefile");
	config_header = read_source_file("kernel/include/lpf/config/lpf_config.h");
	config_backend = read_source_file(
		"kernel/lpf-runtime/config/src/lpf_config_backend.c");
	config_static_header = read_source_file(
		"kernel/lpf-runtime/config/src/lpf_config_static.h");
	static_backend = read_source_file(
		"kernel/lpf-runtime/config/src/lpf_config_static_backend.c");
	static_table = read_source_file(
		"kernel/lpf-runtime/config/configs/lpf_config_configs.c");
	x86_config = read_source_file(
		"kernel/lpf-runtime/config/configs/kernel/x86_modules/lpf_config_kernel_x86_modules_v1.c");
	mock_config = read_source_file(
		"kernel/lpf-runtime/config/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.c");
	if (!config_makefile || !config_header || !config_backend ||
	    !config_static_header || !static_backend || !static_table ||
	    !x86_config || !mock_config) {
		fprintf(stderr, "failed to read static config sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_path_absent(
		"kernel/lpf-runtime/config/configs/kernel/x86_modules/1.0.0");
	failures += expect_path_absent(
		"kernel/lpf-runtime/config/configs/kernel/x86_mock_modules/1.0.0");
	failures += expect_path_present(
		"kernel/lpf-runtime/config/configs/kernel/x86_modules/lpf_config_kernel_x86_modules_v1.c");
	failures += expect_path_present(
		"kernel/lpf-runtime/config/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.c");
	failures += expect_contains(
		"config/Makefile", config_makefile,
		"lpf-runtime/config/configs/kernel/x86_modules/lpf_config_kernel_x86_modules_v1.o");
	failures += expect_contains(
		"config/Makefile", config_makefile,
		"lpf-runtime/config/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.o");
	failures += expect_contains(
		"config/Makefile", config_makefile,
		"lpf-runtime/config/configs/lpf_config_configs.o");
	failures += expect_ordered_contains("lpf_config_backend.c",
					    config_backend,
					    "&g_lpf_config_static_backend",
					    "&g_lpf_config_dt_backend");
	failures += expect_not_contains("config/Makefile", config_makefile,
					"/1.0.0/");
	failures += expect_not_contains("lpf_config.h", config_header,
					"g_lpf_config_platform_table");
	failures += expect_contains("lpf_config_static.h", config_static_header,
				    "lpf_config_static_table_t");
	failures += expect_contains("lpf_config_configs.c", static_table,
				    "g_lpf_config_platform_table");
	failures += expect_contains("lpf_config_static_backend.c", static_backend,
				    "g_lpf_config_platform_table");
	failures += expect_not_contains("lpf_config_kernel_x86_modules_v1.c",
					x86_config,
					"g_lpf_config_platform_table");
	failures += expect_not_contains("lpf_config_kernel_x86_mock_modules_v1.c",
					mock_config,
					"g_lpf_config_platform_table");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    "g_lpf_config_kernel_x86_mock_modules_nodes");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".device_node_count = OSAL_ARRAY_SIZE");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".device_nodes = g_lpf_config_kernel_x86_mock_modules_nodes");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".compatible = \"lpf,mcu\"");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".compatible = \"lpf,led\"");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".mcu_count = 0");
	failures += expect_contains("lpf_config_kernel_x86_mock_modules_v1.c",
				    mock_config,
				    ".led_count = 0");

out:
	free(mock_config);
	free(x86_config);
	free(static_table);
	free(static_backend);
	free(config_static_header);
	free(config_backend);
	free(config_header);
	free(config_makefile);
	return failures ? 1 : 0;
}

static int test_peripheral_layer_dependencies(void)
{
	static const source_file_t files[] = {
		{ "lpf_runtime.c",
		  "kernel/lpf-runtime/runtime/lpf_runtime.c" },
		{ "lpf_runtime_config.c",
		  "kernel/lpf-runtime/runtime/lpf_runtime_config.c" },
		{ "lpf_mcu_service.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_service.c" },
		{ "lpf_mcu_chrdev.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_chrdev.c" },
		{ "lpf_mcu_proc.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_proc.c" },
		{ "lpf_led_service.c",
		  "kernel/lpf-runtime/peripheral/led/lpf_led_service.c" },
		{ "lpf_led_chrdev.c",
		  "kernel/lpf-runtime/peripheral/led/lpf_led_chrdev.c" },
		{ "lpf_led_proc.c",
		  "kernel/lpf-runtime/peripheral/led/lpf_led_proc.c" },
		{ "lpf_mcu_transport.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_transport.c" },
		{ "lpf_mcu_transport_can.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_transport_can.c" },
		{ "lpf_mcu_transport_uart.c",
		  "kernel/lpf-runtime/peripheral/mcu/lpf_mcu_transport_uart.c" },
	};
	static const char *forbidden_tokens[] = {
		"lpf/soc/",
		"lpf/compat/",
		"lpf_soc_",
		"lpf_compat_",
	};
	size_t i;
	size_t j;
	int failures = 0;

	for (i = 0; i < ARRAY_SIZE(files); i++) {
		char *content = read_source_file(files[i].path);

		if (!content) {
			fprintf(stderr, "%s: failed to read source\n",
				files[i].name);
			failures++;
			continue;
		}

		for (j = 0; j < ARRAY_SIZE(forbidden_tokens); j++)
			failures += expect_not_contains(
				files[i].name, content, forbidden_tokens[j]);

		free(content);
	}

	return failures ? 1 : 0;
}

static int test_uapi_headers_are_abi_only(void)
{
	static const source_file_t headers[] = {
		{ "lpf_ctl.h", "uapi/lpf/lpf_ctl.h" },
		{ "lpf_mcu.h", "uapi/lpf/lpf_mcu.h" },
		{ "lpf_led.h", "uapi/lpf/lpf_led.h" },
	};
	static const char *forbidden_tokens[] = {
		"pdi_",
		"PDI_",
		"osal_",
		"OSAL_",
		"<stdio.h>",
		"<stdlib.h>",
		"<unistd.h>",
		"<fcntl.h>",
	};
	size_t i;
	size_t j;
	int failures = 0;

	for (i = 0; i < ARRAY_SIZE(headers); i++) {
		char *content = read_source_file(headers[i].path);

		if (!content) {
			fprintf(stderr, "%s: failed to read header\n",
				headers[i].name);
			failures++;
			continue;
		}

		for (j = 0; j < ARRAY_SIZE(forbidden_tokens); j++)
			failures += expect_not_contains(
				headers[i].name, content, forbidden_tokens[j]);

		free(content);
	}

	return failures ? 1 : 0;
}

static int test_soc_adapter_header_dependencies(void)
{
	char *content;
	int failures = 0;

	content = read_source_file("kernel/include/lpf/soc/lpf_soc_adapter.h");
	if (!content) {
		fprintf(stderr, "lpf_soc_adapter.h: failed to read header\n");
		return 1;
	}

	failures += expect_not_contains("lpf_soc_adapter.h", content,
					"lpf/hw/");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_gpio_types.h");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_pwm_types.h");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_i2c_types.h");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_spi_types.h");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_can_types.h");
	failures += expect_contains("lpf_soc_adapter.h", content,
				    "lpf/types/lpf_serial_types.h");

	free(content);
	return failures ? 1 : 0;
}

static int test_runtime_config_mapping_is_registered(void)
{
	char *runtime_config;
	char *runtime_header;
	char *mcu_driver;
	char *led_driver;
	int failures = 0;

	runtime_config = read_source_file(
		"kernel/lpf-runtime/runtime/lpf_runtime_config.c");
	runtime_header = read_source_file(
		"kernel/lpf-runtime/include/lpf_runtime_internal.h");
	mcu_driver = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_config_driver.c");
	led_driver = read_source_file(
		"kernel/lpf-runtime/peripheral/led/lpf_led_config_driver.c");

	if (!runtime_config || !runtime_header || !mcu_driver || !led_driver) {
		fprintf(stderr, "failed to read runtime config driver sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_contains("lpf_runtime_config.c", runtime_config,
				    "lpf_runtime_config_driver_first");
	failures += expect_contains("lpf_runtime_config.c", runtime_config,
				    "lpf_config_get_device_nodes");
	failures += expect_contains("lpf_runtime_config.c", runtime_config,
				    "driver->compatible");
	failures += expect_contains("lpf_runtime_config.c", runtime_config,
				    "node->compatible");
	failures += expect_contains("lpf_runtime_internal.h", runtime_header,
				    "lpf_runtime_config_compatible_driver_register");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"LPF_CONFIG_DEVICE_TYPE_MCU");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"LPF_CONFIG_DEVICE_TYPE_LED");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"lpf_config_get()");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"mcu_count");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"led_count");
	failures += expect_contains("lpf_mcu_config_driver.c", mcu_driver,
				    "lpf_runtime_config_driver_register");
	failures += expect_contains("lpf_led_config_driver.c", led_driver,
				    "lpf_runtime_config_driver_register");
	failures += expect_contains("lpf_mcu_config_driver.c", mcu_driver,
				    "lpf_device_register");
	failures += expect_contains("lpf_led_config_driver.c", led_driver,
				    "lpf_device_register");
	failures += expect_contains("lpf_mcu_config_driver.c", mcu_driver,
				    "lpf_config_device_node_t");
	failures += expect_contains("lpf_led_config_driver.c", led_driver,
				    "lpf_config_device_node_t");
	failures += expect_not_contains("lpf_mcu_config_driver.c", mcu_driver,
					"lpf_config_hw_get_mcu");
	failures += expect_not_contains("lpf_led_config_driver.c", led_driver,
					"lpf_config_hw_get_led");
	failures += expect_not_contains("lpf_mcu_config_driver.c", mcu_driver,
					"mcu_count");
	failures += expect_not_contains("lpf_led_config_driver.c", led_driver,
					"led_count");

out:
	free(led_driver);
	free(mcu_driver);
	free(runtime_header);
	free(runtime_config);
	return failures ? 1 : 0;
}

static int test_runtime_entries_are_classed(void)
{
	char *runtime_header;
	char *mcu_service;
	char *led_service;
	char *config_selftest;
	int failures = 0;

	runtime_header = read_source_file(
		"kernel/lpf-runtime/include/lpf_runtime_internal.h");
	mcu_service = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_service.c");
	led_service = read_source_file(
		"kernel/lpf-runtime/peripheral/led/lpf_led_service.c");
	config_selftest = read_source_file(
		"kernel/lpf-runtime/config/selftest/lpf_config_of_selftest.c");

	if (!runtime_header || !mcu_service || !led_service || !config_selftest) {
		fprintf(stderr, "failed to read runtime entry sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_contains("lpf_runtime_internal.h", runtime_header,
				    "lpf_runtime_service_register");
	failures += expect_contains("lpf_runtime_internal.h", runtime_header,
				    "lpf_runtime_selftest_register");
	failures += expect_not_contains("lpf_runtime_internal.h", runtime_header,
					"#define lpf_runtime_register");
	failures += expect_contains("lpf_mcu_service.c", mcu_service,
				    "lpf_runtime_service_register");
	failures += expect_contains("lpf_led_service.c", led_service,
				    "lpf_runtime_service_register");
	failures += expect_contains("lpf_config_of_selftest.c", config_selftest,
				    "lpf_runtime_selftest_register");

out:
	free(config_selftest);
	free(led_service);
	free(mcu_service);
	free(runtime_header);
	return failures ? 1 : 0;
}

static int test_core_lifecycle_is_module_owned(void)
{
	char *core_header;
	char *driver_header;
	char *device_header;
	char *core_source;
	char *runtime_source;
	int failures = 0;

	core_header = read_source_file("kernel/include/lpf/core/lpf_core.h");
	driver_header = read_source_file("kernel/include/lpf/core/lpf_driver.h");
	device_header = read_source_file("kernel/include/lpf/core/lpf_device.h");
	core_source = read_source_file("kernel/lpf-core/core/lpf_core.c");
	runtime_source = read_source_file(
		"kernel/lpf-runtime/runtime/lpf_runtime.c");

	if (!core_header || !driver_header || !device_header || !core_source ||
	    !runtime_source) {
		fprintf(stderr, "failed to read core lifecycle sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_not_contains("lpf_core.h", core_header,
					"lpf_core_init");
	failures += expect_not_contains("lpf_core.h", core_header,
					"lpf_core_deinit");
	failures += expect_contains("lpf_core.c", core_source,
				    "static int32_t lpf_core_init(void)");
	failures += expect_contains("lpf_core.c", core_source,
				    "static void lpf_core_deinit(void)");
	failures += expect_not_contains("lpf_core.c", core_source,
					"EXPORT_SYMBOL_GPL(lpf_core_init)");
	failures += expect_not_contains("lpf_core.c", core_source,
					"EXPORT_SYMBOL_GPL(lpf_core_deinit)");
	failures += expect_range_not_contains(
		"lpf_core.c", core_source,
		"int32_t lpf_driver_register(const lpf_driver_t *driver)",
		"EXPORT_SYMBOL_GPL(lpf_driver_register);",
		"lpf_core_init");
	failures += expect_range_not_contains(
		"lpf_core.c", core_source,
		"int32_t lpf_device_register(const lpf_device_config_t *config)",
		"EXPORT_SYMBOL_GPL(lpf_device_register);",
		"lpf_core_init");
	failures += expect_range_not_contains(
		"lpf_core.c", core_source,
		"int32_t lpf_device_event_subscribe",
		"EXPORT_SYMBOL_GPL(lpf_device_event_subscribe);",
		"lpf_core_init");
	failures += expect_not_contains("lpf_runtime.c", runtime_source,
					"lpf_core_init");
	failures += expect_contains("lpf_core.h", core_header,
				    "LPF-owned device model");
	failures += expect_contains("lpf_core.h", core_header,
				    "matches devices to drivers by LPF device type");
	failures += expect_contains("lpf_driver.h", driver_header,
				    "Match key used by LPF Core");
	failures += expect_contains("lpf_device.h", device_header,
				    "Match key used to find the registered LPF driver");
	failures += expect_contains("lpf_core.c", core_source,
				    "lpf_match_driver_by_type_locked");

out:
	free(runtime_source);
	free(core_source);
	free(device_header);
	free(driver_header);
	free(core_header);
	return failures ? 1 : 0;
}

static int test_instance_devnode_mode_policy(void)
{
	char *core_config;
	char *chrdev_source;
	int failures = 0;

	core_config = read_source_file("kernel/lpf-core/Config.in");
	chrdev_source = read_source_file("kernel/lpf-core/core/lpf_chrdev.c");

	if (!core_config || !chrdev_source) {
		fprintf(stderr, "failed to read instance devnode policy sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_contains("Config.in", core_config,
				    "config LPF_INSTANCE_DEVNODE_MODE");
	failures += expect_contains("Config.in", core_config,
				    "default \"0660\"");
	failures += expect_contains("lpf_chrdev.c", chrdev_source,
				    "CONFIG_LPF_INSTANCE_DEVNODE_MODE");
	failures += expect_contains("lpf_chrdev.c", chrdev_source,
				    "LPF_INSTANCE_DEVNODE_MODE_FALLBACK 0660");
	failures += expect_contains("lpf_chrdev.c", chrdev_source,
				    "lpf_chrdev_parse_mode");
	failures += expect_contains("lpf_chrdev.c", chrdev_source,
				    "lpf_chrdev_instance_mode()");
	failures += expect_contains("lpf_chrdev.c", chrdev_source,
				    "LPF_CTL_DEVNODE_MODE");
	failures += expect_not_contains("lpf_chrdev.c", chrdev_source,
					"chrdev->miscdev.mode = 0666;");

out:
	free(chrdev_source);
	free(core_config);
	return failures ? 1 : 0;
}

static int test_no_separate_bus_layer(void)
{
	char *plan;
	char *peripheral_readme;
	int failures = 0;

	plan = read_source_file("docs/LPF_ARCHITECTURE_REFACTOR_PLAN.md");
	peripheral_readme = read_source_file(
		"kernel/lpf-runtime/peripheral/README.md");
	if (!plan || !peripheral_readme) {
		fprintf(stderr, "failed to read bus-layer policy docs\n");
		failures = 1;
		goto out;
	}

	failures += expect_path_absent("kernel/lpf-core/bus");
	failures += expect_path_absent("kernel/lpf-runtime/bus");
	failures += expect_contains("LPF_ARCHITECTURE_REFACTOR_PLAN.md", plan,
				    "do not create extra");
	failures += expect_contains("peripheral/README.md", peripheral_readme,
				    "does not expose a separate physical bus layer");

out:
	free(peripheral_readme);
	free(plan);
	return failures ? 1 : 0;
}

int main(void)
{
	int ret = 0;

	ret += test_service_context_registries();
	ret += test_mcu_transport_is_service_owned();
	ret += test_hw_sources_are_capability_grouped();
	ret += test_static_config_sources_are_version_named();
	ret += test_peripheral_layer_dependencies();
	ret += test_uapi_headers_are_abi_only();
	ret += test_soc_adapter_header_dependencies();
	ret += test_runtime_config_mapping_is_registered();
	ret += test_runtime_entries_are_classed();
	ret += test_core_lifecycle_is_module_owned();
	ret += test_instance_devnode_mode_policy();
	ret += test_no_separate_bus_layer();

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
