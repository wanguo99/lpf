/************************************************************************
 * PDI discovery user API implementation
 ************************************************************************/

#include "pdi/pdi_discovery.h"
#include "pdi_error.h"
#include "pdi_path.h"
#include "pdi_syscall.h"

#include <fcntl.h>
#include <string.h>

static int32_t pdi_ctl_ioctl_checked(pdi_ctl_context_t *ctx,
				     unsigned long request, void *arg)
{
	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;
	if (pdi_check_fd(ctx->fd) < 0)
		return PDI_FAILURE;

	return pdi_result_from_syscall(pdi_syscall_ioctl(ctx->fd, request, arg));
}

int32_t pdi_ctl_open(pdi_ctl_context_t *ctx, const char *device_path)
{
	const char *path;

	if (pdi_check_ptr(ctx) < 0)
		return PDI_FAILURE;

	ctx->fd = -1;
	path = (device_path != NULL) ? device_path : PDI_CTL_DEFAULT_DEVICE;
	ctx->fd = pdi_syscall_open(path, O_RDWR | O_CLOEXEC);
	if (ctx->fd < 0)
		return PDI_FAILURE;

	return PDI_SUCCESS;
}

int32_t pdi_ctl_close(pdi_ctl_context_t *ctx)
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

int32_t pdi_ctl_get_info(pdi_ctl_context_t *ctx, struct lpf_ctl_info *info)
{
	if (pdi_check_ptr(info) < 0)
		return PDI_FAILURE;

	return pdi_ctl_ioctl_checked(ctx, LPF_CTL_IOC_GET_INFO, info);
}

int32_t pdi_list_devices(pdi_ctl_context_t *ctx,
			 struct lpf_ctl_device_info *devices,
			 uint32_t *count)
{
	struct lpf_ctl_device_query query;
	uint32_t requested;
	uint32_t i;
	int32_t ret;

	if (pdi_check_ptr(count) < 0)
		return PDI_FAILURE;

	requested = *count;
	for (i = 0; i < requested; i++) {
		memset(&query, 0, sizeof(query));
		query.match_index = i;
		ret = pdi_ctl_ioctl_checked(ctx, LPF_CTL_IOC_GET_DEVICE,
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
	return PDI_SUCCESS;
}

int32_t pdi_get_device_by_name(pdi_ctl_context_t *ctx, const char *name,
			       struct lpf_ctl_device_info *info)
{
	struct lpf_ctl_device_name_query query;
	int32_t ret;

	if (pdi_check_ptr(name) < 0)
		return PDI_FAILURE;
	if (pdi_check_ptr(info) < 0)
		return PDI_FAILURE;

	memset(&query, 0, sizeof(query));
	strncpy(query.name, name, sizeof(query.name) - 1U);

	ret = pdi_ctl_ioctl_checked(ctx, LPF_CTL_IOC_GET_DEVICE_BY_NAME,
				    &query);
	if (ret < 0)
		return ret;

	*info = query.info;
	return PDI_SUCCESS;
}

int32_t pdi_get_device_by_capability(pdi_ctl_context_t *ctx,
				     uint64_t required_capabilities,
				     uint32_t match_index,
				     struct lpf_ctl_device_info *info)
{
	struct lpf_ctl_device_query query;
	int32_t ret;

	if (pdi_check_ptr(info) < 0)
		return PDI_FAILURE;
	if (required_capabilities == LPF_CTL_DEVICE_CAP_NONE)
		return pdi_fail_invalid_arg();

	memset(&query, 0, sizeof(query));
	query.match_index = match_index;
	query.required_capabilities = required_capabilities;

	ret = pdi_ctl_ioctl_checked(ctx,
				    LPF_CTL_IOC_GET_DEVICE_BY_CAPABILITY,
				    &query);
	if (ret < 0)
		return ret;

	*info = query.info;
	return PDI_SUCCESS;
}
