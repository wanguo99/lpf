/************************************************************************
 * OSAL - select系统调用封装
 *
 * 功能：
 * - 封装POSIX select API和fd_set操作
 * - 1:1映射系统调用，不引入业务逻辑
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 参数使用int32/uint32等固定大小类型
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_SELECT_H
#define OSAL_SELECT_H

#include "osal_types.h"

/*===========================================================================
 * 常量定义
 *===========================================================================*/

/* FD_SETSIZE通常为1024，这里使用固定值 */
#define OSAL_FD_SETSIZE 1024

/* 位操作相关常量 */
#define OSAL_FD_BITS_PER_WORD   32U     /* 每个uint32_t的位数 */

/*===========================================================================
 * 数据结构
 *===========================================================================*/

/**
 * @brief 文件描述符集合
 *
 * 注意：内部实现与平台相关，但接口保持一致
 */
typedef struct {
    uint32_t fds_bits[OSAL_FD_SETSIZE / 32];  /* 位图，每个bit代表一个fd */
} osal_fd_set_t;

/**
 * @brief 时间值结构
 */
typedef struct {
    int32_t tv_sec;   /* 秒 */
    int32_t tv_usec;  /* 微秒 */
} osal_timeval_t;

/*===========================================================================
 * fd_set操作宏
 *===========================================================================*/

/**
 * @brief 清空文件描述符集合
 */
void OSAL_FD_ZERO(osal_fd_set_t *set);

/**
 * @brief 将文件描述符添加到集合
 * @param fd 文件描述符
 * @param set 文件描述符集合
 */
void OSAL_FD_SET(int32_t fd, osal_fd_set_t *set);

/**
 * @brief 从集合中移除文件描述符
 * @param fd 文件描述符
 * @param set 文件描述符集合
 */
void OSAL_FD_CLR(int32_t fd, osal_fd_set_t *set);

/**
 * @brief 检查文件描述符是否在集合中
 * @param fd 文件描述符
 * @param set 文件描述符集合
 * @return 非0表示在集合中，0表示不在
 */
int32_t OSAL_FD_ISSET(int32_t fd, const osal_fd_set_t *set);

/*===========================================================================
 * select系统调用
 *===========================================================================*/

/**
 * @brief 多路复用I/O
 *
 * @param nfds 最大文件描述符+1
 * @param readfds 读文件描述符集合（可为NULL）
 * @param writefds 写文件描述符集合（可为NULL）
 * @param exceptfds 异常文件描述符集合（可为NULL）
 * @param timeout 超时时间（NULL表示永久等待，tv_sec和tv_usec都为0表示立即返回）
 *
 * @return 就绪的文件描述符数量(>0)，0表示超时，-1表示失败
 *
 * @note
 * - readfds/writefds/exceptfds会被修改，返回时只包含就绪的fd
 * - timeout会被修改为剩余时间（Linux特性，其他系统可能不修改）
 */
int32_t OSAL_select(int32_t nfds, osal_fd_set_t *readfds, osal_fd_set_t *writefds,
                  osal_fd_set_t *exceptfds, osal_timeval_t *timeout);

/**
 * @brief pselect系统调用（带信号屏蔽）
 *
 * @param nfds 最大文件描述符+1
 * @param readfds 读文件描述符集合（可为NULL）
 * @param writefds 写文件描述符集合（可为NULL）
 * @param exceptfds 异常文件描述符集合（可为NULL）
 * @param timeout 超时时间（使用timespec结构，精度为纳秒）
 * @param sigmask 信号屏蔽集（可为NULL）
 *
 * @return 就绪的文件描述符数量(>0)，0表示超时，-1表示失败
 *
 * @note
 * - timeout使用timespec结构（秒+纳秒），不会被修改
 * - sigmask在等待期间临时替换进程的信号屏蔽字
 */
typedef struct {
    int32_t tv_sec;   /* 秒 */
    int32_t tv_nsec;  /* 纳秒 */
} osal_timespec_t;

int32_t OSAL_pselect(int32_t nfds, osal_fd_set_t *readfds, osal_fd_set_t *writefds,
                   osal_fd_set_t *exceptfds, const osal_timespec_t *timeout,
                   const void *sigmask);

#endif /* OSAL_SELECT_H */
