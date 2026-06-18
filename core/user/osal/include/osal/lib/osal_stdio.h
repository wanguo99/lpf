/************************************************************************
 * OSAL - stdio系统调用封装
 *
 * 功能：
 * - 封装标准输入输出函数
 * - 1:1映射系统调用，不引入业务逻辑
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 提供标准C库stdio函数的封装
 * - 返回值与系统调用保持一致
 * - 保持 Linux/POSIX 语义明确
 ************************************************************************/

#ifndef OSAL_STDIO_H
#define OSAL_STDIO_H

/*
 * 标准流访问宏
 */
#define OSAL_stdin osal_stdio_stdin()
#define OSAL_stdout osal_stdio_stdout()
#define OSAL_stderr osal_stdio_stderr()

/*
 * 标准输入输出
 */
int32_t osal_getchar(void);
void *osal_stdio_stdin(void);
void *osal_stdio_stdout(void);
void *osal_stdio_stderr(void);
char *osal_fgets(char *str, osal_size_t size, void *stream);
int32_t osal_fflush(void *stream);

#endif /* OSAL_STDIO_H */
