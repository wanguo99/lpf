/************************************************************************
 * OSAL Shared Memory API
 *
 * Shared memory operations for inter-process communication
 ************************************************************************/

#ifndef OSAL_SHM_H
#define OSAL_SHM_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 共享内存类型定义
 *===========================================================================*/

/**
 * @brief 共享内存句柄（不透明类型）
 */
typedef void* osal_shm_t;

/*===========================================================================
 * 共享内存标志定义
 *===========================================================================*/

/* 共享内存访问权限 */
#define OSAL_SHM_RDONLY   0x01  /* 只读 */
#define OSAL_SHM_RDWR     0x02  /* 读写 */

/* 共享内存创建标志 */
#define OSAL_SHM_CREATE   0x0100  /* 创建新的共享内存 */
#define OSAL_SHM_EXCL     0x0200  /* 与CREATE配合，如果已存在则失败 */

/*===========================================================================
 * 共享内存管理 API
 *===========================================================================*/

/**
 * @brief 创建或打开共享内存对象
 *
 * @param[in] name 共享内存名称（以'/'开头，如"/ems_shm"）
 * @param[in] size 共享内存大小（字节）
 * @param[in] flags 创建标志（OSAL_SHM_CREATE | OSAL_SHM_RDWR等）
 * @param[out] shm 返回共享内存句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER name或shm为NULL
 * @return OSAL_ERR_GENERIC 创建失败
 */
int32_t OSAL_ShmCreate(const char *name, size_t size, int32_t flags, osal_shm_t *shm);

/**
 * @brief 映射共享内存到进程地址空间
 *
 * @param[in] shm 共享内存句柄
 * @param[in] offset 映射偏移量（通常为0）
 * @param[in] size 映射大小（0表示整个共享内存）
 * @param[in] flags 访问权限（OSAL_SHM_RDONLY或OSAL_SHM_RDWR）
 * @param[out] addr 返回映射的地址
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER addr为NULL
 * @return OSAL_ERR_GENERIC 映射失败
 */
int32_t OSAL_ShmMap(osal_shm_t shm, size_t offset, size_t size, int32_t flags, void **addr);

/**
 * @brief 解除共享内存映射
 *
 * @param[in] addr 映射的地址
 * @param[in] size 映射的大小
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER addr为NULL
 * @return OSAL_ERR_GENERIC 解映射失败
 */
int32_t OSAL_ShmUnmap(void *addr, size_t size);

/**
 * @brief 关闭共享内存句柄
 *
 * @param[in] shm 共享内存句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 关闭失败
 */
int32_t OSAL_ShmClose(osal_shm_t shm);

/**
 * @brief 删除共享内存对象
 *
 * @param[in] name 共享内存名称
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER name为NULL
 * @return OSAL_ERR_GENERIC 删除失败
 */
int32_t OSAL_ShmUnlink(const char *name);

/**
 * @brief 同步共享内存到磁盘
 *
 * @param[in] addr 映射的地址
 * @param[in] size 同步的大小
 * @param[in] async 是否异步同步（true=异步，false=同步）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER addr为NULL
 * @return OSAL_ERR_GENERIC 同步失败
 */
int32_t OSAL_ShmSync(void *addr, size_t size, bool async);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SHM_H */
