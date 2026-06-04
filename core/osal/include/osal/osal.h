/************************************************************************
 * EMS - Embedded Middleware System
 *
 * 轻量化操作系统抽象层
 *
 * 设计原则:
 * 1. 保留核心API设计
 * 2. 移除不常用的功能(文件系统、模块加载等)
 * 3. 简化实现，减少代码量
 * 4. 保持跨平台兼容性
 ************************************************************************/

#ifndef OSAL_H
#define OSAL_H

/*
 * OSAL版本信息
 */
#define OSAL_LITE_VERSION_MAJOR  1
#define OSAL_LITE_VERSION_MINOR  0
#define OSAL_LITE_VERSION_PATCH  0


/* IPC - 进程间通信 */
#include "osal_platform.h"
#include "osal_types.h"

/* IPC - 进程间通信 */
#include "ipc/osal_atomic_api.h"
#include "ipc/osal_mutex_api.h"
#include "ipc/osal_cond_api.h"
#include "ipc/osal_rwlock_api.h"
#include "ipc/osal_semaphore_api.h"
#include "ipc/osal_shm_api.h"
#include "ipc/osal_shm_cache_api.h"

/* LIB - 标准库封装 */
#include "lib/osal_errno_api.h"
#include "lib/osal_flock_api.h"
#include "lib/osal_heap_api.h"
#include "lib/osal_stdio_api.h"
#include "lib/osal_string_api.h"

/* NET - 网络相关 */
#include "net/osal_socket_api.h"
#include "net/osal_termios_api.h"

/* SYS - 系统调用封装 */
#include "sys/osal_clock_api.h"
#include "sys/osal_env_api.h"
#include "sys/osal_file_api.h"
#include "sys/osal_poll_api.h"
#include "sys/osal_process_api.h"
#include "sys/osal_thread_api.h"
#include "sys/osal_sched_api.h"
#include "sys/osal_select_api.h"
#include "sys/osal_signal_api.h"
#include "sys/osal_time_api.h"

/* UTIL - 工具类 */
#include "util/osal_log_api.h"
#include "util/osal_version_api.h"

#endif /* OSAL_H */
