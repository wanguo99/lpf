/************************************************************************
 * OSAL - termios系统调用封装实现（POSIX）
 ************************************************************************/

#include "osal.h"
#include <termios.h>
#include <string.h>

/*
 * 终端控制函数
 */

int32_t osal_tcgetattr(int32_t fd, osal_termios_t *termios_p)
{
	struct termios native_termios;
	int32_t ret;

	ret = tcgetattr(fd, &native_termios);
	if (0 != ret) {
		return ret;
	}

	/* 转换为平台无关结构 */
	termios_p->c_iflag = native_termios.c_iflag;
	termios_p->c_oflag = native_termios.c_oflag;
	termios_p->c_cflag = native_termios.c_cflag;
	termios_p->c_lflag = native_termios.c_lflag;
	osal_memcpy(termios_p->c_cc, native_termios.c_cc,
				OSAL_sizeof(termios_p->c_cc));
	termios_p->c_ispeed = cfgetispeed(&native_termios);
	termios_p->c_ospeed = cfgetospeed(&native_termios);

	return 0;
}

int32_t osal_tcsetattr(int32_t fd, int32_t optional_actions,
					   const osal_termios_t *termios_p)
{
	struct termios native_termios;

	/* 转换为原生结构 */
	native_termios.c_iflag = termios_p->c_iflag;
	native_termios.c_oflag = termios_p->c_oflag;
	native_termios.c_cflag = termios_p->c_cflag;
	native_termios.c_lflag = termios_p->c_lflag;
	osal_memcpy(native_termios.c_cc, termios_p->c_cc,
				OSAL_sizeof(native_termios.c_cc));
	cfsetispeed(&native_termios, termios_p->c_ispeed);
	cfsetospeed(&native_termios, termios_p->c_ospeed);

	return tcsetattr(fd, optional_actions, &native_termios);
}

int32_t osal_tcflush(int32_t fd, int32_t queue_selector)
{
	return tcflush(fd, queue_selector);
}

int32_t osal_cfsetispeed(osal_termios_t *termios_p, uint32_t speed)
{
	union {
		osal_termios_t *osal_termios;
		struct termios *posix_termios;
	} termios_union;

	termios_union.osal_termios = termios_p;
	return cfsetispeed(termios_union.posix_termios, speed);
}

int32_t osal_cfsetospeed(osal_termios_t *termios_p, uint32_t speed)
{
	union {
		osal_termios_t *osal_termios;
		struct termios *posix_termios;
	} termios_union;

	termios_union.osal_termios = termios_p;
	return cfsetospeed(termios_union.posix_termios, speed);
}

uint32_t osal_cfgetispeed(const osal_termios_t *termios_p)
{
	return termios_p->c_ispeed;
}

uint32_t osal_cfgetospeed(const osal_termios_t *termios_p)
{
	return termios_p->c_ospeed;
}

int32_t osal_tcdrain(int32_t fd)
{
	return tcdrain(fd);
}

int32_t osal_tcsendbreak(int32_t fd, int32_t duration)
{
	return tcsendbreak(fd, duration);
}

int32_t osal_tcflow(int32_t fd, int32_t action)
{
	return tcflow(fd, action);
}
