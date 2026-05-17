/************************************************************************
 * OSAL - 共享内存封装（POSIX实现，重点支持Linux）
 ************************************************************************/

#define _DEFAULT_SOURCE  /* 启用MADV_WILLNEED等非POSIX扩展 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "ipc/osal_shm.h"
#include "lib/osal_errno.h"

/* 共享内存对象结构 */
struct osal_shm_s {
    int fd;           /* 文件描述符 */
    size_t size;      /* 共享内存大小 */
    char name[256];   /* 共享内存名称 */
};

/**
 * 创建或打开共享内存对象
 */
int32_t OSAL_ShmCreate(const char *name, size_t size, int32_t flags, osal_shm_t *shm)
{
    struct osal_shm_s *shm_obj;
    int fd;
    int oflags = 0;
    mode_t mode = 0666;  /* 默认权限：rw-rw-rw- */
    int ret;

    /* 参数校验 */
    if (name == NULL || shm == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (size == 0) {
        return OSAL_EINVAL;
    }

    /* 名称必须以'/'开头 */
    if (name[0] != '/') {
        return OSAL_EINVAL;
    }

    /* 转换标志位 */
    if (flags & OSAL_SHM_CREATE) {
        oflags |= O_CREAT;
    }
    if (flags & OSAL_SHM_EXCL) {
        oflags |= O_EXCL;
    }
    if (flags & OSAL_SHM_RDWR) {
        oflags |= O_RDWR;
    } else if (flags & OSAL_SHM_RDONLY) {
        oflags |= O_RDONLY;
    } else {
        oflags |= O_RDWR;  /* 默认读写 */
    }

    /* 创建或打开共享内存对象 */
    fd = shm_open(name, oflags, mode);
    if (fd < 0) {
        if (errno == EEXIST) {
            return OSAL_EEXIST;
        } else if (errno == ENOENT) {
            return OSAL_ENOENT;
        } else if (errno == EACCES || errno == EPERM) {
            return OSAL_ERR_PERMISSION;
        }
        return OSAL_EIO;
    }

    /* 如果是创建模式，设置共享内存大小 */
    if (flags & OSAL_SHM_CREATE) {
        ret = ftruncate(fd, size);
        if (ret != 0) {
            close(fd);
            if (errno == ENOSPC) {
                return OSAL_ERR_NO_MEMORY;
            }
            return OSAL_EIO;
        }
    } else {
        /* 如果是打开模式，获取实际大小 */
        struct stat st;
        ret = fstat(fd, &st);
        if (ret != 0) {
            close(fd);
            return OSAL_EIO;
        }
        size = st.st_size;
    }

    /* 分配共享内存对象 */
    shm_obj = (struct osal_shm_s *)malloc(sizeof(struct osal_shm_s));
    if (shm_obj == NULL) {
        close(fd);
        return OSAL_ERR_NO_MEMORY;
    }

    shm_obj->fd = fd;
    shm_obj->size = size;
    strncpy(shm_obj->name, name, sizeof(shm_obj->name) - 1);
    shm_obj->name[sizeof(shm_obj->name) - 1] = '\0';

    *shm = (osal_shm_t)shm_obj;
    return OSAL_SUCCESS;
}

/**
 * 映射共享内存到进程地址空间
 */
int32_t OSAL_ShmMap(osal_shm_t shm, size_t offset, size_t size, int32_t flags, void **addr)
{
    struct osal_shm_s *shm_obj = (struct osal_shm_s *)shm;
    void *mapped_addr;
    int prot = 0;
    int mflags = MAP_SHARED;  /* Linux默认使用MAP_SHARED */

    /* 参数校验 */
    if (shm_obj == NULL || addr == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 如果size为0，映射整个共享内存 */
    if (size == 0) {
        size = shm_obj->size;
    }

    /* 检查映射范围 */
    if (offset + size > shm_obj->size) {
        return OSAL_EINVAL;
    }

    /* 转换保护标志 */
    if (flags & OSAL_SHM_RDWR) {
        prot = PROT_READ | PROT_WRITE;
    } else if (flags & OSAL_SHM_RDONLY) {
        prot = PROT_READ;
    } else {
        prot = PROT_READ | PROT_WRITE;  /* 默认读写 */
    }

    /* 执行映射 */
    mapped_addr = mmap(NULL, size, prot, mflags, shm_obj->fd, offset);
    if (mapped_addr == MAP_FAILED) {
        if (errno == ENOMEM) {
            return OSAL_ERR_NO_MEMORY;
        } else if (errno == EACCES) {
            return OSAL_ERR_PERMISSION;
        }
        return OSAL_EIO;
    }

#ifdef __linux__
    /* Linux特有优化：建议内核预读页面 */
    (void)madvise(mapped_addr, size, MADV_WILLNEED);

    /* 对于实时应用，锁定页面防止交换 */
    /* 注意：这需要足够的内存限制权限 */
    /* mlock(mapped_addr, size); */
#endif

    *addr = mapped_addr;
    return OSAL_SUCCESS;
}

/**
 * 解除共享内存映射
 */
int32_t OSAL_ShmUnmap(void *addr, size_t size)
{
    int ret;

    /* 参数校验 */
    if (addr == NULL || size == 0) {
        return OSAL_EINVAL;
    }

#ifdef __linux__
    /* 如果之前锁定了页面，先解锁 */
    /* munlock(addr, size); */
#endif

    /* 解除映射 */
    ret = munmap(addr, size);
    if (ret != 0) {
        return OSAL_EIO;
    }

    return OSAL_SUCCESS;
}

/**
 * 关闭共享内存句柄
 */
int32_t OSAL_ShmClose(osal_shm_t shm)
{
    struct osal_shm_s *shm_obj = (struct osal_shm_s *)shm;

    /* 参数校验 */
    if (shm_obj == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 关闭文件描述符 */
    if (shm_obj->fd >= 0) {
        close(shm_obj->fd);
    }

    /* 释放对象 */
    free(shm_obj);

    return OSAL_SUCCESS;
}

/**
 * 删除共享内存对象
 */
int32_t OSAL_ShmUnlink(const char *name)
{
    int ret;

    /* 参数校验 */
    if (name == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 名称必须以'/'开头 */
    if (name[0] != '/') {
        return OSAL_EINVAL;
    }

    /* 删除共享内存对象 */
    ret = shm_unlink(name);
    if (ret != 0) {
        if (errno == ENOENT) {
            return OSAL_ENOENT;
        } else if (errno == EACCES || errno == EPERM) {
            return OSAL_ERR_PERMISSION;
        }
        return OSAL_EIO;
    }

    return OSAL_SUCCESS;
}

/**
 * 同步共享内存到磁盘
 */
int32_t OSAL_ShmSync(void *addr, size_t size, bool async)
{
    int ret;
    int flags;

    /* 参数校验 */
    if (addr == NULL || size == 0) {
        return OSAL_EINVAL;
    }

    /* 设置同步标志 */
    flags = async ? MS_ASYNC : MS_SYNC;

#ifdef __linux__
    /* Linux特有：使页面失效，强制从共享内存重新读取 */
    /* flags |= MS_INVALIDATE; */
#endif

    /* 执行同步 */
    ret = msync(addr, size, flags);
    if (ret != 0) {
        if (errno == ENOMEM) {
            return OSAL_ERR_NO_MEMORY;
        }
        return OSAL_EIO;
    }

    return OSAL_SUCCESS;
}
