/************************************************************************
 * OSAL POSIX实现 - 原子操作
 *
 * 修复：移除不安全的类型转换，直接使用 _Atomic 类型
 ************************************************************************/

#include "osal/ipc/osal_atomic.h"
#include <stdatomic.h>

/*===========================================================================
 * 32位原子操作实现
 *===========================================================================*/

void OSAL_AtomicInit(osal_atomic_uint32_t *atomic, uint32_t value)
{
    atomic_init(&atomic->value, value);
}

uint32_t OSAL_AtomicLoad(const osal_atomic_uint32_t *atomic)
{
    return atomic_load(&atomic->value);
}

void OSAL_AtomicStore(osal_atomic_uint32_t *atomic, uint32_t value)
{
    atomic_store(&atomic->value, value);
}

uint32_t OSAL_AtomicFetchAdd(osal_atomic_uint32_t *atomic, uint32_t value)
{
    return atomic_fetch_add(&atomic->value, value);
}

uint32_t OSAL_AtomicFetchSub(osal_atomic_uint32_t *atomic, uint32_t value)
{
    return atomic_fetch_sub(&atomic->value, value);
}

uint32_t OSAL_AtomicIncrement(osal_atomic_uint32_t *atomic)
{
    return atomic_fetch_add(&atomic->value, 1) + 1;
}

uint32_t OSAL_AtomicDecrement(osal_atomic_uint32_t *atomic)
{
    return atomic_fetch_sub(&atomic->value, 1) - 1;
}

bool OSAL_AtomicCompareExchange(osal_atomic_uint32_t *atomic, uint32_t expected, uint32_t desired)
{
    return atomic_compare_exchange_strong(&atomic->value, &expected, desired);
}

/*===========================================================================
 * 64位原子操作实现
 *===========================================================================*/

void OSAL_AtomicInit64(osal_atomic_uint64_t *atomic, uint64_t value)
{
    atomic_init(&atomic->value, value);
}

uint64_t OSAL_AtomicLoad64(const osal_atomic_uint64_t *atomic)
{
    return atomic_load(&atomic->value);
}

void OSAL_AtomicStore64(osal_atomic_uint64_t *atomic, uint64_t value)
{
    atomic_store(&atomic->value, value);
}

uint64_t OSAL_AtomicFetchAdd64(osal_atomic_uint64_t *atomic, uint64_t value)
{
    return atomic_fetch_add(&atomic->value, value);
}

uint64_t OSAL_AtomicFetchSub64(osal_atomic_uint64_t *atomic, uint64_t value)
{
    return atomic_fetch_sub(&atomic->value, value);
}

uint64_t OSAL_AtomicIncrement64(osal_atomic_uint64_t *atomic)
{
    return atomic_fetch_add(&atomic->value, 1) + 1;
}

uint64_t OSAL_AtomicDecrement64(osal_atomic_uint64_t *atomic)
{
    return atomic_fetch_sub(&atomic->value, 1) - 1;
}

bool OSAL_AtomicCompareExchange64(osal_atomic_uint64_t *atomic, uint64_t expected, uint64_t desired)
{
    return atomic_compare_exchange_strong(&atomic->value, &expected, desired);
}

