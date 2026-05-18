/**
 * @file osal_version.h
 * @brief OSAL版本信息
 */

#ifndef OSAL_VERSION_H
#define OSAL_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取OSAL版本字符串
 * @return 版本字符串（格式："OSAL vX.Y.Z"）
 */
const char *OSAL_GetVersionString(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_VERSION_H */
