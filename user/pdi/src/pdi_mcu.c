/************************************************************************
 * PDI MCU user API implementation
 ************************************************************************/

#include "pdi/mcu.h"
#include "pdi/pdi_discovery.h"
#include "pdi_error.h"
#include "pdi_path.h"
#include "pdi_syscall.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

static int32_t pdi_mcu_ioctl_checked(pdi_mcu_context_t *ctx,
				     unsigned long request, void *arg)
{
	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;
	if (pdi_check_fd(ctx->fd) < 0)
		return PDI_FAILURE;

	return pdi_result_from_syscall(pdi_syscall_ioctl(ctx->fd, request, arg));
}

int32_t pdi_mcu_open(pdi_mcu_context_t *ctx, const char *device_path)
{
	const char *path;

	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;

	ctx->fd = -1;
	path = (device_path != NULL) ? device_path : PDI_MCU_DEFAULT_DEVICE;
	ctx->fd = pdi_syscall_open(path, O_RDWR | O_CLOEXEC);
	if (ctx->fd < 0)
		return PDI_FAILURE;

	return PDI_SUCCESS;
}

int32_t pdi_mcu_open_by_name(pdi_mcu_context_t *ctx, const char *name)
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

	if (info.type != LPF_CTL_DEVICE_TYPE_MCU)
		return pdi_fail_no_device();

	ret = snprintf(path, sizeof(path), PDI_MCU_DEVICE_PATH_FORMAT,
		       info.index);
	if (ret < 0 || (size_t)ret >= sizeof(path))
		return pdi_fail_invalid_arg();

	return pdi_mcu_open(ctx, path);
}

int32_t pdi_mcu_close(pdi_mcu_context_t *ctx)
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

int32_t pdi_mcu_get_info(pdi_mcu_context_t *ctx, struct lpf_mcu_info *info)
{
	if (pdi_check_ptr(info) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_GET_INFO, info);
}

int32_t pdi_mcu_get_version(pdi_mcu_context_t *ctx,
			    struct lpf_mcu_version *version)
{
	if (pdi_check_ptr(version) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_GET_VERSION, version);
}

int32_t pdi_mcu_get_status(pdi_mcu_context_t *ctx,
			   struct lpf_mcu_status *status)
{
	if (pdi_check_ptr(status) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_GET_STATUS, status);
}

int32_t pdi_mcu_reset(pdi_mcu_context_t *ctx, uint32_t index)
{
	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_RESET, &index);
}

int32_t pdi_mcu_command(pdi_mcu_context_t *ctx,
			struct lpf_mcu_command *command)
{
	if (pdi_check_ptr(command) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_COMMAND, command);
}

int32_t pdi_mcu_read_data(pdi_mcu_context_t *ctx,
			  struct lpf_mcu_data *data)
{
	if (pdi_check_ptr(data) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_READ_DATA, data);
}

int32_t pdi_mcu_write_data(pdi_mcu_context_t *ctx,
			   const struct lpf_mcu_data *data)
{
	if (pdi_check_ptr(data) < 0)
		return PDI_FAILURE;

	return pdi_mcu_ioctl_checked(ctx, LPF_MCU_IOC_WRITE_DATA, (void *)data);
}
