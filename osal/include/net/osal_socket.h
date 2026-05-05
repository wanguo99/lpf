/************************************************************************
 * OSAL - socket系统调用封装
 *
 * 功能：
 * - 封装POSIX socket API
 * - 1:1映射系统调用，不引入业务逻辑
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 参数使用int32/uint32等固定大小类型
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_SOCKET_H
#define OSAL_SOCKET_H

#include "osal_types.h"

/*===========================================================================
 * 地址族（Address Family）
 *===========================================================================*/
#define OSAL_AF_UNSPEC    0   /* 未指定 */
#define OSAL_AF_INET      2   /* IPv4 */
#define OSAL_AF_INET6     10  /* IPv6 */
#define OSAL_AF_CAN       29  /* CAN总线 */
#define OSAL_AF_PACKET    17  /* 原始数据包 */

/* 协议族（Protocol Family）与地址族相同 */
#define OSAL_PF_UNSPEC    OSAL_AF_UNSPEC
#define OSAL_PF_INET      OSAL_AF_INET
#define OSAL_PF_INET6     OSAL_AF_INET6
#define OSAL_PF_CAN       OSAL_AF_CAN
#define OSAL_PF_PACKET    OSAL_AF_PACKET

/*===========================================================================
 * Socket类型
 *===========================================================================*/
#define OSAL_SOCK_STREAM    1  /* TCP流式socket */
#define OSAL_SOCK_DGRAM     2  /* UDP数据报socket */
#define OSAL_SOCK_RAW       3  /* 原始socket */

/*===========================================================================
 * 协议类型
 *===========================================================================*/
#define OSAL_IPPROTO_IP      0   /* IP协议 */
#define OSAL_IPPROTO_TCP     6   /* TCP协议 */
#define OSAL_IPPROTO_UDP     17  /* UDP协议 */
#define OSAL_IPPROTO_RAW     255 /* 原始IP协议 */
#define OSAL_CAN_RAW         1   /* CAN原始协议 */

/*===========================================================================
 * Socket选项级别
 *===========================================================================*/
#define OSAL_SOL_SOCKET      1    /* Socket层选项 */
#define OSAL_SOL_CAN_RAW     101  /* CAN RAW层选项 */
#define OSAL_IPPROTO_TCP     6    /* TCP协议层选项 */

/*===========================================================================
 * Socket选项名称
 *===========================================================================*/
#define OSAL_SO_REUSEADDR    2   /* 允许重用本地地址 */
#define OSAL_SO_KEEPALIVE    9   /* 保持连接 */
#define OSAL_SO_BROADCAST    6   /* 允许广播 */
#define OSAL_SO_RCVBUF       8   /* 接收缓冲区大小 */
#define OSAL_SO_SNDBUF       7   /* 发送缓冲区大小 */
#define OSAL_SO_RCVTIMEO     20  /* 接收超时 */
#define OSAL_SO_SNDTIMEO     21  /* 发送超时 */
#define OSAL_SO_ERROR        4   /* 获取错误状态 */

/* TCP特定选项 */
#define OSAL_TCP_NODELAY     1   /* 禁用Nagle算法 */

/* CAN特定选项 */
#define OSAL_CAN_RAW_FILTER  1   /* CAN过滤器 */

/*===========================================================================
 * shutdown参数
 *===========================================================================*/
#define OSAL_SHUT_RD    0  /* 关闭读 */
#define OSAL_SHUT_WR    1  /* 关闭写 */
#define OSAL_SHUT_RDWR  2  /* 关闭读写 */

/*===========================================================================
 * send/recv标志
 *===========================================================================*/
#define OSAL_MSG_OOB        0x01  /* 带外数据 */
#define OSAL_MSG_PEEK       0x02  /* 查看数据但不移除 */
#define OSAL_MSG_DONTROUTE  0x04  /* 不使用路由 */
#define OSAL_MSG_WAITALL    0x100 /* 等待所有数据 */
#define OSAL_MSG_NOSIGNAL   0x4000 /* 不产生SIGPIPE信号 */

/*===========================================================================
 * 地址结构（通用）
 *===========================================================================*/

/* 通用socket地址（最大128字节） */
typedef struct {
    uint16_t sa_family;      /* 地址族 */
    uint8_t  sa_data[126];   /* 地址数据 */
} osal_sockaddr_t;

/* IPv4地址结构 */
typedef struct {
    uint16_t sin_family;     /* AF_INET */
    uint16_t sin_port;       /* 端口号（网络字节序） */
    uint32_t sin_addr;       /* IP地址（网络字节序） */
    uint8_t  sin_zero[8];    /* 填充 */
} osal_sockaddr_in_t;

/* IPv6地址结构 */
typedef struct {
    uint16_t sin6_family;    /* AF_INET6 */
    uint16_t sin6_port;      /* 端口号（网络字节序） */
    uint32_t sin6_flowinfo;  /* 流信息 */
    uint8_t  sin6_addr[16];  /* IPv6地址 */
    uint32_t sin6_scope_id;  /* 作用域ID */
} osal_sockaddr_in6_t;

/* CAN地址结构 */
typedef struct {
    uint16_t can_family;     /* AF_CAN */
    uint32_t can_ifindex;    /* CAN接口索引 */
} osal_sockaddr_can_t;

/*===========================================================================
 * Socket基本操作
 *===========================================================================*/

/**
 * @brief 创建socket
 * @param domain 协议族（OSAL_AF_*）
 * @param type Socket类型（OSAL_SOCK_*）
 * @param protocol 协议（OSAL_IPPROTO_*或OSAL_CAN_RAW）
 * @return socket描述符(>=0)，失败返回-1
 */
int32_t OSAL_socket(int32_t domain, int32_t type, int32_t protocol);

/**
 * @brief 绑定地址
 * @param sockfd socket描述符
 * @param addr 地址结构指针
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int32_t OSAL_bind(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen);

/**
 * @brief 监听连接
 * @param sockfd socket描述符
 * @param backlog 等待队列最大长度
 * @return 0成功，-1失败
 */
int32_t OSAL_listen(int32_t sockfd, int32_t backlog);

/**
 * @brief 接受连接
 * @param sockfd socket描述符
 * @param addr 客户端地址结构指针（可为NULL）
 * @param addrlen 地址结构长度指针（可为NULL）
 * @return 新socket描述符(>=0)，失败返回-1
 */
int32_t OSAL_accept(int32_t sockfd, osal_sockaddr_t *addr, uint32_t *addrlen);

/**
 * @brief 连接到服务器
 * @param sockfd socket描述符
 * @param addr 服务器地址结构指针
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int32_t OSAL_connect(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen);

/**
 * @brief 发送数据
 * @param sockfd socket描述符
 * @param buf 数据缓冲区
 * @param len 数据长度（最大4GB）
 * @param flags 标志（通常为0）
 * @return 实际发送字节数(>=0)，-1失败
 */
int32_t OSAL_send(int32_t sockfd, const void *buf, uint32_t len, int32_t flags);

/**
 * @brief 接收数据
 * @param sockfd socket描述符
 * @param buf 数据缓冲区
 * @param len 缓冲区长度（最大4GB）
 * @param flags 标志（通常为0）
 * @return 实际接收字节数(>=0)，0表示连接关闭，-1失败
 */
int32_t OSAL_recv(int32_t sockfd, void *buf, uint32_t len, int32_t flags);

/**
 * @brief 发送数据到指定地址
 * @param sockfd socket描述符
 * @param buf 数据缓冲区
 * @param len 数据长度（最大4GB）
 * @param flags 标志（通常为0）
 * @param dest_addr 目标地址结构指针
 * @param addrlen 地址结构长度
 * @return 实际发送字节数(>=0)，-1失败
 */
int32_t OSAL_sendto(int32_t sockfd, const void *buf, uint32_t len, int32_t flags,
                  const osal_sockaddr_t *dest_addr, uint32_t addrlen);

/**
 * @brief 从指定地址接收数据
 * @param sockfd socket描述符
 * @param buf 数据缓冲区
 * @param len 缓冲区长度（最大4GB）
 * @param flags 标志（通常为0）
 * @param src_addr 源地址结构指针（可为NULL）
 * @param addrlen 地址结构长度指针（可为NULL）
 * @return 实际接收字节数(>=0)，-1失败
 */
int32_t OSAL_recvfrom(int32_t sockfd, void *buf, uint32_t len, int32_t flags,
                    osal_sockaddr_t *src_addr, uint32_t *addrlen);

/**
 * @brief 关闭socket的部分功能
 * @param sockfd socket描述符
 * @param how 关闭方式（OSAL_SHUT_RD/WR/RDWR）
 * @return 0成功，-1失败
 */
int32_t OSAL_shutdown(int32_t sockfd, int32_t how);

/*===========================================================================
 * Socket选项操作
 *===========================================================================*/

/**
 * @brief 设置socket选项
 * @param sockfd socket描述符
 * @param level 选项级别（OSAL_SOL_*）
 * @param optname 选项名称（OSAL_SO_*）
 * @param optval 选项值指针
 * @param optlen 选项值长度
 * @return 0成功，-1失败
 */
int32_t OSAL_setsockopt(int32_t sockfd, int32_t level, int32_t optname,
                      const void *optval, uint32_t optlen);

/**
 * @brief 获取socket选项
 * @param sockfd socket描述符
 * @param level 选项级别（OSAL_SOL_*）
 * @param optname 选项名称（OSAL_SO_*）
 * @param optval 选项值指针
 * @param optlen 选项值长度指针
 * @return 0成功，-1失败
 */
int32_t OSAL_getsockopt(int32_t sockfd, int32_t level, int32_t optname,
                      void *optval, uint32_t *optlen);

/*===========================================================================
 * 网络接口操作
 *===========================================================================*/

/**
 * @brief 根据接口名获取接口索引
 * @param ifname 接口名称（如"eth0", "can0"）
 * @return 接口索引(>0)，失败返回0
 */
uint32_t OSAL_if_nametoindex(const char *ifname);

/**
 * @brief 根据接口索引获取接口名
 * @param ifindex 接口索引
 * @param ifname 接口名称缓冲区（至少16字节）
 * @return 接口名称指针，失败返回NULL
 */
char *OSAL_if_indextoname(uint32_t ifindex, char *ifname);

/*===========================================================================
 * 字节序转换
 *===========================================================================*/

/**
 * @brief 主机字节序转网络字节序（16位）
 */
uint16_t OSAL_htons(uint16_t hostshort);

/**
 * @brief 主机字节序转网络字节序（32位）
 */
uint32_t OSAL_htonl(uint32_t hostlong);

/**
 * @brief 网络字节序转主机字节序（16位）
 */
uint16_t OSAL_ntohs(uint16_t netshort);

/**
 * @brief 网络字节序转主机字节序（32位）
 */
uint32_t OSAL_ntohl(uint32_t netlong);

/*===========================================================================
 * IP地址转换
 *===========================================================================*/

/**
 * @brief 将点分十进制IP地址转换为网络字节序
 * @param af 地址族（OSAL_AF_INET或OSAL_AF_INET6）
 * @param src IP地址字符串
 * @param dst 输出缓冲区
 * @return 1成功，0格式错误，-1地址族不支持
 */
int32_t OSAL_inet_pton(int32_t af, const char *src, void *dst);

/**
 * @brief 将网络字节序IP地址转换为点分十进制
 * @param af 地址族（OSAL_AF_INET或OSAL_AF_INET6）
 * @param src IP地址缓冲区
 * @param dst 输出字符串缓冲区
 * @param size 缓冲区大小
 * @return 字符串指针，失败返回NULL
 */
const char *OSAL_inet_ntop(int32_t af, const void *src, char *dst, uint32_t size);

#endif /* OSAL_SOCKET_H */
