/************************************************************************
 * OSAL Public API - Main Header
 *
 * This is the main convenience header that includes all OSAL public APIs.
 * Applications should include this header to access OSAL functionality.
 *
 * Module Organization:
 * - osal_types_api.h    : Core types, macros, and constants
 * - osal_errno_api.h    : Error handling
 * - osal_thread_api.h   : Thread management
 * - osal_mutex_api.h    : Mutex locks
 * - osal_semaphore_api.h: Semaphores
 * - osal_rwlock_api.h   : Reader-writer locks
 * - osal_atomic_api.h   : Atomic operations
 * - osal_shm_api.h      : Shared memory
 * - osal_time_api.h     : Time and sleep functions
 * - osal_file_api.h     : File operations
 * - osal_string_api.h   : String and memory manipulation
 * - osal_heap_api.h     : Memory allocation
 * - osal_log_api.h      : Logging system
 * - osal_socket_api.h   : Network sockets
 *
 * Example usage:
 *   #include "osal_api.h"
 *
 *   int main(void) {
 *       osal_thread_t thread;
 *       OSAL_ThreadCreate(&thread, NULL, my_thread_func, NULL);
 *       OSAL_ThreadJoin(thread, NULL);
 *       return 0;
 *   }
 ************************************************************************/

#ifndef OSAL_API_H
#define OSAL_API_H

/* Core types and error handling */
#include "osal_types_api.h"
#include "osal_errno_api.h"

/* Thread synchronization */
#include "osal_thread_api.h"
#include "osal_mutex_api.h"
#include "osal_semaphore_api.h"
#include "osal_rwlock_api.h"
#include "osal_atomic_api.h"

/* Inter-process communication */
#include "osal_shm_api.h"

/* System services */
#include "osal_time_api.h"
#include "osal_file_api.h"
#include "osal_socket_api.h"

/* Utilities */
#include "osal_string_api.h"
#include "osal_heap_api.h"
#include "osal_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OSAL version information
 */
#define OSAL_VERSION_MAJOR 2
#define OSAL_VERSION_MINOR 0
#define OSAL_VERSION_PATCH 0

/**
 * @brief Get OSAL version string
 *
 * @return Version string (e.g., "2.0.0")
 */
static inline const char *OSAL_GetVersion(void)
{
	return "2.0.0";
}

#ifdef __cplusplus
}
#endif

#endif /* OSAL_API_H */
