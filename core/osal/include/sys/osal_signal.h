/************************************************************************
 * 信号处理API
 ************************************************************************/

#ifndef OSAPI_SIGNAL_H
#define OSAPI_SIGNAL_H

#include "osal_types.h"

/*
 * 信号类型
 */
#define OS_SIGNAL_INT       2   /* SIGINT - 中断信号 (Ctrl+C) */
#define OS_SIGNAL_TERM      15  /* SIGTERM - 终止信号 */
#define OS_SIGNAL_HUP       1   /* SIGHUP - 挂起信号 */
#define OS_SIGNAL_QUIT      3   /* SIGQUIT - 退出信号 */
#define OS_SIGNAL_KILL      9   /* SIGKILL - 强制终止信号 */
#define OS_SIGNAL_USR1      10  /* SIGUSR1 - 用户自定义信号1 */
#define OS_SIGNAL_USR2      12  /* SIGUSR2 - 用户自定义信号2 */

/* POSIX标准信号常量（用于兼容性） */
#define SIGINT      OS_SIGNAL_INT
#define SIGTERM     OS_SIGNAL_TERM
#define SIGHUP      OS_SIGNAL_HUP
#define SIGQUIT     OS_SIGNAL_QUIT
#define SIGKILL     OS_SIGNAL_KILL
#define SIGUSR1     OS_SIGNAL_USR1
#define SIGUSR2     OS_SIGNAL_USR2

/*
 * 信号处理函数类型
 */
typedef void (*os_signal_handler_t)(int32_t signum);

/**
 * @brief 注册信号处理函数
 *
 * @param[in] signum  信号编号 (OS_SIGNAL_*)
 * @param[in] handler 信号处理函数，NULL表示使用默认处理
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER handler为NULL
 * @return OSAL_ERR_GENERIC 注册失败
 */
int32_t OSAL_SignalRegister(int32_t signum, os_signal_handler_t handler);

/**
 * @brief 忽略信号
 *
 * @param[in] signum 信号编号 (OS_SIGNAL_*)
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 设置失败
 */
int32_t OSAL_SignalIgnore(int32_t signum);

/**
 * @brief 恢复信号的默认处理
 *
 * @param[in] signum 信号编号 (OS_SIGNAL_*)
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 恢复失败
 */
int32_t OSAL_SignalDefault(int32_t signum);

/**
 * @brief 阻塞信号
 *
 * @param[in] signum 信号编号 (OS_SIGNAL_*)
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 阻塞失败
 */
int32_t OSAL_SignalBlock(int32_t signum);

/**
 * @brief 解除信号阻塞
 *
 * @param[in] signum 信号编号 (OS_SIGNAL_*)
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 解除失败
 */
int32_t OSAL_SignalUnblock(int32_t signum);

#endif /* OSAPI_SIGNAL_H */
