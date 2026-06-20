// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int test_service_context_registries(void)
{
	char *mcu_service;
	char *led_service;
	char *mcu_chrdev;
	char *led_chrdev;
	int failures = 0;

	mcu_service = read_source_file(
		"kernel/lpf/peripheral/mcu/lpf_mcu_service.c");
	led_service = read_source_file(
		"kernel/lpf/peripheral/led/lpf_led_service.c");
	mcu_chrdev = read_source_file(
		"kernel/lpf/peripheral/mcu/lpf_mcu_chrdev.c");
	led_chrdev = read_source_file(
		"kernel/lpf/peripheral/led/lpf_led_chrdev.c");

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

static int test_peripheral_layer_dependencies(void)
{
	static const source_file_t files[] = {
		{ "lpf_peripheral.c",
		  "kernel/lpf/peripheral/lpf_peripheral.c" },
		{ "lpf_peripheral_config.c",
		  "kernel/lpf/peripheral/lpf_peripheral_config.c" },
		{ "lpf_mcu_service.c",
		  "kernel/lpf/peripheral/mcu/lpf_mcu_service.c" },
		{ "lpf_mcu_chrdev.c",
		  "kernel/lpf/peripheral/mcu/lpf_mcu_chrdev.c" },
		{ "lpf_mcu_proc.c",
		  "kernel/lpf/peripheral/mcu/lpf_mcu_proc.c" },
		{ "lpf_led_service.c",
		  "kernel/lpf/peripheral/led/lpf_led_service.c" },
		{ "lpf_led_chrdev.c",
		  "kernel/lpf/peripheral/led/lpf_led_chrdev.c" },
		{ "lpf_led_proc.c",
		  "kernel/lpf/peripheral/led/lpf_led_proc.c" },
		{ "lpf_mcu_transport.c",
		  "kernel/lpf/transport/mcu/lpf_mcu_transport.c" },
		{ "lpf_mcu_transport_can.c",
		  "kernel/lpf/transport/mcu/lpf_mcu_transport_can.c" },
		{ "lpf_mcu_transport_uart.c",
		  "kernel/lpf/transport/mcu/lpf_mcu_transport_uart.c" },
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

int main(void)
{
	int ret = 0;

	ret += test_service_context_registries();
	ret += test_peripheral_layer_dependencies();
	ret += test_uapi_headers_are_abi_only();

	return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
