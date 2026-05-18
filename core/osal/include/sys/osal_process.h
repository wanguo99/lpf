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

#include "osal_types.h"
#include <stdbool.h>

/* 基础进程控制函数（保持兼容） */
void OSAL_Exit(int32_t status);
int32_t OSAL_Getpid(void);
int32_t OSAL_Kill(int32_t pid, int32_t sig);
void OSAL_Abort(void);

/* 进程管理接口（新增，支持多进程架构） */

/**
 * @brief 创建子进程
 *
 * @param[out] proc_id  进程ID
 * @param[in]  path     可执行文件路径
 * @param[in]  argv     参数列表（NULL结尾）
 * @param[in]  envp     环境变量（NULL结尾，NULL表示继承父进程）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_FREE_IDS 无法创建进程
 */
int32_t OSAL_ProcessCreate(osal_id_t *proc_id, const char *path,
                           char *const argv[], char *const envp[]);

/**
 * @brief 等待子进程退出
 *
 * @param[in]  proc_id     进程ID
 * @param[out] status      退出状态码（可选，NULL表示不关心）
 * @param[in]  timeout_ms  超时时间（0表示不等待，其他值表示阻塞等待）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 进程未退出（仅timeout_ms=0时）
 * @return OSAL_ERR_INVALID_ID 进程不存在
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 不支持带超时的等待，如需超时功能请在上层实现轮询
 */
int32_t OSAL_ProcessWait(osal_id_t proc_id, int32_t *status, int32_t timeout_ms);

/**
 * @brief 发送信号给进程
 *
 * @param[in] proc_id  进程ID
 * @param[in] signal   信号编号（SIGTERM/SIGKILL等）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32_t OSAL_ProcessKill(osal_id_t proc_id, int32_t signal);

/**
 * @brief 检查进程是否存在
 *
 * @param[in] proc_id  进程ID
 *
 * @return true 进程存在
 * @return false 进程不存在
 */
bool OSAL_ProcessExists(osal_id_t proc_id);

/**
 * @brief 获取当前进程ID
 *
 * @return 进程ID
 */
osal_id_t OSAL_ProcessGetId(void);

/**
 * @brief 获取父进程ID
 *
 * @return 父进程ID
 */
osal_id_t OSAL_ProcessGetParentId(void);

#endif /* OSAL_PROCESS_H */
