/************************************************************************
 * OSAL - 环境变量操作封装
 *
 * 功能：
 * - 封装环境变量相关函数（getenv/setenv/unsetenv）
 * - 1:1映射系统调用，不引入业务逻辑
 *
 * 设计原则：
 * - 使用标准char类型表示字符串
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_ENV_H
#define OSAL_ENV_H

#include "osal_types.h"

/*===========================================================================
 * 环境变量操作
 *===========================================================================*/

/**
 * @brief 获取环境变量值
 * @param name 环境变量名
 * @return 环境变量值（字符串指针），不存在返回NULL
 * @note 返回的指针指向静态存储区，不要修改或释放
 */
char *OSAL_getenv(const char *name);

/**
 * @brief 设置环境变量
 * @param name 环境变量名
 * @param value 环境变量值
 * @param overwrite 是否覆盖已存在的变量（1=覆盖，0=不覆盖）
 * @return 0成功，-1失败
 */
int32_t OSAL_setenv(const char *name, const char *value, int32_t overwrite);

/**
 * @brief 删除环境变量
 * @param name 环境变量名
 * @return 0成功，-1失败
 */
int32_t OSAL_unsetenv(const char *name);

#endif /* OSAL_ENV_H */
