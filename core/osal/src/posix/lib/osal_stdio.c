/************************************************************************
 * OSAL - stdio系统调用封装实现（POSIX）
 ************************************************************************/

#include "lib/osal_stdio.h"
#include <stdio.h>

/*
 * 标准流定义
 */
void *OSAL_stdin = NULL;
void *OSAL_stdout = NULL;
void *OSAL_stderr = NULL;

/*
 * 初始化标准流（构造函数自动调用）
 */
__attribute__((constructor))
static void init_stdio_streams(void)
{
    OSAL_stdin = stdin;
    OSAL_stdout = stdout;
    OSAL_stderr = stderr;
}

/*
 * 标准输入输出
 */

int32_t OSAL_Getchar(void)
{
    return getchar();
}

char* OSAL_Fgets(char *str, int32_t size, void *stream)
{
    union {
        void *osal_stream;
        FILE *posix_stream;
    } stream_union;

    stream_union.osal_stream = stream;
    return fgets(str, size, stream_union.posix_stream);
}

int32_t OSAL_Fflush(void *stream)
{
    union {
        void *osal_stream;
        FILE *posix_stream;
    } stream_union;

    stream_union.osal_stream = stream;
    return fflush(stream_union.posix_stream);
}
