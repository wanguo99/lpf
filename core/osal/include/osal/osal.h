/************************************************************************
 * ES-Middleware - Embedded Software - Middleware
 *
 * 轻量化操作系统抽象层
 *
 * 设计原则:
 * 1. 保留核心API设计
 * 2. 移除不常用的功能(文件系统、模块加载等)
 * 3. 简化实现，减少代码量
 * 4. 保持跨平台兼容性
 *
 * 说明:
 * - 所有头文件无条件包含（提供完整的类型定义和 API 声明）
 * - 功能裁剪通过 CMakeLists.txt 中的 Kconfig 控制（选择性编译 .c 文件）
 * - 这避免了头文件条件编译导致的类型定义不一致问题
 ************************************************************************/

#ifndef OSAL_H
#define OSAL_H

/*
 * OSAL版本信息
 */
#define OSAL_LITE_VERSION_MAJOR  0x1
#define OSAL_LITE_VERSION_MINOR  0x0
#define OSAL_LITE_VERSION_PATCH  0x0


/* IPC - 进程间通信 */
#include "osal_platform.h"
#include "osal_types.h"

/* IPC - 进程间通信 */
#include "ipc/osal_atomic.h"
#include "ipc/osal_mutex.h"
#include "ipc/osal_cond.h"
#include "ipc/osal_rwlock.h"
#include "ipc/osal_semaphore.h"
#include "ipc/osal_shm.h"
#include "ipc/osal_shm_cache.h"

/* LIB - 标准库封装 */
#include "lib/osal_errno.h"
#include "lib/osal_flock.h"
#include "lib/osal_heap.h"
#include "lib/osal_stdio.h"
#include "lib/osal_string.h"

/* NET - 网络相关 */
#include "net/osal_socket.h"
#include "net/osal_termios.h"

/* SYS - 系统调用封装 */
#include "sys/osal_clock.h"
#include "sys/osal_env.h"
#include "sys/osal_file.h"
#include "sys/osal_poll.h"
#include "sys/osal_process.h"
#include "sys/osal_thread.h"
#include "sys/osal_thread_compat.h"  /* 临时兼容层 */
#include "sys/osal_sched.h"
#include "sys/osal_select.h"
#include "sys/osal_signal.h"
#include "sys/osal_time.h"

/* UTIL - 工具类 */
#include "util/osal_log.h"
#include "util/osal_version.h"
#include "util/osal_crc.h"

#endif /* OSAL_H */
