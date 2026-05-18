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
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_STDIO_H
#define OSAL_STDIO_H

#include "osal_types.h"

/*
 * 标准输入输出
 */
int32_t OSAL_Getchar(void);
char* OSAL_Fgets(char *str, int32_t size, void *stream);
int32_t OSAL_Fflush(void *stream);

/*
 * 标准流定义
 */
extern void *OSAL_stdin;
extern void *OSAL_stdout;
extern void *OSAL_stderr;

#endif /* OSAL_STDIO_H */
