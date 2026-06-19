/************************************************************************
 * PDI discovery user API implementation
 ************************************************************************/

#include "pdi/pdi_discovery.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int32_t pdi_ctl_ioctl_checked(pdi_ctl_context_t *ctx,
				     unsigned long request, void *arg)
{
	if (ctx == NULL || ctx->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(ctx->fd, request, arg);
}

int32_t pdi_ctl_open(pdi_ctl_context_t *ctx, const char *device_path)
{
	const char *path;

	if (ctx == NULL) {
		errno = EINVAL;
		return -1;
	}

	path = (device_path != NULL) ? device_path : PDI_CTL_DEFAULT_DEVICE;
	ctx->fd = open(path, O_RDWR | O_CLOEXEC);
	return (ctx->fd < 0) ? -1 : 0;
}

int32_t pdi_ctl_close(pdi_ctl_context_t *ctx)
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

int32_t pdi_ctl_get_info(pdi_ctl_context_t *ctx, struct pdi_ctl_info *info)
{
	if (info == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pdi_ctl_ioctl_checked(ctx, PDI_CTL_IOC_GET_INFO, info);
}

int32_t pdi_list_devices(pdi_ctl_context_t *ctx,
			 struct pdi_ctl_device_info *devices,
			 uint32_t *count)
{
	struct pdi_ctl_device_query query;
	uint32_t requested;
	uint32_t i;
	int32_t ret;

	if (count == NULL) {
		errno = EINVAL;
		return -1;
	}

	requested = *count;
	for (i = 0; i < requested; i++) {
		memset(&query, 0, sizeof(query));
		query.match_index = i;
		ret = pdi_ctl_ioctl_checked(ctx, PDI_CTL_IOC_GET_DEVICE,
					    &query);
		if (ret < 0) {
			if (i == 0)
				return ret;
			break;
		}

		if (devices != NULL)
			devices[i] = query.info;
	}

	*count = i;
	return 0;
}

int32_t pdi_get_device_by_name(pdi_ctl_context_t *ctx, const char *name,
			       struct pdi_ctl_device_info *info)
{
	struct pdi_ctl_device_name_query query;
	int32_t ret;

	if (name == NULL || info == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(&query, 0, sizeof(query));
	strncpy(query.name, name, sizeof(query.name) - 1U);

	ret = pdi_ctl_ioctl_checked(ctx, PDI_CTL_IOC_GET_DEVICE_BY_NAME,
				    &query);
	if (ret < 0)
		return ret;

	*info = query.info;
	return 0;
}

int32_t pdi_get_device_by_capability(pdi_ctl_context_t *ctx,
				     uint64_t required_capabilities,
				     uint32_t match_index,
				     struct pdi_ctl_device_info *info)
{
	struct pdi_ctl_device_query query;
	int32_t ret;

	if (info == NULL || required_capabilities == PDI_CTL_DEVICE_CAP_NONE) {
		errno = EINVAL;
		return -1;
	}

	memset(&query, 0, sizeof(query));
	query.match_index = match_index;
	query.required_capabilities = required_capabilities;

	ret = pdi_ctl_ioctl_checked(ctx,
				    PDI_CTL_IOC_GET_DEVICE_BY_CAPABILITY,
				    &query);
	if (ret < 0)
		return ret;

	*info = query.info;
	return 0;
}
