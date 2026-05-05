/************************************************************************
 * OSAL - socket系统调用封装实现（POSIX）
 ************************************************************************/

#include "net/osal_socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/*===========================================================================
 * Socket基本操作
 *===========================================================================*/

int32_t OSAL_socket(int32_t domain, int32_t type, int32_t protocol)
{
    int32_t result = socket(domain, type, protocol);
    return result;
}

int32_t OSAL_bind(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen)
{
    union {
        const osal_sockaddr_t *osal_addr;
        const struct sockaddr *posix_addr;
    } addr_union;

    addr_union.osal_addr = addr;
    socklen_t len = (socklen_t)addrlen;
    int32_t result = bind(sockfd, addr_union.posix_addr, len);
    return result;
}

int32_t OSAL_listen(int32_t sockfd, int32_t backlog)
{
    int32_t result = listen(sockfd, backlog);
    return result;
}

int32_t OSAL_accept(int32_t sockfd, osal_sockaddr_t *addr, uint32_t *addrlen)
{
    socklen_t len = addrlen ? (socklen_t)(*addrlen) : 0;

    union {
        osal_sockaddr_t *osal_addr;
        struct sockaddr *posix_addr;
    } addr_union;

    addr_union.osal_addr = addr;
    int32_t result = accept(sockfd, addr_union.posix_addr, addrlen ? &len : NULL);
    if (addrlen) {
        *addrlen = (uint32_t)len;
    }
    return result;
}

int32_t OSAL_connect(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen)
{
    union {
        const osal_sockaddr_t *osal_addr;
        const struct sockaddr *posix_addr;
    } addr_union;

    addr_union.osal_addr = addr;
    socklen_t len = (socklen_t)addrlen;
    int32_t result = connect(sockfd, addr_union.posix_addr, len);
    return result;
}

int32_t OSAL_send(int32_t sockfd, const void *buf, uint32_t len, int32_t flags)
{
    ssize_t result = send(sockfd, buf, (size_t)len, flags);

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    int32_t safe_result = (int32_t)result;
    return safe_result;
}

int32_t OSAL_recv(int32_t sockfd, void *buf, uint32_t len, int32_t flags)
{
    ssize_t result = recv(sockfd, buf, (size_t)len, flags);

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    int32_t safe_result = (int32_t)result;
    return safe_result;
}

int32_t OSAL_sendto(int32_t sockfd, const void *buf, uint32_t len, int32_t flags,
                  const osal_sockaddr_t *dest_addr, uint32_t addrlen)
{
    union {
        const osal_sockaddr_t *osal_addr;
        const struct sockaddr *posix_addr;
    } addr_union;

    addr_union.osal_addr = dest_addr;
    socklen_t addr_len = (socklen_t)addrlen;
    ssize_t result = sendto(sockfd, buf, (size_t)len, flags, addr_union.posix_addr, addr_len);

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    int32_t safe_result = (int32_t)result;
    return safe_result;
}

int32_t OSAL_recvfrom(int32_t sockfd, void *buf, uint32_t len, int32_t flags,
                    osal_sockaddr_t *src_addr, uint32_t *addrlen)
{
    socklen_t slen = addrlen ? (socklen_t)(*addrlen) : 0;

    union {
        osal_sockaddr_t *osal_addr;
        struct sockaddr *posix_addr;
    } addr_union;

    addr_union.osal_addr = src_addr;
    ssize_t result = recvfrom(sockfd, buf, (size_t)len, flags,
                             addr_union.posix_addr, addrlen ? &slen : NULL);
    if (addrlen) {
        *addrlen = (uint32_t)slen;
    }

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    int32_t safe_result = (int32_t)result;
    return safe_result;
}

int32_t OSAL_shutdown(int32_t sockfd, int32_t how)
{
    int32_t result = shutdown(sockfd, how);
    return result;
}

/*===========================================================================
 * Socket选项操作
 *===========================================================================*/

int32_t OSAL_setsockopt(int32_t sockfd, int32_t level, int32_t optname,
                      const void *optval, uint32_t optlen)
{
    socklen_t len = (socklen_t)optlen;
    int32_t result = setsockopt(sockfd, level, optname, optval, len);
    return result;
}

int32_t OSAL_getsockopt(int32_t sockfd, int32_t level, int32_t optname,
                      void *optval, uint32_t *optlen)
{
    socklen_t len = (socklen_t)(*optlen);
    int32_t result = getsockopt(sockfd, level, optname, optval, &len);
    *optlen = (uint32_t)len;
    return result;
}

/*===========================================================================
 * 网络接口操作
 *===========================================================================*/

uint32_t OSAL_if_nametoindex(const char *ifname)
{
    uint32_t result = if_nametoindex(ifname);
    return result;
}

char *OSAL_if_indextoname(uint32_t ifindex, char *ifname)
{
    uint32_t index = ifindex;
    char *result = if_indextoname(index, ifname);
    return result;
}

/*===========================================================================
 * 字节序转换
 *===========================================================================*/

uint16_t OSAL_htons(uint16_t hostshort)
{
    return htons(hostshort);
}

uint32_t OSAL_htonl(uint32_t hostlong)
{
    return htonl(hostlong);
}

uint16_t OSAL_ntohs(uint16_t netshort)
{
    return ntohs(netshort);
}

uint32_t OSAL_ntohl(uint32_t netlong)
{
    return ntohl(netlong);
}

/*===========================================================================
 * IP地址转换
 *===========================================================================*/

int32_t OSAL_inet_pton(int32_t af, const char *src, void *dst)
{
    int32_t result = inet_pton(af, src, dst);
    return result;
}

const char *OSAL_inet_ntop(int32_t af, const void *src, char *dst, uint32_t size)
{
    socklen_t len = (socklen_t)size;
    return inet_ntop(af, src, dst, len);
}
