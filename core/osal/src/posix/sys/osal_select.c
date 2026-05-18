/************************************************************************
 * OSAL - select系统调用封装实现（POSIX）
 ************************************************************************/

#include "sys/osal_select.h"
#include <sys/select.h>
#include <string.h>

/*===========================================================================
 * fd_set操作实现
 *===========================================================================*/

void OSAL_FD_ZERO(osal_fd_set_t *set)
{
    memset(set, 0, sizeof(osal_fd_set_t));
}

void OSAL_FD_SET(int32_t fd, osal_fd_set_t *set)
{
    if (fd >= 0 && fd < OSAL_FD_SETSIZE) {
        set->fds_bits[fd / OSAL_FD_BITS_PER_WORD] |= (1U << (fd % OSAL_FD_BITS_PER_WORD));
    }
}

void OSAL_FD_CLR(int32_t fd, osal_fd_set_t *set)
{
    if (fd >= 0 && fd < OSAL_FD_SETSIZE) {
        set->fds_bits[fd / OSAL_FD_BITS_PER_WORD] &= ~(1U << (fd % OSAL_FD_BITS_PER_WORD));
    }
}

int32_t OSAL_FD_ISSET(int32_t fd, const osal_fd_set_t *set)
{
    if (fd >= 0 && fd < OSAL_FD_SETSIZE) {
        return (set->fds_bits[fd / OSAL_FD_BITS_PER_WORD] & (1U << (fd % OSAL_FD_BITS_PER_WORD))) ? 1 : 0;
    }
    return 0;
}

/*===========================================================================
 * select系统调用实现
 *===========================================================================*/

int32_t OSAL_select(int32_t nfds, osal_fd_set_t *readfds, osal_fd_set_t *writefds,
                  osal_fd_set_t *exceptfds, osal_timeval_t *timeout)
{
    struct timeval tv;
    struct timeval *ptv = NULL;

    if (timeout) {
        tv.tv_sec = timeout->tv_sec;
        tv.tv_usec = timeout->tv_usec;
        ptv = &tv;
    }

    union {
        osal_fd_set_t *osal_set;
        fd_set *posix_set;
    } read_union, write_union, except_union;

    read_union.osal_set = readfds;
    write_union.osal_set = writefds;
    except_union.osal_set = exceptfds;

    int32_t result = select(nfds, read_union.posix_set, write_union.posix_set,
                           except_union.posix_set, ptv);

    if (timeout && ptv) {
        timeout->tv_sec = tv.tv_sec;
        timeout->tv_usec = tv.tv_usec;
    }

    return result;
}

int32_t OSAL_pselect(int32_t nfds, osal_fd_set_t *readfds, osal_fd_set_t *writefds,
                   osal_fd_set_t *exceptfds, const osal_timespec_t *timeout,
                   const void *sigmask)
{
    struct timespec ts;
    struct timespec *pts = NULL;

    if (timeout) {
        ts.tv_sec = timeout->tv_sec;
        ts.tv_nsec = timeout->tv_nsec;
        pts = &ts;
    }

    union {
        osal_fd_set_t *osal_set;
        fd_set *posix_set;
    } read_union, write_union, except_union;

    union {
        const void *osal_mask;
        const sigset_t *posix_mask;
    } sigmask_union;

    read_union.osal_set = readfds;
    write_union.osal_set = writefds;
    except_union.osal_set = exceptfds;
    sigmask_union.osal_mask = sigmask;

    int32_t result = pselect(nfds, read_union.posix_set, write_union.posix_set,
                            except_union.posix_set, pts, sigmask_union.posix_mask);

    return result;
}
