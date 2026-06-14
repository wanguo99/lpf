/************************************************************************
 * OSAL - 进程管理接口
 *
 * 功能：
 * - 封装进程创建、等待、信号等操作
 * - 支持多进程架构
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 提供标准进程管理函数的封装
 * - 返回值统一使用OSAL错误码
 * - 便于RTOS移植（RTOS可能不支持进程）
 ************************************************************************/

#ifndef OSAL_PROCESS_H
#define OSAL_PROCESS_H


/* 基础进程控制函数（保持兼容） */
void OSAL_exit(int32_t status);
int32_t OSAL_getpid(void);
int32_t OSAL_getppid(void);
int32_t OSAL_kill(int32_t pid, int32_t sig);
void OSAL_abort(void);
int32_t OSAL_fork(void);
int32_t OSAL_execvp(const char *file, char *const argv[]);
int32_t OSAL_waitpid(int32_t pid, int32_t *status, int32_t options);

/* 等待选项 */
#define OSAL_WNOHANG  0x01  /* 非阻塞等待 */

#endif /* OSAL_PROCESS_H */
