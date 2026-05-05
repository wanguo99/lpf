/**
 * @file test_osal_heap.c
 * @brief OSAL内存管理单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"

/*===========================================================================
 * 内存分配测试
 *===========================================================================*/

/* 测试用例: Malloc - 成功 */
TEST_CASE(test_osal_malloc_success)
{
    void *ptr;

    /* 分配小块内存 */
    ptr = OSAL_Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_Free(ptr);

    /* 分配中等内存 */
    ptr = OSAL_Malloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_Free(ptr);

    /* 分配大块内存 */
    ptr = OSAL_Malloc(1024 * 1024);  /* 1MB */
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_Free(ptr);
}

/* 测试用例: Malloc - 零大小 */
TEST_CASE(test_osal_malloc_zero_size)
{
    void *ptr;

    /* 分配0字节（行为依赖于实现） */
    ptr = OSAL_Malloc(0);
    /* 某些实现返回NULL，某些返回有效指针 */
    if (NULL != ptr) {
        OSAL_Free(ptr);
    }
}

/* 测试用例: Free - 成功 */
TEST_CASE(test_osal_free_success)
{
    void *ptr;

    ptr = OSAL_Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 释放内存（不应崩溃） */
    OSAL_Free(ptr);
}

/* 测试用例: Free - NULL指针 */
TEST_CASE(test_osal_free_null_pointer)
{
    /* Free(NULL) 应该安全（不崩溃） */
    OSAL_Free(NULL);
}

/*===========================================================================
 * 堆信息测试
 *===========================================================================*/

/* 测试用例: HeapGetInfo - 成功 */
TEST_CASE(test_osal_heap_get_info_success)
{
    uint32_t free_bytes, total_bytes;
    int32_t ret;

    ret = OSAL_HeapGetInfo(&free_bytes, &total_bytes);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(total_bytes > 0);
    TEST_ASSERT_TRUE(free_bytes <= total_bytes);
}

/* 测试用例: HeapGetInfo - 空指针 */
TEST_CASE(test_osal_heap_get_info_null_pointer)
{
    uint32_t free_bytes, total_bytes;
    int32_t ret;

    /* 第一个参数为NULL */
    ret = OSAL_HeapGetInfo(NULL, &total_bytes);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个参数为NULL */
    ret = OSAL_HeapGetInfo(&free_bytes, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 两个参数都为NULL */
    ret = OSAL_HeapGetInfo(NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapGetStats - 成功 */
TEST_CASE(test_osal_heap_get_stats_success)
{
    uint32_t current, peak;
    int32_t ret;

    ret = OSAL_HeapGetStats(&current, &peak);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(peak >= current);
}

/* 测试用例: HeapGetStats - 空指针 */
TEST_CASE(test_osal_heap_get_stats_null_pointer)
{
    uint32_t current, peak;
    int32_t ret;

    /* 第一个参数为NULL */
    ret = OSAL_HeapGetStats(NULL, &peak);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* 第二个参数为NULL */
    ret = OSAL_HeapGetStats(&current, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: HeapSetThreshold - 成功 */
TEST_CASE(test_osal_heap_set_threshold_success)
{
    int32_t ret;

    /* 设置50%阈值 */
    ret = OSAL_HeapSetThreshold(50);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置80%阈值 */
    ret = OSAL_HeapSetThreshold(80);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置0%阈值 */
    ret = OSAL_HeapSetThreshold(0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置100%阈值 */
    ret = OSAL_HeapSetThreshold(100);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapSetThreshold - 无效参数 */
TEST_CASE(test_osal_heap_set_threshold_invalid)
{
    int32_t ret;

    /* 超过100% */
    ret = OSAL_HeapSetThreshold(101);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_SIZE, ret);

    /* 负数（会被转换为大数） */
    ret = OSAL_HeapSetThreshold((uint32_t)-1);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_SIZE, ret);
}

/* 测试用例: HeapCheckThreshold - 成功 */
TEST_CASE(test_osal_heap_check_threshold_success)
{
    bool exceeded;
    int32_t ret;

    /* 设置阈值 */
    OSAL_HeapSetThreshold(90);

    /* 检查阈值 */
    ret = OSAL_HeapCheckThreshold(&exceeded);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapCheckThreshold - 空指针 */
TEST_CASE(test_osal_heap_check_threshold_null_pointer)
{
    int32_t ret;

    ret = OSAL_HeapCheckThreshold(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 内存对齐测试
 *===========================================================================*/

/* 测试用例: 内存对齐 */
TEST_CASE(test_osal_malloc_alignment)
{
    void *ptr;

    /* 分配内存 */
    ptr = OSAL_Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 验证对齐（通常是8字节或16字节对齐） */
    uintptr_t addr = (uintptr_t)ptr;
    TEST_ASSERT_EQUAL(0, addr % sizeof(void *));

    OSAL_Free(ptr);
}

/*===========================================================================
 * 内存泄漏检测测试
 *===========================================================================*/

/* 测试用例: 多次分配释放 */
TEST_CASE(test_osal_malloc_free_multiple)
{
    const int32_t iterations = 100;
    void *ptrs[100];

    /* 分配多块内存 */
    for (int32_t i = 0; i < iterations; i++) {
        ptrs[i] = OSAL_Malloc(64);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* 释放所有内存 */
    for (int32_t i = 0; i < iterations; i++) {
        OSAL_Free(ptrs[i]);
    }
}

/* 测试用例: 交替分配释放 */
TEST_CASE(test_osal_malloc_free_interleaved)
{
    void *ptr1, *ptr2, *ptr3;

    ptr1 = OSAL_Malloc(64);
    TEST_ASSERT_NOT_NULL(ptr1);

    ptr2 = OSAL_Malloc(128);
    TEST_ASSERT_NOT_NULL(ptr2);

    OSAL_Free(ptr1);

    ptr3 = OSAL_Malloc(256);
    TEST_ASSERT_NOT_NULL(ptr3);

    OSAL_Free(ptr2);
    OSAL_Free(ptr3);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: 大内存分配 */
TEST_CASE(test_osal_malloc_large)
{
    void *ptr;
    osal_size_t large_size = 10 * 1024 * 1024;  /* 10MB */

    /* 尝试分配大块内存 */
    ptr = OSAL_Malloc(large_size);
    if (NULL != ptr) {
        /* 如果分配成功，验证可以写入 */
        uint8_t *byte_ptr = (uint8_t *)ptr;
        byte_ptr[0] = 0xFF;
        byte_ptr[large_size - 1] = 0xFF;
        TEST_ASSERT_EQUAL(0xFF, byte_ptr[0]);
        TEST_ASSERT_EQUAL(0xFF, byte_ptr[large_size - 1]);

        OSAL_Free(ptr);
    }
    /* 如果分配失败，也是可接受的（系统内存不足） */
}

/* 测试用例: 内存写入验证 */
TEST_CASE(test_osal_malloc_write_verify)
{
    uint8_t *ptr;
    osal_size_t size = 256;

    ptr = (uint8_t *)OSAL_Malloc(size);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 写入数据 */
    for (osal_size_t i = 0; i < size; i++) {
        ptr[i] = (uint8_t)(i & 0xFF);
    }

    /* 验证数据 */
    for (osal_size_t i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL((uint8_t)(i & 0xFF), ptr[i]);
    }

    OSAL_Free(ptr);
}

/* 测试用例: 内存统计验证 */
TEST_CASE(test_osal_heap_stats_verify)
{
    uint32_t current1, peak1, current2, peak2;
    void *ptr;

    /* 获取初始统计 */
    OSAL_HeapGetStats(&current1, &peak1);

    /* 分配内存 */
    ptr = OSAL_Malloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 获取新统计 */
    OSAL_HeapGetStats(&current2, &peak2);

    /* 验证统计增加 */
    TEST_ASSERT_TRUE(current2 >= current1);
    TEST_ASSERT_TRUE(peak2 >= peak1);

    /* 释放内存 */
    OSAL_Free(ptr);
}

/*===========================================================================
 * 性能测试
 *===========================================================================*/

/* 测试用例: 分配性能 */
TEST_CASE(test_osal_malloc_performance)
{
    const int32_t iterations = 1000;
    void *ptrs[1000];
    uint64_t start_time, end_time;

    /* 测试分配性能 */
    start_time = OSAL_GetTickCount();
    for (int32_t i = 0; i < iterations; i++) {
        ptrs[i] = OSAL_Malloc(64);
    }
    end_time = OSAL_GetTickCount();

    /* 验证所有分配成功 */
    for (int32_t i = 0; i < iterations; i++) {
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* 平均每次分配应该小于1ms */
    uint64_t elapsed = end_time - start_time;
    TEST_ASSERT_TRUE(elapsed < (uint64_t)iterations);

    /* 释放所有内存 */
    for (int32_t i = 0; i < iterations; i++) {
        OSAL_Free(ptrs[i]);
    }
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_osal_heap, "osal_heap", "OSAL")
    // OSAL内存管理测试
    /* 内存分配 */
    TEST_CASE_REF(test_osal_malloc_success)
    TEST_CASE_REF(test_osal_malloc_zero_size)
    TEST_CASE_REF(test_osal_free_success)
    TEST_CASE_REF(test_osal_free_null_pointer)

    /* 堆信息 */
    TEST_CASE_REF(test_osal_heap_get_info_success)
    TEST_CASE_REF(test_osal_heap_get_info_null_pointer)
    TEST_CASE_REF(test_osal_heap_get_stats_success)
    TEST_CASE_REF(test_osal_heap_get_stats_null_pointer)
    TEST_CASE_REF(test_osal_heap_set_threshold_success)
    TEST_CASE_REF(test_osal_heap_set_threshold_invalid)
    TEST_CASE_REF(test_osal_heap_check_threshold_success)
    TEST_CASE_REF(test_osal_heap_check_threshold_null_pointer)

    /* 内存对齐 */
    TEST_CASE_REF(test_osal_malloc_alignment)

    /* 内存泄漏检测 */
    TEST_CASE_REF(test_osal_malloc_free_multiple)
    TEST_CASE_REF(test_osal_malloc_free_interleaved)

    /* 边界条件 */
    TEST_CASE_REF(test_osal_malloc_large)
    TEST_CASE_REF(test_osal_malloc_write_verify)
    TEST_CASE_REF(test_osal_heap_stats_verify)

    /* 性能测试 */
    TEST_CASE_REF(test_osal_malloc_performance)
TEST_SUITE_END(test_osal_heap, "test_osal_heap", "OSAL")
