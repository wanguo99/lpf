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
	char *mcu_mapper;
	char *led_mapper;
	int failures = 0;

	runtime_config = read_source_file(
		"kernel/lpf-runtime/runtime/lpf_runtime_config.c");
	mcu_mapper = read_source_file(
		"kernel/lpf-runtime/peripheral/mcu/lpf_mcu_config_mapper.c");
	led_mapper = read_source_file(
		"kernel/lpf-runtime/peripheral/led/lpf_led_config_mapper.c");

	if (!runtime_config || !mcu_mapper || !led_mapper) {
		fprintf(stderr, "failed to read runtime config mapper sources\n");
		failures = 1;
		goto out;
	}

	failures += expect_contains("lpf_runtime_config.c", runtime_config,
				    "lpf_runtime_config_mapper_first");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"LPF_CONFIG_DEVICE_TYPE_MCU");
	failures += expect_not_contains("lpf_runtime_config.c", runtime_config,
					"LPF_CONFIG_DEVICE_TYPE_LED");
	failures += expect_contains("lpf_mcu_config_mapper.c", mcu_mapper,
				    "lpf_runtime_config_mapper_register");
	failures += expect_contains("lpf_led_config_mapper.c", led_mapper,
				    "lpf_runtime_config_mapper_register");

out:
	free(led_mapper);
	free(mcu_mapper);
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
	char *core_source;
	char *runtime_source;
	int failures = 0;

	core_header = read_source_file("kernel/include/lpf/core/lpf_core.h");
	core_source = read_source_file("kernel/lpf-core/core/lpf_core.c");
	runtime_source = read_source_file(
		"kernel/lpf-runtime/runtime/lpf_runtime.c");

	if (!core_header || !core_source || !runtime_source) {
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

out:
	free(runtime_source);
	free(core_source);
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

int main(void)
{
	int ret = 0;

	ret += test_service_context_registries();
	ret += test_mcu_transport_is_service_owned();
	ret += test_hw_sources_are_capability_grouped();
	ret += test_peripheral_layer_dependencies();
	ret += test_uapi_headers_are_abi_only();
	ret += test_soc_adapter_header_dependencies();
	ret += test_runtime_config_mapping_is_registered();
	ret += test_runtime_entries_are_classed();
	ret += test_core_lifecycle_is_module_owned();
	ret += test_instance_devnode_mode_policy();

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
