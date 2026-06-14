/**
 * @file osal_version.c
 * @brief OSAL版本信息实现
 */

#include "osal.h"
#include "version.h"
#include <stdio.h>

/**
 * @brief 获取OSAL版本字符串
 * @return 版本字符串（格式："OSAL vX.Y.Z"）
 */
const char *OSAL_GetVersionString(void)
{
    static char version_buf[64];
    snprintf(version_buf, sizeof(version_buf), "OSAL v%s", ES_MIDDLEWARE_VERSION);
    return version_buf;
}

/**
 * @brief 打印ES-Middleware版本信息
 */
void print_version_info(void)
{
    printf("=== ES-Middleware Version Information ===\n");
    printf("Version:      %s\n", ES_MIDDLEWARE_VERSION);
    printf("Git Commit:   %s\n", ES_MIDDLEWARE_GIT_COMMIT);
    printf("Build Time:   %s\n", ES_MIDDLEWARE_COMPILE_TIME);
    printf("Built by:     %s@%s\n", ES_MIDDLEWARE_COMPILE_BY, ES_MIDDLEWARE_COMPILE_HOST);
    printf("Compiler:     %s\n", ES_MIDDLEWARE_COMPILER);
    printf("Architecture: %s\n", ES_MIDDLEWARE_BUILD_ARCH);
    printf("Kernel:       %s\n", ES_MIDDLEWARE_BUILD_KERNEL);
    printf("==========================================\n");
}
