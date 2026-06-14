/************************************************************************
 * OSAL Shared Memory 兼容层
 *
 * 提供旧 API 的兼容包装
 ************************************************************************/

#ifndef OSAL_SHM_COMPAT_H
#define OSAL_SHM_COMPAT_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 兼容类型和标志
 *===========================================================================*/

/* 旧的 osal_shm_t -> 新的文件描述符 */
typedef int32_t osal_shm_t;

/* 共享内存访问权限（兼容） */
#define OSAL_SHM_RDONLY   O_RDONLY
#define OSAL_SHM_RDWR     O_RDWR

/* 共享内存创建标志（兼容） */
#define OSAL_SHM_CREATE   O_CREAT
#define OSAL_SHM_EXCL     O_EXCL

/*===========================================================================
 * 兼容 API
 *===========================================================================*/

/**
 * @brief 创建或打开共享内存（兼容旧 API）
 */
static inline int32_t OSAL_ShmCreate(const char *name, size_t size,
                                     int32_t flags, osal_shm_t *shm)
{
    int fd;
    int oflag = flags;

    if (name == NULL || shm == NULL || size == 0) {
        return -1;
    }

    /* 打开或创建共享内存 */
    fd = OSAL_shm_open(name, oflag, 0666);
    if (fd < 0) {
        return -1;
    }

    /* 设置大小（如果是创建） */
    if (flags & O_CREAT) {
        if (OSAL_ftruncate(fd, size) < 0) {
            OSAL_close(fd);
            return -1;
        }
    }

    *shm = fd;
    return 0;
}

/**
 * @brief 映射共享内存（兼容旧 API）
 */
static inline int32_t OSAL_ShmMap(osal_shm_t shm, size_t offset, size_t size,
                                  int32_t flags, void **addr)
{
    int prot = 0;
    void *ptr;
    struct stat st;

    if (addr == NULL) {
        return -1;
    }

    /* 如果 size 为 0，获取共享内存大小 */
    if (size == 0) {
        if (fstat(shm, &st) < 0) {
            return -1;
        }
        size = st.st_size;
    }

    /* 转换保护标志 */
    if (flags & O_RDWR) {
        prot = PROT_READ | PROT_WRITE;
    } else if (flags & O_RDONLY) {
        prot = PROT_READ;
    }

    ptr = OSAL_mmap(NULL, size, prot, MAP_SHARED, shm, offset);
    if (ptr == MAP_FAILED) {
        return -1;
    }

    *addr = ptr;
    return 0;
}

/**
 * @brief 解除共享内存映射（兼容旧 API）
 */
static inline int32_t OSAL_ShmUnmap(void *addr, size_t size)
{
    return OSAL_munmap(addr, size);
}

/**
 * @brief 关闭共享内存句柄（兼容旧 API）
 */
static inline int32_t OSAL_ShmClose(osal_shm_t shm)
{
    return OSAL_close(shm);
}

/**
 * @brief 删除共享内存对象（兼容旧 API）
 */
static inline int32_t OSAL_ShmUnlink(const char *name)
{
    return OSAL_shm_unlink(name);
}

/**
 * @brief 同步共享内存到磁盘（兼容旧 API）
 */
static inline int32_t OSAL_ShmSync(void *addr, size_t size, int32_t flags)
{
    return OSAL_msync(addr, size, flags);
}

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SHM_COMPAT_H */
