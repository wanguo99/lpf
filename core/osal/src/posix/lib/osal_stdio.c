/************************************************************************
 * OSAL - stdio系统调用封装实现（POSIX）
 ************************************************************************/

#include "osal.h"
#include <stdio.h>

/*
 * 标准输入输出
 */

int32_t osal_getchar(void)
{
	return getchar();
}

void *osal_stdio_stdin(void)
{
	return stdin;
}

void *osal_stdio_stdout(void)
{
	return stdout;
}

void *osal_stdio_stderr(void)
{
	return stderr;
}

char *osal_fgets(char *str, osal_size_t size, void *stream)
{
	union {
		void *osal_stream;
		FILE *posix_stream;
	} stream_union;

	stream_union.osal_stream = stream;
	return fgets(str, (int)size, stream_union.posix_stream);
}

int32_t osal_fflush(void *stream)
{
	union {
		void *osal_stream;
		FILE *posix_stream;
	} stream_union;

	stream_union.osal_stream = stream;
	return fflush(stream_union.posix_stream);
}
