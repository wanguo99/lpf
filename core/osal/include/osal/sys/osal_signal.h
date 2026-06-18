/************************************************************************
 * OSAL Signal API
 *
 * 提供进程信号处理的统一访问接口。
 ************************************************************************/

#ifndef OSAL_SIGNAL_H
#define OSAL_SIGNAL_H

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 信号类型定义
 *===========================================================================*/

/**
 * @brief 信号处理函数类型
 */
typedef void (*osal_sighandler_t)(int);

/**
 * @brief 设置信号处理函数
 *
 * @param[in] signum 信号编号（SIGINT/SIGTERM/etc）
 * @param[in] handler 处理函数（SIG_IGN/SIG_DFL 或自定义函数）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_signal(int32_t signum, osal_sighandler_t handler);

/**
 * @brief 发送信号到进程
 *
 * @param[in] pid 进程 ID
 * @param[in] sig 信号编号
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_kill(osal_pid_t pid, int32_t sig);

/**
 * @brief 发送信号到当前进程
 *
 * @param[in] sig 信号编号
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_raise(int32_t sig);

/**
 * @brief 阻塞信号集操作
 *
 * @param[in] how 操作方式（SIG_BLOCK/SIG_UNBLOCK/SIG_SETMASK）
 * @param[in] set 信号集（可为 NULL）
 * @param[out] oldset 旧信号集（可为 NULL）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigprocmask(int32_t how, const sigset_t *set, sigset_t *oldset);

/**
 * @brief 初始化信号集为空
 *
 * @param[out] set 信号集指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigemptyset(sigset_t *set);

/**
 * @brief 初始化信号集为全集
 *
 * @param[out] set 信号集指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigfillset(sigset_t *set);

/**
 * @brief 添加信号到信号集
 *
 * @param[in] set 信号集指针
 * @param[in] signum 信号编号
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigaddset(sigset_t *set, int32_t signum);

/**
 * @brief 从信号集删除信号
 *
 * @param[in] set 信号集指针
 * @param[in] signum 信号编号
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigdelset(sigset_t *set, int32_t signum);

/**
 * @brief 测试信号是否在信号集中
 *
 * @param[in] set 信号集指针
 * @param[in] signum 信号编号
 * @return 1 信号在集合中
 * @return 0 信号不在集合中
 * @return -1 失败
 */
int32_t osal_sigismember(const sigset_t *set, int32_t signum);

/**
 * @brief 等待信号
 *
 * @param[in] set 等待的信号集
 * @param[out] sig 接收到的信号编号（可为 NULL）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigwait(const sigset_t *set, int32_t *sig);

/**
 * @brief 高级信号处理（sigaction）
 *
 * @param[in] signum 信号编号
 * @param[in] act 新的信号动作（可为 NULL）
 * @param[out] oldact 旧的信号动作（可为 NULL）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sigaction(int32_t signum, const struct sigaction *act,
					   struct sigaction *oldact);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SIGNAL_H */
