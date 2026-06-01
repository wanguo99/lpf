/************************************************************************
 * OSAL - 套接字操作封装实现（POSIX）
 ************************************************************************/

#include <sys/socket.h>
#include "sys/osal_socket.h"

/*===========================================================================
 * 套接字操作
 *===========================================================================*/

int32_t OSAL_socket(int32_t domain, int32_t type, int32_t protocol)
{
    int32_t result = socket(domain, type, protocol);
    return result;
}

int32_t OSAL_bind(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen)
{
    const struct sockaddr *posix_addr = (const struct sockaddr *)addr->opaque;
    int32_t result = bind(sockfd, posix_addr, (socklen_t)addrlen);
    return result;
}

int32_t OSAL_setsockopt(int32_t sockfd, int32_t level, int32_t optname,
                        const void *optval, uint32_t optlen)
{
    int32_t result = setsockopt(sockfd, level, optname, optval, (socklen_t)optlen);
    return result;
}
