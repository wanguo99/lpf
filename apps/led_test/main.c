/*
 * LED Test Program
 * Tests PDI LED API functionality
 */

#include "pdi/led.h"
#include "osal.h"
#include <unistd.h>

static void print_led_info(struct pdm_led_info *info)
{
	OSAL_printf("LED Module Info:\n");
	OSAL_printf("  ABI Version:    0x%08x\n", info->abi_version);
	OSAL_printf("  Module Version: %u.%u.%u\n",
		    info->module_version_major,
		    info->module_version_minor,
		    info->module_version_patch);
	OSAL_printf("  Open Count:     %u\n", info->open_count);
	OSAL_printf("  Max Devices:    %u\n", info->max_devices);
}

static void print_led_state(struct pdm_led_state *state)
{
	OSAL_printf("LED State (index %u):\n", state->index);
	OSAL_printf("  Brightness:     %u / %u\n",
		    state->brightness, state->max_brightness);
	OSAL_printf("  Enabled:        %s\n",
		    state->enabled ? "yes" : "no");
}

static int test_led_basic(void)
{
	pdi_led_context_t ctx;
	struct pdm_led_info info;
	struct pdm_led_state state;
	int ret;

	OSAL_printf("\n=== LED Basic Test ===\n\n");

	/* Open LED device */
	OSAL_printf("Opening LED device...\n");
	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		OSAL_printf("Failed to open LED device: %d\n", ret);
		OSAL_printf("Make sure pdm_led module is loaded and device exists\n");
		return ret;
	}
	OSAL_printf("LED device opened successfully\n\n");

	/* Get module info */
	ret = pdi_led_get_info(&ctx, &info);
	if (ret < 0) {
		OSAL_printf("Failed to get LED info: %d\n", ret);
		goto cleanup;
	}
	print_led_info(&info);
	OSAL_printf("\n");

	/* Get initial state */
	state.index = 0;
	ret = pdi_led_get_state(&ctx, &state);
	if (ret < 0) {
		OSAL_printf("Failed to get LED state: %d\n", ret);
		goto cleanup;
	}
	OSAL_printf("Initial state:\n");
	print_led_state(&state);
	OSAL_printf("\n");

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

	OSAL_printf("\n=== LED Brightness Test ===\n\n");

	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		OSAL_printf("Failed to open LED device: %d\n", ret);
		return ret;
	}

	/* Enable LED */
	OSAL_printf("Enabling LED 0...\n");
	ret = pdi_led_enable(&ctx, 0);
	if (ret < 0) {
		OSAL_printf("Failed to enable LED: %d\n", ret);
		goto cleanup;
	}

	/* Test different brightness levels */
	for (i = 0; i < sizeof(brightness_levels) / sizeof(brightness_levels[0]); i++) {
		uint32_t brightness = brightness_levels[i];

		OSAL_printf("Setting brightness to %u...\n", brightness);
		ret = pdi_led_set_brightness(&ctx, 0, brightness);
		if (ret < 0) {
			OSAL_printf("Failed to set brightness: %d\n", ret);
			goto cleanup;
		}

		/* Verify the change */
		state.index = 0;
		ret = pdi_led_get_state(&ctx, &state);
		if (ret < 0) {
			OSAL_printf("Failed to get LED state: %d\n", ret);
			goto cleanup;
		}

		OSAL_printf("  Current brightness: %u\n", state.brightness);
		sleep(1);
	}

	/* Disable LED */
	OSAL_printf("\nDisabling LED 0...\n");
	ret = pdi_led_disable(&ctx, 0);
	if (ret < 0) {
		OSAL_printf("Failed to disable LED: %d\n", ret);
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

	OSAL_printf("\n=== LED Toggle Test ===\n\n");

	ret = pdi_led_open(&ctx, PDI_LED_DEFAULT_DEVICE);
	if (ret < 0) {
		OSAL_printf("Failed to open LED device: %d\n", ret);
		return ret;
	}

	/* Set to full brightness */
	ret = pdi_led_set_brightness(&ctx, 0, 255);
	if (ret < 0) {
		OSAL_printf("Failed to set brightness: %d\n", ret);
		goto cleanup;
	}

	/* Toggle LED 5 times */
	for (i = 0; i < 5; i++) {
		OSAL_printf("Cycle %d: Enabling LED...\n", i + 1);
		ret = pdi_led_enable(&ctx, 0);
		if (ret < 0) {
			OSAL_printf("Failed to enable LED: %d\n", ret);
			goto cleanup;
		}
		sleep(1);

		OSAL_printf("Cycle %d: Disabling LED...\n", i + 1);
		ret = pdi_led_disable(&ctx, 0);
		if (ret < 0) {
			OSAL_printf("Failed to disable LED: %d\n", ret);
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

	OSAL_printf("\n");
	OSAL_printf("========================================\n");
	OSAL_printf("  LPF LED Test Program\n");
	OSAL_printf("========================================\n");

	/* Test 1: Basic operations */
	ret = test_led_basic();
	if (ret < 0) {
		OSAL_printf("\nBasic test failed!\n");
		return 1;
	}

	/* Test 2: Brightness control */
	ret = test_led_brightness();
	if (ret < 0) {
		OSAL_printf("\nBrightness test failed!\n");
		return 1;
	}

	/* Test 3: Toggle */
	ret = test_led_toggle();
	if (ret < 0) {
		OSAL_printf("\nToggle test failed!\n");
		return 1;
	}

	OSAL_printf("\n");
	OSAL_printf("========================================\n");
	OSAL_printf("  All LED tests passed!\n");
	OSAL_printf("========================================\n");
	OSAL_printf("\n");

	return 0;
}
