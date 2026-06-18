/************************************************************************
 * OSAL - PTY系统调用封装实现（POSIX）
 ************************************************************************/

#include "osal.h"
#include <pty.h>

int32_t osal_openpty(int32_t *master_fd, int32_t *slave_fd, char *name,
					 const void *termios_p, const void *winsize_p)
{
	return openpty(master_fd, slave_fd, name, (const struct termios *)termios_p,
				   (const struct winsize *)winsize_p);
}
