/************************************************************************
 * OSAL Atomic API - C11 stdatomic 静态内联封装
 *
 * 零开销的原子操作封装（static inline）
 ************************************************************************/

#ifndef OSAL_ATOMIC_H
#define OSAL_ATOMIC_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 原子类型定义
 *===========================================================================*/

/**
 * @brief 原子无符号32位整数类型
 */
typedef struct {
	_Atomic uint32_t value;
} osal_atomic_uint32_t;

/**
 * @brief 原子无符号64位整数类型
 */
typedef struct {
	_Atomic uint64_t value;
} osal_atomic_uint64_t;

/**
 * @brief 原子布尔类型
 */
typedef struct {
	_Atomic uint32_t value;  /* 0=false, 1=true */
} osal_atomic_bool_t;

/*===========================================================================
 * 32位原子操作 API（静态内联）
 *===========================================================================*/

static inline void OSAL_AtomicInit(osal_atomic_uint32_t *atomic, uint32_t value)
{
	atomic_init(&atomic->value, value);
}

static inline uint32_t OSAL_AtomicLoad(const osal_atomic_uint32_t *atomic)
{
	return atomic_load(&atomic->value);
}

static inline void OSAL_AtomicStore(osal_atomic_uint32_t *atomic, uint32_t value)
{
	atomic_store(&atomic->value, value);
}

static inline uint32_t OSAL_AtomicFetchAdd(osal_atomic_uint32_t *atomic, uint32_t value)
{
	return atomic_fetch_add(&atomic->value, value);
}

static inline uint32_t OSAL_AtomicFetchSub(osal_atomic_uint32_t *atomic, uint32_t value)
{
	return atomic_fetch_sub(&atomic->value, value);
}

static inline uint32_t OSAL_AtomicIncrement(osal_atomic_uint32_t *atomic)
{
	return atomic_fetch_add(&atomic->value, 1) + 1;
}

static inline uint32_t OSAL_AtomicDecrement(osal_atomic_uint32_t *atomic)
{
	return atomic_fetch_sub(&atomic->value, 1) - 1;
}

static inline bool OSAL_AtomicCompareExchange(osal_atomic_uint32_t *atomic,
                                               uint32_t *expected,
                                               uint32_t desired)
{
	return atomic_compare_exchange_strong(&atomic->value, expected, desired);
}

/*===========================================================================
 * 64位原子操作 API（静态内联）
 *===========================================================================*/

static inline void OSAL_AtomicInit64(osal_atomic_uint64_t *atomic, uint64_t value)
{
	atomic_init(&atomic->value, value);
}

static inline uint64_t OSAL_AtomicLoad64(const osal_atomic_uint64_t *atomic)
{
	return atomic_load(&atomic->value);
}

static inline void OSAL_AtomicStore64(osal_atomic_uint64_t *atomic, uint64_t value)
{
	atomic_store(&atomic->value, value);
}

static inline uint64_t OSAL_AtomicFetchAdd64(osal_atomic_uint64_t *atomic, uint64_t value)
{
	return atomic_fetch_add(&atomic->value, value);
}

static inline uint64_t OSAL_AtomicFetchSub64(osal_atomic_uint64_t *atomic, uint64_t value)
{
	return atomic_fetch_sub(&atomic->value, value);
}

static inline uint64_t OSAL_AtomicIncrement64(osal_atomic_uint64_t *atomic)
{
	return atomic_fetch_add(&atomic->value, 1) + 1;
}

static inline uint64_t OSAL_AtomicDecrement64(osal_atomic_uint64_t *atomic)
{
	return atomic_fetch_sub(&atomic->value, 1) - 1;
}

static inline bool OSAL_AtomicCompareExchange64(osal_atomic_uint64_t *atomic,
                                                 uint64_t *expected,
                                                 uint64_t desired)
{
	return atomic_compare_exchange_strong(&atomic->value, expected, desired);
}

/*===========================================================================
 * 布尔原子操作（静态内联）
 *===========================================================================*/

static inline void OSAL_AtomicInitBool(osal_atomic_bool_t *atomic, bool value)
{
	atomic_init(&atomic->value, value ? 1 : 0);
}

static inline bool OSAL_AtomicLoadBool(const osal_atomic_bool_t *atomic)
{
	return atomic_load(&atomic->value) != 0;
}

static inline void OSAL_AtomicStoreBool(osal_atomic_bool_t *atomic, bool value)
{
	atomic_store(&atomic->value, value ? 1 : 0);
}

static inline bool OSAL_AtomicCompareExchangeBool(osal_atomic_bool_t *atomic,
                                                   bool *expected,
                                                   bool desired)
{
	uint32_t exp_val = *expected ? 1 : 0;
	uint32_t des_val = desired ? 1 : 0;
	bool result = atomic_compare_exchange_strong(&atomic->value, &exp_val, des_val);
	*expected = (exp_val != 0);
	return result;
}

#ifdef __cplusplus
}
#endif

#endif /* OSAL_ATOMIC_H */
