// SPDX-License-Identifier: MIT

#include "pdi/pdi.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LPF_MOCK_EXPECTED_DEVICE_COUNT 3U

static int fail(int code, const char *message)
{
	fprintf(stderr, "lpf_mock_runtime_smoke: %s\n", message);
	return code;
}

static int expect_device(pdi_ctl_context_t *ctl, const char *name,
			 uint32_t type, uint32_t index,
			 uint64_t required_capabilities)
{
	struct lpf_ctl_device_info info;

	memset(&info, 0, sizeof(info));
	if (pdi_get_device_by_name(ctl, name, &info) != 0)
		return fail(100, "device lookup by name failed");

	if (info.type != type || info.index != index)
		return fail(101, "device type/index mismatch");

	if (info.state != LPF_CTL_DEVICE_STATE_BOUND)
		return fail(102, "device is not bound");

	if ((info.capabilities & required_capabilities) != required_capabilities)
		return fail(103, "device capability mismatch");

	return 0;
}

static int expect_missing_devices(void)
{
	pdi_mcu_context_t mcu = { .fd = -1 };
	pdi_led_context_t led = { .fd = -1 };

	errno = 0;
	if (pdi_mcu_open(&mcu, "/dev/lpf/mcu1") == 0) {
		(void)pdi_mcu_close(&mcu);
		return fail(200, "unexpected unconfigured MCU node");
	}
	if (errno != ENOENT && errno != ENODEV)
		return fail(201, "unexpected MCU missing-node errno");

	errno = 0;
	if (pdi_led_open(&led, "/dev/lpf/led2") == 0) {
		(void)pdi_led_close(&led);
		return fail(202, "unexpected unconfigured LED node");
	}
	if (errno != ENOENT && errno != ENODEV)
		return fail(203, "unexpected LED missing-node errno");

	return 0;
}

static int expect_mcu_io(void)
{
	pdi_mcu_context_t mcu = { .fd = -1 };
	struct lpf_mcu_info info;

	if (pdi_mcu_open_by_name(&mcu, "mcu0") != 0)
		return fail(300, "failed to open MCU by name");

	memset(&info, 0, sizeof(info));
	if (pdi_mcu_get_info(&mcu, &info) != 0) {
		(void)pdi_mcu_close(&mcu);
		return fail(301, "MCU GET_INFO failed");
	}

	if (info.abi_version != LPF_MCU_ABI_VERSION || info.max_devices == 0) {
		(void)pdi_mcu_close(&mcu);
		return fail(302, "MCU info mismatch");
	}

	if (pdi_mcu_close(&mcu) != 0)
		return fail(303, "failed to close MCU");

	return 0;
}

static int expect_led_io(const char *name)
{
	pdi_led_context_t led = { .fd = -1 };
	struct lpf_led_info info;

	if (pdi_led_open_by_name(&led, name) != 0)
		return fail(400, "failed to open LED by name");

	memset(&info, 0, sizeof(info));
	if (pdi_led_get_info(&led, &info) != 0) {
		(void)pdi_led_close(&led);
		return fail(401, "LED GET_INFO failed");
	}

	if (info.abi_version != LPF_LED_ABI_VERSION || info.max_devices == 0) {
		(void)pdi_led_close(&led);
		return fail(402, "LED info mismatch");
	}

	if (pdi_led_close(&led) != 0)
		return fail(403, "failed to close LED");

	return 0;
}

int main(void)
{
	pdi_ctl_context_t ctl = { .fd = -1 };
	struct lpf_ctl_info ctl_info;
	struct lpf_ctl_device_info devices[LPF_MOCK_EXPECTED_DEVICE_COUNT];
	uint32_t count = LPF_MOCK_EXPECTED_DEVICE_COUNT;
	int ret;

	if (pdi_ctl_open(&ctl, NULL) != 0)
		return fail(1, "failed to open /dev/lpf_ctl");

	memset(&ctl_info, 0, sizeof(ctl_info));
	if (pdi_ctl_get_info(&ctl, &ctl_info) != 0) {
		(void)pdi_ctl_close(&ctl);
		return fail(2, "control GET_INFO failed");
	}

	if (ctl_info.abi_version != LPF_CTL_ABI_VERSION ||
	    ctl_info.device_count != LPF_MOCK_EXPECTED_DEVICE_COUNT) {
		(void)pdi_ctl_close(&ctl);
		return fail(3, "control info mismatch");
	}

	memset(devices, 0, sizeof(devices));
	if (pdi_list_devices(&ctl, devices, &count) != 0 ||
	    count != LPF_MOCK_EXPECTED_DEVICE_COUNT) {
		(void)pdi_ctl_close(&ctl);
		return fail(4, "device list mismatch");
	}

	ret = expect_device(&ctl, "mcu0", LPF_CTL_DEVICE_TYPE_MCU, 0,
			    LPF_CTL_DEVICE_CAP_USER_IOCTL |
				    LPF_CTL_DEVICE_CAP_DEBUGFS |
				    LPF_CTL_DEVICE_CAP_TRANSPORT_CAN);
	if (ret) {
		(void)pdi_ctl_close(&ctl);
		return ret;
	}

	ret = expect_device(&ctl, "status", LPF_CTL_DEVICE_TYPE_LED, 0,
			    LPF_CTL_DEVICE_CAP_USER_IOCTL |
				    LPF_CTL_DEVICE_CAP_DEBUGFS |
				    LPF_CTL_DEVICE_CAP_CONTROL_GPIO);
	if (ret) {
		(void)pdi_ctl_close(&ctl);
		return ret;
	}

	ret = expect_device(&ctl, "activity", LPF_CTL_DEVICE_TYPE_LED, 1,
			    LPF_CTL_DEVICE_CAP_USER_IOCTL |
				    LPF_CTL_DEVICE_CAP_DEBUGFS |
				    LPF_CTL_DEVICE_CAP_CONTROL_PWM);
	if (ret) {
		(void)pdi_ctl_close(&ctl);
		return ret;
	}

	if (pdi_ctl_close(&ctl) != 0)
		return fail(5, "failed to close control node");

	ret = expect_mcu_io();
	if (ret)
		return ret;

	ret = expect_led_io("status");
	if (ret)
		return ret;

	ret = expect_led_io("activity");
	if (ret)
		return ret;

	ret = expect_missing_devices();
	if (ret)
		return ret;

	printf("lpf_mock_runtime_smoke: passed\n");
	return 0;
}
