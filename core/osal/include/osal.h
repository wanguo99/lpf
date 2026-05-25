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

#include "osal_platform.h"
#include "osal_types.h"

/* IPC - 进程间通信 */
#include "ipc/osal_mutex.h"
#include "ipc/osal_semaphore.h"
#include "ipc/osal_cond.h"
#include "ipc/osal_atomic.h"
#include "ipc/osal_shm_cache.h"

/* SYS - 系统调用封装 */
#include "sys/osal_clock.h"
#include "sys/osal_signal.h"
#include "sys/osal_file.h"
#include "sys/osal_select.h"
#include "sys/osal_env.h"
#include "sys/osal_time.h"
#include "sys/osal_process.h"
#include "sys/osal_thread.h"
#include "sys/osal_sched.h"

/* NET - 网络相关 */
#include "net/osal_socket.h"
#include "net/osal_termios.h"

/* LIB - 标准库封装 */
#include "lib/osal_string.h"
#include "lib/osal_stdio.h"
#include "lib/osal_heap.h"
#include "lib/osal_errno.h"

/* UTIL - 工具类 */
#include "util/osal_log.h"
#include "util/osal_version.h"

/*
 * OSAL版本信息
 */
#define OSAL_LITE_VERSION_MAJOR  1
#define OSAL_LITE_VERSION_MINOR  0
#define OSAL_LITE_VERSION_PATCH  0

#endif /* OSAL_H */
