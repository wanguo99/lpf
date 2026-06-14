/************************************************************************
 * OSAL - 共享内存薄封装（POSIX实现）
 ************************************************************************/

#include "osal.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int32_t OSAL_shm_open(const char *name, int32_t oflag, mode_t mode)
{
    if (name == NULL) {
        errno = EINVAL;
        return -1;
    }

    return shm_open(name, oflag, mode);
}

int32_t OSAL_shm_unlink(const char *name)
{
    if (name == NULL) {
        errno = EINVAL;
        return -1;
    }

    return shm_unlink(name);
}

int32_t OSAL_ftruncate(int32_t fd, off_t length)
{
    return ftruncate(fd, length);
}

void* OSAL_mmap(void *addr, size_t length, int32_t prot, int32_t flags, int32_t fd, off_t offset)
{
    return mmap(addr, length, prot, flags, fd, offset);
}

int32_t OSAL_munmap(void *addr, size_t length)
{
    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return munmap(addr, length);
}

int32_t OSAL_msync(void *addr, size_t length, int32_t flags)
{
    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }

    return msync(addr, length, flags);
}

int32_t OSAL_close(int32_t fd)
{
    return close(fd);
}
