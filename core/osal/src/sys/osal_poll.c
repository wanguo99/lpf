/************************************************************************
 * OSAL - I/O多路复用封装实现（POSIX）
 ************************************************************************/

#include <poll.h>
#include "osal.h"

/*===========================================================================
 * I/O多路复用操作
 *===========================================================================*/

int32_t osal_poll(osal_pollfd_t *fds, uint32_t nfds, int32_t timeout)
{
	/* osal_pollfd_t 与 struct pollfd 内存布局完全一致，可以直接转换 */
	struct pollfd *posix_fds = (struct pollfd *)fds;

	int32_t result = poll(posix_fds, (nfds_t)nfds, timeout);

	return result;
}
