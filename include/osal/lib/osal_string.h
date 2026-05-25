/************************************************************************
 * OSAL - string系统调用封装
 *
 * 功能：
 * - 封装字符串和内存操作函数
 * - 1:1映射系统调用，不引入业务逻辑
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 提供标准C库字符串函数的封装
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_STRING_H
#define OSAL_STRING_H

#include "osal_platform.h"
#include "osal_types.h"
#include <stdarg.h>

/*
 * 内存操作
 */
void* OSAL_Memset(void *ptr, int32_t value, uint32_t size);
void* OSAL_Memcpy(void *dest, const void *src, uint32_t size);
void* OSAL_Memmove(void *dest, const void *src, uint32_t size);
int32_t OSAL_Memcmp(const void *ptr1, const void *ptr2, uint32_t size);

/*
 * 字符串操作
 */
uint32_t OSAL_Strlen(const char *str);
int32_t OSAL_Strcmp(const char *str1, const char *str2);
int32_t OSAL_Strncmp(const char *str1, const char *str2, uint32_t n);
int32_t OSAL_Strcasecmp(const char *str1, const char *str2);
char* OSAL_Strcpy(char *dest, const char *src);
char* OSAL_Strncpy(char *dest, const char *src, uint32_t n);
char* OSAL_Strcat(char *dest, const char *src);
char* OSAL_Strncat(char *dest, const char *src, uint32_t n);
char* OSAL_Strstr(const char *haystack, const char *needle);
uint32_t OSAL_Strcspn(const char *str, const char *reject);

/*
 * 字符串格式化
 */
int32_t OSAL_Sprintf(char *str, const char *format, ...);
int32_t OSAL_Snprintf(char *str, uint32_t size, const char *format, ...);
int32_t OSAL_Vsnprintf(char *str, uint32_t size, const char *format, va_list args);
int32_t OSAL_Sscanf(const char *str, const char *format, ...);

/*
 * 字符串转换
 */
int32_t OSAL_Atoi(const char *str);
int64_t OSAL_Atol(const char *str);
int64_t OSAL_Strtol(const char *str, char **endptr, int32_t base);

#endif /* OSAL_STRING_H */
