// SPDX-License-Identifier: MIT

#include "pdi/pdi.h"
#include "pdi_syscall.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#define MOCK_FD 77

typedef struct {
	int open_calls;
	int close_calls;
	int ioctl_calls;
	char last_path[128];
	int last_flags;
	int last_fd;
	unsigned long last_request;
} mock_state_t;

static mock_state_t g_mock;

static void mock_reset(void)
{
	memset(&g_mock, 0, sizeof(g_mock));
	pdi_syscall_reset_ops();
}

static int mock_open(const char *path, int flags)
{
	g_mock.open_calls++;
	g_mock.last_flags = flags;
	if (path) {
		strncpy(g_mock.last_path, path, sizeof(g_mock.last_path) - 1U);
		g_mock.last_path[sizeof(g_mock.last_path) - 1U] = '\0';
	}
	return MOCK_FD;
}

static int mock_close(int fd)
{
	g_mock.close_calls++;
	g_mock.last_fd = fd;
	return 0;
}

static int mock_ioctl(int fd, unsigned long request, void *arg)
{
	g_mock.ioctl_calls++;
	g_mock.last_fd = fd;
	g_mock.last_request = request;

	switch (request) {
	case LPF_CTL_IOC_GET_DEVICE_BY_NAME: {
		struct lpf_ctl_device_name_query *query = arg;

		if (strcmp(query->name, "mcu-main") == 0) {
			query->info.type = LPF_CTL_DEVICE_TYPE_MCU;
			query->info.index = 3;
			return 0;
		}
		if (strcmp(query->name, "led-front") == 0) {
			query->info.type = LPF_CTL_DEVICE_TYPE_LED;
			query->info.index = 2;
			return 0;
		}

		errno = ENODEV;
		return -1;
	}
	case LPF_MCU_IOC_GET_VERSION: {
		struct lpf_mcu_version *version = arg;

		if (version->index != 0)
			return -1;
		version->major = 1;
		version->minor = 2;
		version->patch = 3;
		version->build = 4;
		strncpy(version->version_string, "1.2.3.4",
			sizeof(version->version_string) - 1U);
		return 0;
	}
	case LPF_MCU_IOC_GET_STATUS: {
		struct lpf_mcu_status *status = arg;

		if (status->index != 0)
			return -1;
		status->online = 1;
		status->state = LPF_MCU_STATE_READY;
		status->uptime_sec = 10;
		status->error_code = 0;
		status->temperature_milli_celsius = 42000;
		status->voltage_mv = 3300;
		status->timestamp_us = 123456U;
		return 0;
	}
	case LPF_MCU_IOC_RESET: {
		const uint32_t *index = arg;

		return index && *index == 9 ? 0 : -1;
	}
	case LPF_MCU_IOC_COMMAND: {
		struct lpf_mcu_command *command = arg;

		if (command->index != 0 || command->command != 0x22 ||
		    command->tx_len != 2 || command->tx_data[0] != 0xAA ||
		    command->tx_data[1] != 0x55)
			return -1;
		command->rx_len = 2;
		command->rx_data[0] = 0x12;
		command->rx_data[1] = 0x34;
		return 0;
	}
	case LPF_MCU_IOC_READ_DATA: {
		struct lpf_mcu_data *data = arg;

		if (data->index != 0 || data->address != 0x1000 ||
		    data->len != 3)
			return -1;
		data->data[0] = 0x01;
		data->data[1] = 0x02;
		data->data[2] = 0x03;
		return 0;
	}
	case LPF_MCU_IOC_WRITE_DATA: {
		const struct lpf_mcu_data *data = arg;

		return data->index == 0 && data->address == 0x2000 &&
			       data->len == 2 && data->data[0] == 0xFE &&
			       data->data[1] == 0xED ?
			       0 :
			       -1;
	}
	case LPF_LED_IOC_GET_STATE: {
		struct lpf_led_state *state = arg;

		if (state->index != 0)
			return -1;
		state->brightness = 7;
		state->max_brightness = 10;
		state->enabled = 1;
		return 0;
	}
	case LPF_LED_IOC_SET_BRIGHTNESS: {
		const struct lpf_led_brightness *brightness = arg;

		return brightness->index == 5 && brightness->brightness == 8 ?
			       0 :
			       -1;
	}
	case LPF_LED_IOC_ENABLE:
	case LPF_LED_IOC_DISABLE: {
		const uint32_t *index = arg;

		return index && *index == 6 ? 0 : -1;
	}
	default:
		errno = ENOTTY;
		return -1;
	}
}

static const pdi_syscall_ops_t g_mock_ops = {
	.open = mock_open,
	.close = mock_close,
	.ioctl = mock_ioctl,
};

static int test_default_paths(void)
{
	pdi_ctl_context_t ctl = { .fd = -1 };
	pdi_mcu_context_t mcu = { .fd = -1 };
	pdi_led_context_t led = { .fd = -1 };

	mock_reset();
	pdi_syscall_set_ops(&g_mock_ops);

	if (pdi_ctl_open(&ctl, NULL) != 0)
		return 301;
	if (strcmp(g_mock.last_path, PDI_CTL_DEFAULT_DEVICE) != 0)
		return 302;
	if (pdi_ctl_close(&ctl) != 0 || ctl.fd != -1)
		return 303;

	if (pdi_mcu_open(&mcu, NULL) != 0)
		return 304;
	if (strcmp(g_mock.last_path, PDI_MCU_DEFAULT_DEVICE) != 0)
		return 305;
	if (pdi_mcu_close(&mcu) != 0 || mcu.fd != -1)
		return 306;

	if (pdi_led_open(&led, NULL) != 0)
		return 307;
	if (strcmp(g_mock.last_path, PDI_LED_DEFAULT_DEVICE) != 0)
		return 308;
	if (pdi_led_close(&led) != 0 || led.fd != -1)
		return 309;

	pdi_syscall_reset_ops();
	return 0;
}

static int test_open_by_name(void)
{
	pdi_mcu_context_t mcu = { .fd = -1 };
	pdi_led_context_t led = { .fd = -1 };

	mock_reset();
	pdi_syscall_set_ops(&g_mock_ops);

	if (pdi_mcu_open_by_name(&mcu, "mcu-main") != 0)
		return 1;
	if (mcu.fd != MOCK_FD)
		return 2;
	if (strcmp(g_mock.last_path, "/dev/lpf/mcu3") != 0)
		return 3;
	if ((g_mock.last_flags & O_CLOEXEC) == 0)
		return 4;

	if (pdi_led_open_by_name(&led, "led-front") != 0)
		return 5;
	if (led.fd != MOCK_FD)
		return 6;
	if (strcmp(g_mock.last_path, "/dev/lpf/led2") != 0)
		return 7;

	if (pdi_mcu_close(&mcu) != 0 || mcu.fd != -1)
		return 8;
	if (pdi_led_close(&led) != 0 || led.fd != -1)
		return 9;

	pdi_syscall_reset_ops();
	return 0;
}

static int test_mcu_operations(void)
{
	pdi_mcu_context_t ctx = { .fd = MOCK_FD };
	struct lpf_mcu_version version;
	struct lpf_mcu_status status;
	struct lpf_mcu_command command;
	struct lpf_mcu_data data;

	mock_reset();
	pdi_syscall_set_ops(&g_mock_ops);

	memset(&version, 0, sizeof(version));
	if (pdi_mcu_get_version(&ctx, &version) != 0)
		return 101;
	if (g_mock.last_request != LPF_MCU_IOC_GET_VERSION ||
	    version.major != 1 || version.minor != 2 || version.patch != 3 ||
	    version.build != 4 || strcmp(version.version_string, "1.2.3.4"))
		return 102;

	memset(&status, 0, sizeof(status));
	if (pdi_mcu_get_status(&ctx, &status) != 0)
		return 103;
	if (g_mock.last_request != LPF_MCU_IOC_GET_STATUS ||
	    status.online != 1 || status.state != LPF_MCU_STATE_READY ||
	    status.temperature_milli_celsius != 42000)
		return 104;

	if (pdi_mcu_reset(&ctx, 9) != 0 ||
	    g_mock.last_request != LPF_MCU_IOC_RESET)
		return 105;

	memset(&command, 0, sizeof(command));
	command.command = 0x22;
	command.tx_len = 2;
	command.rx_len = 2;
	command.tx_data[0] = 0xAA;
	command.tx_data[1] = 0x55;
	if (pdi_mcu_command(&ctx, &command) != 0)
		return 106;
	if (g_mock.last_request != LPF_MCU_IOC_COMMAND ||
	    command.rx_len != 2 || command.rx_data[0] != 0x12 ||
	    command.rx_data[1] != 0x34)
		return 107;

	memset(&data, 0, sizeof(data));
	data.address = 0x1000;
	data.len = 3;
	if (pdi_mcu_read_data(&ctx, &data) != 0)
		return 108;
	if (g_mock.last_request != LPF_MCU_IOC_READ_DATA ||
	    data.data[0] != 0x01 || data.data[1] != 0x02 ||
	    data.data[2] != 0x03)
		return 109;

	memset(&data, 0, sizeof(data));
	data.address = 0x2000;
	data.len = 2;
	data.data[0] = 0xFE;
	data.data[1] = 0xED;
	if (pdi_mcu_write_data(&ctx, &data) != 0)
		return 110;
	if (g_mock.last_request != LPF_MCU_IOC_WRITE_DATA)
		return 111;

	pdi_syscall_reset_ops();
	return 0;
}

static int test_led_operations(void)
{
	pdi_led_context_t ctx = { .fd = MOCK_FD };
	struct lpf_led_state state;

	mock_reset();
	pdi_syscall_set_ops(&g_mock_ops);

	memset(&state, 0, sizeof(state));
	if (pdi_led_get_state(&ctx, &state) != 0)
		return 201;
	if (g_mock.last_request != LPF_LED_IOC_GET_STATE ||
	    state.brightness != 7 || state.max_brightness != 10 ||
	    state.enabled != 1)
		return 202;

	if (pdi_led_set_brightness(&ctx, 5, 8) != 0 ||
	    g_mock.last_request != LPF_LED_IOC_SET_BRIGHTNESS)
		return 203;

	if (pdi_led_enable(&ctx, 6) != 0 ||
	    g_mock.last_request != LPF_LED_IOC_ENABLE)
		return 204;

	if (pdi_led_disable(&ctx, 6) != 0 ||
	    g_mock.last_request != LPF_LED_IOC_DISABLE)
		return 205;

	pdi_syscall_reset_ops();
	return 0;
}

int main(void)
{
	int ret;

	ret = test_default_paths();
	if (ret)
		return ret;

	ret = test_open_by_name();
	if (ret)
		return ret;

	ret = test_mcu_operations();
	if (ret)
		return ret;

	return test_led_operations();
}
