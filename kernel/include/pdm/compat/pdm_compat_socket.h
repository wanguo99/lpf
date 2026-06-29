// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_COMPAT_SOCKET_H
#define PDM_COMPAT_SOCKET_H

#include "pdm/compat/pdm_compat_features.h"

#include <linux/errno.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/sockptr.h>

static inline int pdm_compat_kernel_bind(struct socket *sock,
					 struct sockaddr *addr, int addrlen)
{
#if PDM_KERNEL_HAS_SOCKADDR_UNSIZED
	return kernel_bind(sock, (struct sockaddr_unsized *)addr, addrlen);
#else
	return kernel_bind(sock, addr, addrlen);
#endif
}

static inline int pdm_compat_kernel_setsockopt(struct socket *sock,
					       int level, int optname,
					       void *optval,
					       unsigned int optlen)
{
	if (!sock || !sock->ops || !sock->ops->setsockopt) {
		return -EOPNOTSUPP;
	}

	return sock->ops->setsockopt(sock, level, optname,
				     KERNEL_SOCKPTR(optval), optlen);
}

#endif /* PDM_COMPAT_SOCKET_H */
