/************************************************************************
 * OSAL String API
 *
 * String and memory manipulation functions
 ************************************************************************/

#ifndef OSAL_STRING_API_H
#define OSAL_STRING_API_H

#include "osal_types_api.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 内存操作 API
 *===========================================================================*/

/**
 * @brief 内存设置
 * @param s 目标内存地址
 * @param c 设置的值
 * @param n 字节数
 * @return 目标内存地址
 */
void *OSAL_Memset(void *s, int32_t c, size_t n);

/**
 * @brief 内存复制
 * @param dest 目标内存地址
 * @param src 源内存地址
 * @param n 字节数
 * @return 目标内存地址
 */
void *OSAL_Memcpy(void *dest, const void *src, size_t n);

/**
 * @brief 内存移动（支持重叠）
 * @param dest 目标内存地址
 * @param src 源内存地址
 * @param n 字节数
 * @return 目标内存地址
 */
void *OSAL_Memmove(void *dest, const void *src, size_t n);

/**
 * @brief 内存比较
 * @param s1 第一个内存地址
 * @param s2 第二个内存地址
 * @param n 字节数
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 */
int32_t OSAL_Memcmp(const void *s1, const void *s2, size_t n);

/*===========================================================================
 * 字符串操作 API
 *===========================================================================*/

/**
 * @brief 获取字符串长度
 * @param s 字符串
 * @return 字符串长度（不包括'\0'）
 */
size_t OSAL_Strlen(const char *s);

/**
 * @brief 字符串比较
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 */
int32_t OSAL_Strcmp(const char *s1, const char *s2);

/**
 * @brief 字符串比较（限定长度）
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param n 最大比较长度
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 */
int32_t OSAL_Strncmp(const char *s1, const char *s2, size_t n);

/**
 * @brief 字符串比较（忽略大小写）
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 */
int32_t OSAL_Strcasecmp(const char *s1, const char *s2);

/**
 * @brief 字符串复制
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return 目标缓冲区
 */
char *OSAL_Strcpy(char *dest, const char *src);

/**
 * @brief 字符串复制（限定长度）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param n 最大复制长度
 * @return 目标缓冲区
 */
char *OSAL_Strncpy(char *dest, const char *src, size_t n);

/**
 * @brief 字符串连接
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return 目标缓冲区
 */
char *OSAL_Strcat(char *dest, const char *src);

/**
 * @brief 字符串连接（限定长度）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param n 最大连接长度
 * @return 目标缓冲区
 */
char *OSAL_Strncat(char *dest, const char *src, size_t n);

/**
 * @brief 查找子字符串
 * @param haystack 源字符串
 * @param needle 要查找的子字符串
 * @return 子字符串位置，未找到返回NULL
 */
char *OSAL_Strstr(const char *haystack, const char *needle);

/**
 * @brief 计算字符串前缀长度（不包含指定字符集）
 * @param s 源字符串
 * @param reject 拒绝字符集
 * @return 前缀长度
 */
size_t OSAL_Strcspn(const char *s, const char *reject);

/*===========================================================================
 * 格式化字符串 API
 *===========================================================================*/

/**
 * @brief 格式化输出到字符串
 * @param str 目标缓冲区
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 写入的字符数（不包括'\0'）
 */
int32_t OSAL_Sprintf(char *str, const char *format, ...);

/**
 * @brief 格式化输出到字符串（限定长度）
 * @param str 目标缓冲区
 * @param size 缓冲区大小
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 写入的字符数（不包括'\0'）
 */
int32_t OSAL_Snprintf(char *str, size_t size, const char *format, ...);

/**
 * @brief 格式化输出到字符串（使用va_list）
 * @param str 目标缓冲区
 * @param size 缓冲区大小
 * @param format 格式化字符串
 * @param ap 参数列表
 * @return 写入的字符数（不包括'\0'）
 */
int32_t OSAL_Vsnprintf(char *str, size_t size, const char *format, va_list ap);

/**
 * @brief 从字符串读取格式化输入
 * @param str 源字符串
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 成功读取的项数
 */
int32_t OSAL_Sscanf(const char *str, const char *format, ...);

/*===========================================================================
 * 字符串转换 API
 *===========================================================================*/

/**
 * @brief 字符串转整数
 * @param nptr 字符串
 * @return 整数值
 */
int32_t OSAL_Atoi(const char *nptr);

/**
 * @brief 字符串转长整数
 * @param nptr 字符串
 * @return 长整数值
 */
long OSAL_Atol(const char *nptr);

/**
 * @brief 字符串转长整数（指定进制）
 * @param nptr 字符串
 * @param endptr 结束位置指针（可为NULL）
 * @param base 进制（2-36，0表示自动检测）
 * @return 长整数值
 */
long OSAL_Strtol(const char *nptr, char **endptr, int32_t base);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_STRING_API_H */
