/************************************************************************
 * PDI user API implementation
 ************************************************************************/

#include "pdi/pdi.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int32_t pdi_ioctl_checked(pdi_context_t *ctx, unsigned long request,
				 void *arg)
{
	if (ctx == NULL || ctx->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(ctx->fd, request, arg);
}

int32_t pdi_open(pdi_context_t *ctx, const char *device_path)
{
	const char *path;

	if (ctx == NULL) {
		errno = EINVAL;
		return -1;
	}

	path = (device_path != NULL) ? device_path : PDI_DEFAULT_DEVICE;
	ctx->fd = open(path, O_RDWR | O_CLOEXEC);
	return (ctx->fd < 0) ? -1 : 0;
}

int32_t pdi_close(pdi_context_t *ctx)
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

int32_t pdi_ping(pdi_context_t *ctx)
{
	return pdi_ioctl_checked(ctx, PDI_IOC_PING, NULL);
}

int32_t pdi_get_info(pdi_context_t *ctx, struct pdi_info *info)
{
	if (info == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_ioctl_checked(ctx, PDI_IOC_GET_INFO, info);
}

int32_t pdi_get_echo(pdi_context_t *ctx, uint32_t *enabled)
{
	if (enabled == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_ioctl_checked(ctx, PDI_IOC_GET_ECHO, enabled);
}

int32_t pdi_set_echo(pdi_context_t *ctx, uint32_t enabled)
{
	return pdi_ioctl_checked(ctx, PDI_IOC_SET_ECHO, &enabled);
}
