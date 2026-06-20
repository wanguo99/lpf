/************************************************************************
 * PDI LED user API implementation
 ************************************************************************/

#include "pdi/led.h"
#include "pdi/pdi_discovery.h"
#include "pdi_error.h"
#include "pdi_path.h"
#include "pdi_syscall.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

static int32_t pdi_led_ioctl_checked(pdi_led_context_t *ctx,
				     unsigned long request, void *arg)
{
	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;
	if (pdi_check_fd(ctx->fd) < 0)
		return PDI_FAILURE;

	return pdi_result_from_syscall(pdi_syscall_ioctl(ctx->fd, request, arg));
}

int32_t pdi_led_open(pdi_led_context_t *ctx, const char *device_path)
{
	const char *path;

	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;

	ctx->fd = -1;
	path = (device_path != NULL) ? device_path : PDI_LED_DEFAULT_DEVICE;
	ctx->fd = pdi_syscall_open(path, O_RDWR | O_CLOEXEC);
	if (ctx->fd < 0)
		return PDI_FAILURE;

	return PDI_SUCCESS;
}

int32_t pdi_led_open_by_name(pdi_led_context_t *ctx, const char *name)
{
	pdi_ctl_context_t ctl;
	struct lpf_ctl_device_info info;
	char path[PDI_DEVICE_PATH_LEN];
	int32_t ret;

	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;
	ctx->fd = -1;
	if (pdi_check_ptr(name) < 0)
		return PDI_FAILURE;

	ret = pdi_ctl_open(&ctl, NULL);
	if (ret < 0)
		return ret;

	ret = pdi_get_device_by_name(&ctl, name, &info);
	(void)pdi_ctl_close(&ctl);
	if (ret < 0)
		return ret;

	if (info.type != LPF_CTL_DEVICE_TYPE_LED)
		return pdi_fail_no_device();

	ret = snprintf(path, sizeof(path), PDI_LED_DEVICE_PATH_FORMAT,
		       info.index);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return pdi_fail_invalid_arg();

	return pdi_led_open(ctx, path);
}

int32_t pdi_led_close(pdi_led_context_t *ctx)
{
	int ret;

	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;
	if (pdi_check_fd(ctx->fd) < 0)
		return PDI_FAILURE;

	ret = pdi_syscall_close(ctx->fd);
	ctx->fd = -1;
	return pdi_result_from_syscall(ret);
}

int32_t pdi_led_get_info(pdi_led_context_t *ctx, struct lpf_led_info *info)
{
	if (pdi_check_ptr(info) < 0)
		return PDI_FAILURE;

	return pdi_led_ioctl_checked(ctx, LPF_LED_IOC_GET_INFO, info);
}

int32_t pdi_led_get_state(pdi_led_context_t *ctx,
			  struct lpf_led_state *state)
{
	if (pdi_check_ptr(state) < 0)
		return PDI_FAILURE;

	return pdi_led_ioctl_checked(ctx, LPF_LED_IOC_GET_STATE, state);
}

int32_t pdi_led_set_brightness(pdi_led_context_t *ctx, uint32_t index,
			       uint32_t brightness)
{
	struct lpf_led_brightness request = {
		.index = index,
		.brightness = brightness,
	};

	return pdi_led_ioctl_checked(ctx, LPF_LED_IOC_SET_BRIGHTNESS,
				     &request);
}

int32_t pdi_led_enable(pdi_led_context_t *ctx, uint32_t index)
{
	return pdi_led_ioctl_checked(ctx, LPF_LED_IOC_ENABLE, &index);
}

int32_t pdi_led_disable(pdi_led_context_t *ctx, uint32_t index)
{
	return pdi_led_ioctl_checked(ctx, LPF_LED_IOC_DISABLE, &index);
}
