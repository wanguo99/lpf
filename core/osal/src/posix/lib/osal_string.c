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

void* OSAL_Memset(void *ptr, int32_t value, size_t size)
{
    return memset(ptr, value, size);
}

void* OSAL_Memcpy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

void* OSAL_Memmove(void *dest, const void *src, size_t size)
{
    return memmove(dest, src, size);
}

int32_t OSAL_Memcmp(const void *ptr1, const void *ptr2, size_t size)
{
    return memcmp(ptr1, ptr2, size);
}

/*
 * 字符串操作
 */

size_t OSAL_Strlen(const char *str)
{
    return strlen(str);
}

int32_t OSAL_Strcmp(const char *str1, const char *str2)
{
    return strcmp(str1, str2);
}

int32_t OSAL_Strncmp(const char *str1, const char *str2, size_t n)
{
    return strncmp(str1, str2, n);
}

int32_t OSAL_Strcasecmp(const char *str1, const char *str2)
{
    return strcasecmp(str1, str2);
}

char* OSAL_Strcpy(char *dest, const char *src)
{
    return strcpy(dest, src);
}

char* OSAL_Strncpy(char *dest, const char *src, size_t n)
{
    return strncpy(dest, src, n);
}

char* OSAL_Strcat(char *dest, const char *src)
{
    return strcat(dest, src);
}

char* OSAL_Strncat(char *dest, const char *src, size_t n)
{
    return strncat(dest, src, n);
}

char* OSAL_Strstr(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

size_t OSAL_Strcspn(const char *str, const char *reject)
{
    return strcspn(str, reject);
}

/*
 * 字符串格式化
 */

int32_t OSAL_Sprintf(char *str, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsprintf(str, format, args);
    va_end(args);

    return ret;
}

int32_t OSAL_Snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    int32_t ret;

    va_start(args, format);
    ret = vsnprintf(str, size, format, args);
    va_end(args);

    return ret;
}

int32_t OSAL_Vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    return vsnprintf(str, size, format, args);
}

int32_t OSAL_Sscanf(const char *str, const char *format, ...)
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

int32_t OSAL_Atoi(const char *str)
{
    return atoi(str);
}

int64_t OSAL_Atol(const char *str)
{
    int64_t result = (int64_t)atol(str);
    return result;
}

int64_t OSAL_Strtol(const char *str, char **endptr, int32_t base)
{
    int64_t result = (int64_t)strtol(str, endptr, base);
    return result;
}
