/************************************************************************
 * OSAL Shared Memory API - POSIX 薄封装
 *
 * 直接暴露 POSIX shm_open/mmap API
 ************************************************************************/

#ifndef OSAL_SHM_H
#define OSAL_SHM_H

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * POSIX 共享内存薄封装
 *===========================================================================*/

/**
 * @brief 打开或创建共享内存对象
 *
 * @param[in] name 共享内存名称（以 / 开头）
 * @param[in] oflag 标志（O_CREAT/O_EXCL/O_RDONLY/O_RDWR）
 * @param[in] mode 权限（如 0666）
 * @return 文件描述符（成功）
 * @return -1（失败）
 */
int32_t OSAL_shm_open(const char *name, int32_t oflag, mode_t mode);

/**
 * @brief 删除共享内存对象
 *
 * @param[in] name 共享内存名称
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_shm_unlink(const char *name);

/**
 * @brief 设置共享内存大小
 *
 * @param[in] fd 文件描述符
 * @param[in] length 大小（字节）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_ftruncate(int32_t fd, off_t length);

/**
 * @brief 映射共享内存到进程地址空间
 *
 * @param[in] addr 建议地址（通常为 NULL）
 * @param[in] length 映射大小
 * @param[in] prot 保护标志（PROT_READ/PROT_WRITE）
 * @param[in] flags 映射标志（MAP_SHARED/MAP_PRIVATE）
 * @param[in] fd 文件描述符
 * @param[in] offset 偏移量
 * @return 映射地址（成功）
 * @return MAP_FAILED（失败）
 */
void* OSAL_mmap(void *addr, size_t length, int32_t prot, int32_t flags, int32_t fd, off_t offset);

/**
 * @brief 解除内存映射
 *
 * @param[in] addr 映射地址
 * @param[in] length 映射大小
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_munmap(void *addr, size_t length);

/**
 * @brief 同步内存映射到磁盘
 *
 * @param[in] addr 映射地址
 * @param[in] length 同步大小
 * @param[in] flags 标志（MS_ASYNC/MS_SYNC/MS_INVALIDATE）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_msync(void *addr, size_t length, int32_t flags);

/**
 * @brief 关闭文件描述符
 *
 * @param[in] fd 文件描述符
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_close(int32_t fd);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SHM_H */
