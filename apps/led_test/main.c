/*
 * LED Test Program
 * Tests PDI LED API functionality
 */

#include "pdi/led.h"
#include <stdio.h>
#include <unistd.h>

static void print_led_info(struct pdm_led_info *info)
{
	printf("LED Module Info:\n");
	printf("  ABI Version:    0x%08x\n", info->abi_version);
	printf("  Module Version: %u.%u.%u\n",
		    info->module_version_major,
		    info->module_version_minor,
		    info->module_version_patch);
	printf("  Open Count:     %u\n", info->open_count);
	printf("  Max Devices:    %u\n", info->max_devices);
}

static void print_led_state(struct pdm_led_state *state)
{
	printf("LED State:\n");
	printf("  Brightness:     %u / %u\n",
		    state->brightness, state->max_brightness);
	printf("  Enabled:        %s\n",
		    state->enabled ? "yes" : "no");
}

static int test_led_basic(void)
{
	pdi_led_context_t ctx;
	struct pdm_led_info info;
	struct pdm_led_state state;
	int ret;

	printf("\n=== LED Basic Test ===\n\n");

	/* Open LED device */
	printf("Opening LED device...\n");
	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		printf("Failed to open LED device: %d\n", ret);
		printf("Make sure pdm_led module is loaded and device exists\n");
		return ret;
	}
	printf("LED device opened successfully\n\n");

	/* Get module info */
	ret = pdi_led_get_info(&ctx, &info);
	if (ret < 0) {
		printf("Failed to get LED info: %d\n", ret);
		goto cleanup;
	}
	print_led_info(&info);
	printf("\n");

	/* Get initial state */
	ret = pdi_led_get_state(&ctx, &state);
	if (ret < 0) {
		printf("Failed to get LED state: %d\n", ret);
		goto cleanup;
	}
	printf("Initial state:\n");
	print_led_state(&state);
	printf("\n");

	ret = 0;

cleanup:
	pdi_led_close(&ctx);
	return ret;
}

static int test_led_brightness(void)
{
	pdi_led_context_t ctx;
	struct pdm_led_state state;
	int ret;
	uint32_t brightness_levels[] = {0, 64, 128, 192, 255};
	int i;

	printf("\n=== LED Brightness Test ===\n\n");

	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		printf("Failed to open LED device: %d\n", ret);
		return ret;
	}

	/* Enable LED */
	printf("Enabling LED 0...\n");
	ret = pdi_led_enable(&ctx);
	if (ret < 0) {
		printf("Failed to enable LED: %d\n", ret);
		goto cleanup;
	}

	/* Test different brightness levels */
	for (i = 0; i < sizeof(brightness_levels) / sizeof(brightness_levels[0]); i++) {
		uint32_t brightness = brightness_levels[i];

		printf("Setting brightness to %u...\n", brightness);
		ret = pdi_led_set_brightness(&ctx, brightness);
		if (ret < 0) {
			printf("Failed to set brightness: %d\n", ret);
			goto cleanup;
		}

		/* Verify the change */
		ret = pdi_led_get_state(&ctx, &state);
		if (ret < 0) {
			printf("Failed to get LED state: %d\n", ret);
			goto cleanup;
		}

		printf("  Current brightness: %u\n", state.brightness);
		sleep(1);
	}

	/* Disable LED */
	printf("\nDisabling LED 0...\n");
	ret = pdi_led_disable(&ctx);
	if (ret < 0) {
		printf("Failed to disable LED: %d\n", ret);
	}

	ret = 0;

cleanup:
	pdi_led_close(&ctx);
	return ret;
}

static int test_led_toggle(void)
{
	pdi_led_context_t ctx;
	int ret;
	int i;

	printf("\n=== LED Toggle Test ===\n\n");

	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		printf("Failed to open LED device: %d\n", ret);
		return ret;
	}

	/* Set to full brightness */
	ret = pdi_led_set_brightness(&ctx, 255);
	if (ret < 0) {
		printf("Failed to set brightness: %d\n", ret);
		goto cleanup;
	}

	/* Toggle LED 5 times */
	for (i = 0; i < 5; i++) {
		printf("Cycle %d: Enabling LED...\n", i + 1);
		ret = pdi_led_enable(&ctx);
		if (ret < 0) {
			printf("Failed to enable LED: %d\n", ret);
			goto cleanup;
		}
		sleep(1);

		printf("Cycle %d: Disabling LED...\n", i + 1);
		ret = pdi_led_disable(&ctx);
		if (ret < 0) {
			printf("Failed to disable LED: %d\n", ret);
			goto cleanup;
		}
		sleep(1);
	}

	ret = 0;

cleanup:
	pdi_led_close(&ctx);
	return ret;
}

int main(int argc, char *argv[])
{
	int ret;

	printf("\n");
	printf("========================================\n");
	printf("  PDM LED Test Program\n");
	printf("========================================\n");

	/* Test 1: Basic operations */
	ret = test_led_basic();
	if (ret < 0) {
		printf("\nBasic test failed!\n");
		return 1;
	}

	/* Test 2: Brightness control */
	ret = test_led_brightness();
	if (ret < 0) {
		printf("\nBrightness test failed!\n");
		return 1;
	}

	/* Test 3: Toggle */
	ret = test_led_toggle();
	if (ret < 0) {
		printf("\nToggle test failed!\n");
		return 1;
	}

	printf("\n");
	printf("========================================\n");
	printf("  All LED tests passed!\n");
	printf("========================================\n");
	printf("\n");

	return 0;
}
