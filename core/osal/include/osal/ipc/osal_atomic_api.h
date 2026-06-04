/************************************************************************
 * OSAL Atomic API
 *
 * Lock-free atomic operations for lock-free programming
 ************************************************************************/

#ifndef OSAL_ATOMIC_API_H
#define OSAL_ATOMIC_API_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 原子类型定义
 *===========================================================================*/

/**
 * @brief 原子无符号32位整数类型
 *
 * 注意：使用 _Atomic 类型限定符确保原子操作的正确性
 * 避免类型转换导致的未定义行为
 */
typedef struct {
	_Atomic uint32_t value;
} osal_atomic_uint32_t;

/**
 * @brief 原子无符号64位整数类型
 *
 * 用于原子时间戳等需要64位精度的场景
 */
typedef struct {
	_Atomic uint64_t value;
} osal_atomic_uint64_t;

/*===========================================================================
 * 32位原子操作 API
 *===========================================================================*/

/**
 * @brief 初始化原子变量
 * @param atomic 原子变量指针
 * @param value 初始值
 */
void OSAL_AtomicInit(osal_atomic_uint32_t *atomic, uint32_t value);

/**
 * @brief 原子加载（读取）
 * @param atomic 原子变量指针
 * @return 当前值
 */
uint32_t OSAL_AtomicLoad(const osal_atomic_uint32_t *atomic);

/**
 * @brief 原子存储（写入）
 * @param atomic 原子变量指针
 * @param value 要写入的值
 */
void OSAL_AtomicStore(osal_atomic_uint32_t *atomic, uint32_t value);

/**
 * @brief 原子加法（fetch_add）
 * @param atomic 原子变量指针
 * @param value 要增加的值
 * @return 增加前的值
 */
uint32_t OSAL_AtomicFetchAdd(osal_atomic_uint32_t *atomic, uint32_t value);

/**
 * @brief 原子减法（fetch_sub）
 * @param atomic 原子变量指针
 * @param value 要减少的值
 * @return 减少前的值
 */
uint32_t OSAL_AtomicFetchSub(osal_atomic_uint32_t *atomic, uint32_t value);

/**
 * @brief 原子自增（++）
 * @param atomic 原子变量指针
 * @return 自增后的值
 */
uint32_t OSAL_AtomicIncrement(osal_atomic_uint32_t *atomic);

/**
 * @brief 原子自减（--）
 * @param atomic 原子变量指针
 * @return 自减后的值
 */
uint32_t OSAL_AtomicDecrement(osal_atomic_uint32_t *atomic);

/**
 * @brief 原子比较并交换（CAS）
 * @param atomic 原子变量指针
 * @param expected 期望值
 * @param desired 目标值
 * @return true 交换成功，false 交换失败
 */
bool OSAL_AtomicCompareExchange(osal_atomic_uint32_t *atomic, uint32_t expected, uint32_t desired);

/*===========================================================================
 * 64位原子操作 API
 *===========================================================================*/

/**
 * @brief 初始化64位原子变量
 * @param atomic 原子变量指针
 * @param value 初始值
 */
void OSAL_AtomicInit64(osal_atomic_uint64_t *atomic, uint64_t value);

/**
 * @brief 64位原子加载（读取）
 * @param atomic 原子变量指针
 * @return 当前值
 */
uint64_t OSAL_AtomicLoad64(const osal_atomic_uint64_t *atomic);

/**
 * @brief 64位原子存储（写入）
 * @param atomic 原子变量指针
 * @param value 要写入的值
 */
void OSAL_AtomicStore64(osal_atomic_uint64_t *atomic, uint64_t value);

/**
 * @brief 64位原子加法（fetch_add）
 * @param atomic 原子变量指针
 * @param value 要增加的值
 * @return 增加前的值
 */
uint64_t OSAL_AtomicFetchAdd64(osal_atomic_uint64_t *atomic, uint64_t value);

/**
 * @brief 64位原子减法（fetch_sub）
 * @param atomic 原子变量指针
 * @param value 要减少的值
 * @return 减少前的值
 */
uint64_t OSAL_AtomicFetchSub64(osal_atomic_uint64_t *atomic, uint64_t value);

/**
 * @brief 64位原子自增（++）
 * @param atomic 原子变量指针
 * @return 自增后的值
 */
uint64_t OSAL_AtomicIncrement64(osal_atomic_uint64_t *atomic);

/**
 * @brief 64位原子自减（--）
 * @param atomic 原子变量指针
 * @return 自减后的值
 */
uint64_t OSAL_AtomicDecrement64(osal_atomic_uint64_t *atomic);

/**
 * @brief 64位原子比较并交换（CAS）
 * @param atomic 原子变量指针
 * @param expected 期望值
 * @param desired 目标值
 * @return true 交换成功，false 交换失败
 */
bool OSAL_AtomicCompareExchange64(osal_atomic_uint64_t *atomic, uint64_t expected, uint64_t desired);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_ATOMIC_API_H */
