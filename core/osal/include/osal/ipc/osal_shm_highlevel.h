/************************************************************************
 * OSAL 高级共享内存 API
 *
 * 提供更高级的共享内存封装，简化常见使用场景
 ************************************************************************/

#ifndef OSAL_SHM_HIGHLEVEL_H
#define OSAL_SHM_HIGHLEVEL_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 标志定义
 *===========================================================================*/

#define OSAL_SHM_CREATE 0x0001 /* 创建共享内存 */
#define OSAL_SHM_RDWR 0x0002 /* 读写权限 */
#define OSAL_SHM_RDONLY 0x0004 /* 只读权限 */

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief 共享内存句柄
 */
typedef int32_t osal_shm_t;

/*===========================================================================
 * API 函数
 *===========================================================================*/

/**
 * @brief 创建或打开共享内存
 *
 * @param[in] name 共享内存名称
 * @param[in] size 大小（字节）
 * @param[in] flags 标志（OSAL_SHM_CREATE | OSAL_SHM_RDWR）
 * @param[out] shm 共享内存句柄
 * @return OSAL_SUCCESS 成功
 * @return 其他值 失败
 */
int32_t osal_shm_create(const char *name, osal_size_t size, int32_t flags,
						osal_shm_t *shm);

/**
 * @brief 映射共享内存到进程地址空间
 *
 * @param[in] shm 共享内存句柄
 * @param[in] offset 偏移量
 * @param[in] length 映射大小（0 表示全部）
 * @param[in] flags 标志（OSAL_SHM_RDWR | OSAL_SHM_RDONLY）
 * @param[out] addr 映射地址
 * @return OSAL_SUCCESS 成功
 * @return 其他值 失败
 */
int32_t osal_shm_map(osal_shm_t shm, osal_off_t offset, osal_size_t length,
					 int32_t flags, void **addr);

/**
 * @brief 解除共享内存映射
 *
 * @param[in] addr 映射地址
 * @param[in] length 映射大小
 * @return OSAL_SUCCESS 成功
 * @return 其他值 失败
 */
int32_t osal_shm_unmap(void *addr, osal_size_t length);

/**
 * @brief 关闭共享内存句柄
 *
 * @param[in] shm 共享内存句柄
 * @return OSAL_SUCCESS 成功
 * @return 其他值 失败
 */
int32_t osal_shm_close(osal_shm_t shm);

/**
 * @brief 删除共享内存对象
 *
 * @param[in] name 共享内存名称
 * @return OSAL_SUCCESS 成功
 * @return 其他值 失败
 */
int32_t osal_shm_remove(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SHM_HIGHLEVEL_H */
