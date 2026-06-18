/**
 * @file osal_version.h
 * @brief OSAL版本信息和构建信息API
 *
 * 提供ES-Middleware的版本、构建信息查询接口，以及便利宏定义
 * 类似Linux内核的版本信息机制
 */

#ifndef OSAL_VERSION_H
#define OSAL_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Convenience Macros (similar to kernel's __FILE__, __LINE__, etc.)
 * ======================================================================== */

/**
 * @brief 当前软件版本号
 * 可在任何模块中使用，获取ES-Middleware版本
 */
#define OSAL_VERSION osal_get_version()

/**
 * @brief 当前软件完整版本（版本号+Git）
 */
#define OSAL_VERSION_FULL osal_get_version_full()

/**
 * @brief 当前Git commit
 */
#define OSAL_GIT_COMMIT osal_get_git_commit()

/**
 * @brief 构建时间戳
 */
#define OSAL_BUILD_TIME osal_get_build_time()

/**
 * @brief 当前文件名（内核风格）
 */
#define OSAL_FILE __FILE__

/**
 * @brief 当前行号（内核风格）
 */
#define OSAL_LINE __LINE__

/**
 * @brief 当前函数名（内核风格）
 */
#define OSAL_FUNC __func__

/**
 * @brief 格式化位置信息宏
 * 输出格式："文件名:行号:函数名"
 */
#define OSAL_LOCATION __FILE__ ":" OSAL_STRINGIFY(__LINE__) ":" __func__

/**
 * @brief 带版本信息的位置宏
 * 输出格式："[版本号] 文件名:行号:函数名"
 */
#define OSAL_VERSION_LOCATION "[" OSAL_VERSION "] " OSAL_LOCATION

/* Helper macro for stringification */
#define OSAL_STRINGIFY(x) OSAL_STRINGIFY_HELPER(x)
#define OSAL_STRINGIFY_HELPER(x) #x

/* ========================================================================
 * Version Query APIs
 * ======================================================================== */

/**
 * @brief 获取OSAL版本字符串
 * @return 版本字符串（格式："OSAL vX.Y.Z"）
 */
const char *osal_get_version_string(void);

/**
 * @brief 获取ES-Middleware版本号
 * @return 版本号字符串（如 "1.0.0"）
 */
const char *osal_get_version(void);

/**
 * @brief 获取完整版本字符串（版本号 + Git commit）
 * @return 完整版本字符串（如 "1.0.0-abc1234"）
 */
const char *osal_get_version_full(void);

/**
 * @brief 获取Git commit哈希
 * @return Git commit短哈希（如 "abc1234" 或 "abc1234-dirty"）
 */
const char *osal_get_git_commit(void);

/**
 * @brief 获取构建时间戳
 * @return 构建时间字符串（如 "2024-06-14 10:30:00 UTC"）
 */
const char *osal_get_build_time(void);

/**
 * @brief 获取构建用户和主机
 * @return 构建者信息（如 "user@hostname"）
 */
const char *osal_get_build_by(void);

/**
 * @brief 获取编译器版本
 * @return 编译器版本字符串
 */
const char *osal_get_compiler(void);

/**
 * @brief 获取目标架构
 * @return 架构字符串（如 "x86_64", "aarch64"）
 */
const char *osal_get_arch(void);

/**
 * @brief 获取内核版本
 * @return 内核版本字符串
 */
const char *osal_get_kernel(void);

/**
 * @brief 获取完整的版本横幅（类似Linux内核启动信息）
 * @return 版本横幅字符串
 */
const char *osal_get_banner(void);

/**
 * @brief 打印详细的版本信息到控制台
 * 使用OSAL_printf输出所有版本和构建信息
 */
void osal_print_version_info(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_VERSION_H */
