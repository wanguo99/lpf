/************************************************************************
 * OSAL日志API头文件
 *
 * 功能特性：
 * - 分级日志（DEBUG/INFO/WARN/ERROR/FATAL）
 * - 模块级日志控制
 * - 结构化日志支持
 * - 日志过滤和采样
 * - 远程日志传输（syslog/UDP）
 * - 自动日志轮转
 ************************************************************************/

#ifndef OSAL_LOG_H
#define OSAL_LOG_H

/*
 * 日志级别
 */
#define OS_LOG_LEVEL_DEBUG  0x0
#define OS_LOG_LEVEL_INFO   0x1
#define OS_LOG_LEVEL_WARN   0x2
#define OS_LOG_LEVEL_ERROR  0x3
#define OS_LOG_LEVEL_FATAL  0x4

/* 日志配置 */
#define OSAL_LOG_DEFAULT_LEVEL       OS_LOG_LEVEL_INFO
#define OSAL_LOG_FILE_PATH           "/var/log/ems.log"
#define OSAL_LOG_FILE_MAX_SIZE_MB    0xA
#define OSAL_LOG_FILE_BACKUP_COUNT   0x5

/* 日志缓冲区大小 */
#define OSAL_LOG_PATH_SIZE           0x100U   /* 日志路径缓冲区大小 */
#define OSAL_LOG_FILENAME_SIZE       0x200U   /* 日志文件名缓冲区大小 */
#define OSAL_LOG_TIMESTAMP_SIZE      0x40U    /* 时间戳缓冲区大小 */
#define OSAL_LOG_MESSAGE_SIZE        0x400U  /* 日志消息缓冲区大小 */
#define OSAL_LOG_MAX_MODULES         0x20U    /* 最大模块数 */
#define OSAL_LOG_MAX_KV_PAIRS        0x8U     /* 结构化日志最大键值对数 */

/*
 * 日志模块枚举
 */
typedef enum {
    LOG_MODULE_OSAL = 0x0,
    LOG_MODULE_HAL,
    LOG_MODULE_PCL,
    LOG_MODULE_PDL,
    LOG_MODULE_ACL,
    LOG_MODULE_APP,
    LOG_MODULE_TEST,
    LOG_MODULE_MAX
} log_module_t;

/*
 * 结构化日志键值对
 */
typedef struct {
    const char *key;
    const char *value;
} log_kv_pair_t;

/**
 * @brief 初始化日志系统
 *
 * @param[in] log_file_path 日志文件路径，NULL表示只输出到终端
 * @param[in] level 日志级别
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t OSAL_LogInit(const char *log_file_path, int32_t level);

/**
 * @brief 关闭日志系统
 */
void OSAL_LogShutdown(void);

/**
 * @brief 设置日志级别
 *
 * @param[in] level 日志级别
 */
void OSAL_LogSetLevel(int32_t level);

/**
 * @brief 设置日志文件最大大小
 *
 * @param[in] size_bytes 最大文件大小（字节）
 */
void OSAL_LogSetMaxFileSize(uint32_t size_bytes);

/**
 * @brief 设置最大日志文件数
 *
 * @param[in] max_files 最大备份文件数
 */
void OSAL_LogSetMaxFiles(uint32_t max_files);

/**
 * @brief 设置模块日志级别
 *
 * @param[in] module 模块ID
 * @param[in] level 日志级别
 */
void OSAL_LogSetModuleLevel(log_module_t module, int32_t level);

/**
 * @brief 获取模块日志级别
 *
 * @param[in] module 模块ID
 * @return 日志级别
 */
int32_t OSAL_LogGetModuleLevel(log_module_t module);

/**
 * @brief 设置日志过滤器（正则表达式）
 *
 * @param[in] pattern 过滤模式，NULL表示清除过滤器
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_LogSetFilter(const char *pattern);

/**
 * @brief 设置日志采样率
 *
 * @param[in] rate 采样率（1表示全部记录，N表示每N条记录1条）
 */
void OSAL_LogSetSampling(uint32_t rate);

/**
 * @brief 启用远程日志（syslog/UDP）
 *
 * @param[in] host 远程主机地址
 * @param[in] port 远程端口
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_LogSetRemote(const char *host, uint16_t port);

/**
 * @brief 禁用远程日志
 */
void OSAL_LogDisableRemote(void);

/**
 * @brief 通用日志函数
 *
 * @param[in] level 日志级别
 * @param[in] module 模块名称
 * @param[in] format 格式化字符串
 * @param[in] ... 可变参数
 */
void OSAL_Log(int32_t level, const char *module, const char *format, ...);

/**
 * @brief 结构化日志函数
 *
 * @param[in] level 日志级别
 * @param[in] module 模块ID
 * @param[in] message 日志消息
 * @param[in] kv_pairs 键值对数组
 * @param[in] kv_count 键值对数量
 */
void OSAL_LogStructured(int32_t level, log_module_t module, const char *message,
                        const log_kv_pair_t *kv_pairs, uint32_t kv_count);

/**
 * @brief 简单打印（不带日志级别和模块名）
 */
void OSAL_Printf(const char *format, ...);

/**
 * @brief 获取日志统计信息
 *
 * @param[out] total_count 总日志数
 * @param[out] dropped_count 丢弃的日志数（采样）
 */
void OSAL_LogGetStats(uint64_t *total_count, uint64_t *dropped_count);

/*
 * 编译时日志级别控制
 * 可通过 Kconfig 配置 OSAL_LOG_COMPILE_LEVEL
 * 低于此级别的日志在编译时被完全优化掉（零开销）
 */
#ifndef OSAL_LOG_COMPILE_LEVEL
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_DEBUG  /* 默认编译所有级别 */
#endif

/*
 * 统一的底层日志实现函数
 * 所有日志宏最终调用此函数
 */
void OSAL_LogEmit(int32_t level, const char *module,
                  const char *file, const char *func, int32_t line,
                  const char *format, ...) __attribute__((format(printf, 6, 7)));

/*
 * 日志宏定义（推荐使用）
 *
 * 设计说明：
 * 1. 两级过滤：编译时级别检查 + 运行时级别检查
 * 2. 编译时优化：低于 OSAL_LOG_COMPILE_LEVEL 的日志被完全移除
 * 3. 运行时快速路径：级别不够时立即返回，不进行格式化
 * 4. 自动添加文件名、函数名、行号信息用于调试
 *
 * 参考：Linux kernel pr_debug/pr_info 设计模式
 */
#define LOG_DEBUG(module, ...) \
    do { \
        if (OS_LOG_LEVEL_DEBUG >= OSAL_LOG_COMPILE_LEVEL) \
            OSAL_LogEmit(OS_LOG_LEVEL_DEBUG, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

#define LOG_INFO(module, ...) \
    do { \
        if (OS_LOG_LEVEL_INFO >= OSAL_LOG_COMPILE_LEVEL) \
            OSAL_LogEmit(OS_LOG_LEVEL_INFO, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

#define LOG_WARN(module, ...) \
    do { \
        if (OS_LOG_LEVEL_WARN >= OSAL_LOG_COMPILE_LEVEL) \
            OSAL_LogEmit(OS_LOG_LEVEL_WARN, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

#define LOG_ERROR(module, ...) \
    do { \
        if (OS_LOG_LEVEL_ERROR >= OSAL_LOG_COMPILE_LEVEL) \
            OSAL_LogEmit(OS_LOG_LEVEL_ERROR, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

#define LOG_FATAL(module, ...) \
    do { \
        if (OS_LOG_LEVEL_FATAL >= OSAL_LOG_COMPILE_LEVEL) \
            OSAL_LogEmit(OS_LOG_LEVEL_FATAL, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while(0)

/*
 * 仅打印一次的日志宏（防止日志洪水）
 * 使用静态变量确保同一日志点只输出一次
 */
#define LOG_DEBUG_ONCE(module, ...) \
    do { \
        static uint8_t __logged = 0; \
        if (!__logged && OS_LOG_LEVEL_DEBUG >= OSAL_LOG_COMPILE_LEVEL) { \
            __logged = 1; \
            OSAL_LogEmit(OS_LOG_LEVEL_DEBUG, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN_ONCE(module, ...) \
    do { \
        static uint8_t __logged = 0; \
        if (!__logged && OS_LOG_LEVEL_WARN >= OSAL_LOG_COMPILE_LEVEL) { \
            __logged = 1; \
            OSAL_LogEmit(OS_LOG_LEVEL_WARN, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR_ONCE(module, ...) \
    do { \
        static uint8_t __logged = 0; \
        if (!__logged && OS_LOG_LEVEL_ERROR >= OSAL_LOG_COMPILE_LEVEL) { \
            __logged = 1; \
            OSAL_LogEmit(OS_LOG_LEVEL_ERROR, module, __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

/*
 * 结构化日志宏（便捷接口）
 * 使用示例：
 *   LOG_STRUCTURED(LOG_MODULE_PDL, "channel_switch",
 *                  "from", "ethernet",
 *                  "to", "uart",
 *                  "reason", "timeout");
 */
#define LOG_STRUCTURED(module, msg, ...) \
    do { \
        log_kv_pair_t __kv_pairs[] = { __VA_ARGS__ }; \
        OSAL_LogStructured(OS_LOG_LEVEL_INFO, module, msg, __kv_pairs, \
                          sizeof(__kv_pairs) / sizeof(log_kv_pair_t)); \
    } while(0)

#endif /* OSAL_LOG_H */
