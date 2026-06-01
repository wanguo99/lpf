/************************************************************************
 * OSAL - 套接字操作封装
 *
 * 功能：
 * - 封装POSIX socket函数（socket/bind/setsockopt等）
 * - 提供跨平台的网络编程接口
 * - 1:1映射系统调用，不引入业务逻辑
 *
 * 设计原则：
 * - 使用固定大小类型，避免平台相关类型
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_SOCKET_H
#define OSAL_SOCKET_H

#include "osal_platform.h"
#include "osal_types.h"

/*===========================================================================
 * 地址族（Address Family）
 *===========================================================================*/

#define OSAL_AF_UNSPEC   0   /* 未指定 */
#define OSAL_AF_UNIX     1   /* Unix域套接字 */
#define OSAL_AF_INET     2   /* IPv4 */
#define OSAL_AF_INET6    10  /* IPv6 */
#define OSAL_AF_CAN      29  /* CAN总线 */

/*===========================================================================
 * 套接字类型
 *===========================================================================*/

#define OSAL_SOCK_STREAM    1  /* 流式套接字（TCP） */
#define OSAL_SOCK_DGRAM     2  /* 数据报套接字（UDP） */
#define OSAL_SOCK_RAW       3  /* 原始套接字 */
#define OSAL_SOCK_SEQPACKET 5  /* 有序分组套接字 */

/*===========================================================================
 * 协议
 *===========================================================================*/

#define OSAL_IPPROTO_IP      0   /* IP协议 */
#define OSAL_IPPROTO_ICMP    1   /* ICMP协议 */
#define OSAL_IPPROTO_TCP     6   /* TCP协议 */
#define OSAL_IPPROTO_UDP     17  /* UDP协议 */
#define OSAL_IPPROTO_RAW     255 /* 原始IP协议 */
#define OSAL_CAN_RAW         1   /* CAN原始协议 */

/*===========================================================================
 * 套接字选项级别
 *===========================================================================*/

#define OSAL_SOL_SOCKET      1   /* 套接字级别选项 */
#define OSAL_SOL_CAN_RAW     101 /* CAN原始套接字选项 */

/*===========================================================================
 * 套接字选项名称
 *===========================================================================*/

/* SOL_SOCKET级别选项 */
#define OSAL_SO_REUSEADDR    2   /* 允许重用本地地址 */
#define OSAL_SO_KEEPALIVE    9   /* 保持连接 */
#define OSAL_SO_BROADCAST    6   /* 允许广播 */
#define OSAL_SO_RCVBUF       8   /* 接收缓冲区大小 */
#define OSAL_SO_SNDBUF       7   /* 发送缓冲区大小 */
#define OSAL_SO_RCVTIMEO     20  /* 接收超时 */
#define OSAL_SO_SNDTIMEO     21  /* 发送超时 */

/* CAN特定选项 */
#define OSAL_CAN_RAW_FILTER  1   /* CAN过滤器 */
#define OSAL_CAN_RAW_LOOPBACK 3  /* CAN环回 */

/*===========================================================================
 * 套接字地址结构（不透明类型）
 *===========================================================================*/

/**
 * @brief 通用套接字地址结构（不透明类型）
 *
 * 注意：此结构体大小必须足够容纳各种地址类型
 * - struct sockaddr_in (16字节)
 * - struct sockaddr_in6 (28字节)
 * - struct sockaddr_can (16字节)
 * - struct sockaddr_un (110字节)
 */
typedef struct {
    uint8_t opaque[128];  /* 不透明数据，足够容纳各种sockaddr */
} osal_sockaddr_t;

/*===========================================================================
 * 套接字操作
 *===========================================================================*/

/**
 * @brief 创建套接字
 * @param domain 地址族（OSAL_AF_INET/AF_CAN等）
 * @param type 套接字类型（OSAL_SOCK_STREAM/SOCK_DGRAM/SOCK_RAW等）
 * @param protocol 协议（OSAL_IPPROTO_TCP/CAN_RAW等，0表示自动选择）
 * @return 套接字描述符(>=0)，失败返回-1
 */
int32_t OSAL_socket(int32_t domain, int32_t type, int32_t protocol);

/**
 * @brief 绑定套接字到地址
 * @param sockfd 套接字描述符
 * @param addr 地址结构指针
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int32_t OSAL_bind(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen);

/**
 * @brief 设置套接字选项
 * @param sockfd 套接字描述符
 * @param level 选项级别（OSAL_SOL_SOCKET/SOL_CAN_RAW等）
 * @param optname 选项名称（OSAL_SO_REUSEADDR/CAN_RAW_FILTER等）
 * @param optval 选项值指针
 * @param optlen 选项值长度
 * @return 0成功，-1失败
 */
int32_t OSAL_setsockopt(int32_t sockfd, int32_t level, int32_t optname,
                        const void *optval, uint32_t optlen);

#endif /* OSAL_SOCKET_H */
