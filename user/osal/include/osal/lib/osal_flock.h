/**
 * @file osal_flock.h
 * @brief OSAL 文件锁接口
 *
 * 功能：
 * - 提供跨进程的文件锁机制（基于 fcntl）
 * - 支持独占锁和共享锁
 * - 支持阻塞和非阻塞模式
 * - 进程崩溃自动释放
 *
 * 使用场景：
 * - 多进程访问同一硬件资源
 * - 配合 osal_mutex_lock 实现进程间 + 线程间双重保护
 *
 * 注意：
 * - fcntl 文件锁是进程级别的，不能解决同一进程内的线程并发
 * - 必须配合 osal_mutex_lock 使用才能完整保护
 */

#ifndef OSAL_FLOCK_H
#define OSAL_FLOCK_H

/*===========================================================================
 * 锁文件配置
 *===========================================================================*/

/**
 * @brief 锁文件根目录
 *
 * 所有文件锁都存放在此目录下
 * Linux 标准：/var/lock 通常是 /run/lock 的符号链接
 */
#define OSAL_LOCK_DIR "/var/lock"

/**
 * @brief 默认文件锁超时时间（毫秒）
 */
#define OSAL_LOCK_DEFAULT_TIMEOUT_MS 0x1388

/**
 * @brief 锁文件路径最大长度
 */
#define OSAL_LOCK_PATH_MAX_LEN 0x100

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief 文件锁句柄（不透明类型）
 */
typedef struct osal_flock_s osal_flock_t;

/**
 * @brief 锁类型
 */
typedef enum {
	OSAL_FLOCK_SHARED = 0, /* 共享锁（读锁） */
	OSAL_FLOCK_EXCLUSIVE = 1 /* 独占锁（写锁） */
} osal_flock_type_t;

/*===========================================================================
 * 基础 API
 *===========================================================================*/

/**
 * @brief 创建文件锁
 *
 * @param[in] lock_file 锁文件路径（如 "/var/lock/lpf_can0.lock"）
 * @param[out] flock 文件锁句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 * @return OSAL_ERR_NO_MEMORY 内存不足
 * @return OSAL_ERR_GENERIC 创建失败
 */
int32_t osal_flock_create(const char *lock_file, osal_flock_t **flock);

/**
 * @brief 销毁文件锁
 *
 * @param[in] flock 文件锁句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 */
int32_t osal_flock_destroy(osal_flock_t *flock);

/**
 * @brief 加锁（阻塞模式）
 *
 * @param[in] flock 文件锁句柄
 * @param[in] type 锁类型
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 * @return OSAL_ERR_GENERIC 加锁失败
 */
int32_t osal_flock_lock(osal_flock_t *flock, osal_flock_type_t type);

/**
 * @brief 尝试加锁（非阻塞模式）
 *
 * @param[in] flock 文件锁句柄
 * @param[in] type 锁类型
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_BUSY 锁被占用
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 * @return OSAL_ERR_GENERIC 加锁失败
 */
int32_t osal_flock_try_lock(osal_flock_t *flock, osal_flock_type_t type);

/**
 * @brief 带超时的加锁
 *
 * @param[in] flock 文件锁句柄
 * @param[in] type 锁类型
 * @param[in] timeout_ms 超时时间（毫秒）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 * @return OSAL_ERR_GENERIC 加锁失败
 */
int32_t osal_flock_timed_lock(osal_flock_t *flock, osal_flock_type_t type,
							  uint32_t timeout_ms);

/**
 * @brief 解锁
 *
 * @param[in] flock 文件锁句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为空
 * @return OSAL_ERR_GENERIC 解锁失败
 */
int32_t osal_flock_unlock(osal_flock_t *flock);

#endif /* OSAL_FLOCK_H */
