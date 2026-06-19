/************************************************************************
 * PDI internal error helpers
 ************************************************************************/

#ifndef PDI_ERROR_H
#define PDI_ERROR_H

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#define PDI_SUCCESS 0
#define PDI_FAILURE (-1)

static inline int32_t pdi_fail_errno(int err)
{
	errno = (err > 0) ? err : EIO;
	return PDI_FAILURE;
}

static inline int32_t pdi_fail_invalid_arg(void)
{
	return pdi_fail_errno(EINVAL);
}

static inline int32_t pdi_fail_bad_context(void)
{
	return pdi_fail_errno(EBADF);
}

static inline int32_t pdi_fail_no_device(void)
{
	return pdi_fail_errno(ENODEV);
}

static inline int32_t pdi_result_from_syscall(int ret)
{
	return (ret < 0) ? PDI_FAILURE : PDI_SUCCESS;
}

static inline int32_t pdi_check_ptr(const void *ptr)
{
	return (ptr != NULL) ? PDI_SUCCESS : pdi_fail_invalid_arg();
}

static inline int32_t pdi_check_fd(int fd)
{
	return (fd >= 0) ? PDI_SUCCESS : pdi_fail_bad_context();
}

#endif /* PDI_ERROR_H */
