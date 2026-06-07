/************************************************************************
 * OSAL - string系统调用封装实现（POSIX）
 ************************************************************************/
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "osal.h"

/*
 * 内存操作
 */

void* OSAL_memset(void *ptr, int32_t value, size_t size)
{
    return memset(ptr, value, size);
}

void* OSAL_memcpy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

void* OSAL_memmove(void *dest, const void *src, size_t size)
{
    return memmove(dest, src, size);
}

int32_t OSAL_memcmp(const void *ptr1, const void *ptr2, size_t size)
{
    return memcmp(ptr1, ptr2, size);
}

/*
 * 字符串操作
 */

size_t OSAL_strlen(const char *str)
{
    return strlen(str);
}

int32_t OSAL_strcmp(const char *str1, const char *str2)
{
    return strcmp(str1, str2);
}

int32_t OSAL_strncmp(const char *str1, const char *str2, size_t n)
{
    return strncmp(str1, str2, n);
}

int32_t OSAL_strcasecmp(const char *str1, const char *str2)
{
    return strcasecmp(str1, str2);
}

char* OSAL_strcpy(char *dest, const char *src)
{
    return strcpy(dest, src);
}

char* OSAL_strncpy(char *dest, const char *src, size_t n)
{
    return strncpy(dest, src, n);
}

char* OSAL_strcat(char *dest, const char *src)
{
    return strcat(dest, src);
}

char* OSAL_strncat(char *dest, const char *src, size_t n)
{
    return strncat(dest, src, n);
}

char* OSAL_strstr(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

size_t OSAL_strcspn(const char *str, const char *reject)
{
    return strcspn(str, reject);
}

/*
 * 字符串格式化
 */

int32_t OSAL_sprintf(char *str, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsprintf(str, format, args);
    va_end(args);

    return ret;
}

int32_t OSAL_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsnprintf(str, size, format, args);
    va_end(args);

    return ret;
}

int32_t OSAL_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    return vsnprintf(str, size, format, args);
}

int32_t OSAL_sscanf(const char *str, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsscanf(str, format, args);
    va_end(args);

    return ret;
}

/*
 * 字符串转换
 */

int32_t OSAL_atoi(const char *str)
{
    return atoi(str);
}

int64_t OSAL_atol(const char *str)
{
    int64_t result = (int64_t)atol(str);
    return result;
}

int64_t OSAL_strtol(const char *str, char **endptr, int32_t base)
{
    int64_t result = (int64_t)strtol(str, endptr, base);
    return result;
}
