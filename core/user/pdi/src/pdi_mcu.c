/************************************************************************
 * PDI MCU user API implementation
 ************************************************************************/

#include "pdi/pdi_mcu.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int32_t pdi_mcu_ioctl_checked(pdi_mcu_context_t *ctx,
				     unsigned long request, void *arg)
{
	if (ctx == NULL || ctx->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(ctx->fd, request, arg);
}

int32_t pdi_mcu_open(pdi_mcu_context_t *ctx, const char *device_path)
{
	const char *path;

	if (ctx == NULL) {
		errno = EINVAL;
		return -1;
	}

	path = (device_path != NULL) ? device_path : PDI_MCU_DEFAULT_DEVICE;
	ctx->fd = open(path, O_RDWR | O_CLOEXEC);
	return (ctx->fd < 0) ? -1 : 0;
}

int32_t pdi_mcu_close(pdi_mcu_context_t *ctx)
{
	int ret;

	if (ctx == NULL || ctx->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	ret = close(ctx->fd);
	ctx->fd = -1;
	return ret;
}

int32_t pdi_mcu_get_info(pdi_mcu_context_t *ctx, struct pdi_mcu_info *info)
{
	if (info == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_GET_INFO, info);
}

int32_t pdi_mcu_get_version(pdi_mcu_context_t *ctx,
			    struct pdi_mcu_version *version)
{
	if (version == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_GET_VERSION, version);
}

int32_t pdi_mcu_get_status(pdi_mcu_context_t *ctx,
			   struct pdi_mcu_status *status)
{
	if (status == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_GET_STATUS, status);
}

int32_t pdi_mcu_reset(pdi_mcu_context_t *ctx, uint32_t index)
{
	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_RESET, &index);
}

int32_t pdi_mcu_command(pdi_mcu_context_t *ctx,
			struct pdi_mcu_command *command)
{
	if (command == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_COMMAND, command);
}

int32_t pdi_mcu_read_data(pdi_mcu_context_t *ctx,
			  struct pdi_mcu_data *data)
{
	if (data == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_READ_DATA, data);
}

int32_t pdi_mcu_write_data(pdi_mcu_context_t *ctx,
			   const struct pdi_mcu_data *data)
{
	if (data == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_mcu_ioctl_checked(ctx, PDI_MCU_IOC_WRITE_DATA, (void *)data);
}
