#include <test_framework/test_framework.h>
/**
 * @file test_osal_heap.c
 * @brief OSAL内存管理单元测试
 */

#include "osal.h"

/*===========================================================================
 * 内存分配测试
 *===========================================================================*/

/* 测试用例: Malloc - 成功 */
static void test_osal_malloc_success(void)
{
    void *ptr;

    /* 分配小块内存 */
    ptr = OSAL_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_free(ptr);

    /* 分配中等内存 */
    ptr = OSAL_malloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_free(ptr);

    /* 分配大块内存 */
    ptr = OSAL_malloc(1024 * 1024); /* 1MB */
    TEST_ASSERT_NOT_NULL(ptr);
    OSAL_free(ptr);
}

/* 测试用例: Malloc - 零大小 */
static void test_osal_malloc_zero_size(void)
{
    void *ptr;

    /* 分配0字节（行为依赖于实现） */
    ptr = OSAL_malloc(0);
    /* 某些实现返回NULL，某些返回有效指针 */
    if (NULL != ptr) {
        OSAL_free(ptr);
    }
}

/* 测试用例: Free - 成功 */
static void test_osal_free_success(void)
{
    void *ptr;

    ptr = OSAL_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 释放内存（不应崩溃） */
    OSAL_free(ptr);
}

/* 测试用例: Free - NULL指针 */
static void test_osal_free_null_pointer(void)
{
    /* Free(NULL) 应该安全（不崩溃） */
    OSAL_free(NULL);
}

/*===========================================================================
 * 堆信息测试
 *===========================================================================*/

/* 测试用例: HeapGetInfo - 成功 */
static void test_osal_heap_get_info_success(void)
{
    uint32_t free_bytes, total_bytes;
    int32_t ret;

    ret = OSAL_heap_get_info(&free_bytes, &total_bytes);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(total_bytes > 0);
    TEST_ASSERT_TRUE(free_bytes <= total_bytes);
}

/* 测试用例: HeapGetInfo - 空指针 */
static void test_osal_heap_get_info_null_pointer(void)
{
    uint32_t free_bytes, total_bytes;
    int32_t ret;

    /* 第一个参数为NULL */
    ret = OSAL_heap_get_info(NULL, &total_bytes);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 第二个参数为NULL */
    ret = OSAL_heap_get_info(&free_bytes, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* 两个参数都为NULL */
    ret = OSAL_heap_get_info(NULL, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapGetStats - 成功 */
static void test_osal_heap_get_stats_success(void)
{
    uint32_t current, peak;
    int32_t ret;

    ret = OSAL_heap_get_stats(&current, &peak);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(peak >= current);
}

/* 测试用例: HeapGetStats - 空指针 */
static void test_osal_heap_get_stats_null_pointer(void)
{
    uint32_t current, peak;
    int32_t ret;

    /* 第一个参数为NULL */
    ret = OSAL_heap_get_stats(NULL, &peak);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* 第二个参数为NULL */
    ret = OSAL_heap_get_stats(&current, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: HeapSetThreshold - 成功 */
static void test_osal_heap_set_threshold_success(void)
{
    int32_t ret;

    /* 设置50%阈值 */
    ret = OSAL_heap_set_threshold(50);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置80%阈值 */
    ret = OSAL_heap_set_threshold(80);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置0%阈值 */
    ret = OSAL_heap_set_threshold(0);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 设置100%阈值 */
    ret = OSAL_heap_set_threshold(100);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapSetThreshold - 无效参数 */
static void test_osal_heap_set_threshold_invalid(void)
{
    int32_t ret;

    /* 超过100% */
    ret = OSAL_heap_set_threshold(101);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_SIZE, ret);

    /* 负数（会被转换为大数） */
    ret = OSAL_heap_set_threshold((uint32_t)-1);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_SIZE, ret);
}

/* 测试用例: HeapCheckThreshold - 成功 */
static void test_osal_heap_check_threshold_success(void)
{
    bool exceeded;
    int32_t ret;

    /* 设置阈值 */
    OSAL_heap_set_threshold(90);

    /* 检查阈值 */
    ret = OSAL_heap_check_threshold(&exceeded);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: HeapCheckThreshold - 空指针 */
static void test_osal_heap_check_threshold_null_pointer(void)
{
    int32_t ret;

    ret = OSAL_heap_check_threshold(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/*===========================================================================
 * 内存对齐测试
 *===========================================================================*/

/* 测试用例: 内存对齐 */
static void test_osal_malloc_alignment(void)
{
    void *ptr;

    /* 分配内存 */
    ptr = OSAL_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 验证对齐（通常是8字节或16字节对齐） */
    uintptr_t addr = (uintptr_t)ptr;
    TEST_ASSERT_EQUAL(0, addr % OSAL_sizeof(void *));

    OSAL_free(ptr);
}

/*===========================================================================
 * 内存泄漏检测测试
 *===========================================================================*/

/* 测试用例: 多次分配释放 */
static void test_osal_malloc_free_multiple(void)
{
    const int32_t iterations = 100;
    void *ptrs[100];

    /* 分配多块内存 */
    int32_t i;

    for (i = 0; i < iterations; i++) {
        ptrs[i] = OSAL_malloc(64);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* 释放所有内存 */

    for (i = 0; i < iterations; i++) {
        OSAL_free(ptrs[i]);
    }
}

/* 测试用例: 交替分配释放 */
static void test_osal_malloc_free_interleaved(void)
{
    void *ptr1, *ptr2, *ptr3;

    ptr1 = OSAL_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr1);

    ptr2 = OSAL_malloc(128);
    TEST_ASSERT_NOT_NULL(ptr2);

    OSAL_free(ptr1);

    ptr3 = OSAL_malloc(256);
    TEST_ASSERT_NOT_NULL(ptr3);

    OSAL_free(ptr2);
    OSAL_free(ptr3);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: 大内存分配 */
static void test_osal_malloc_large(void)
{
    void *ptr;
    osal_size_t large_size = 10 * 1024 * 1024; /* 10MB */

    /* 尝试分配大块内存 */
    ptr = OSAL_malloc(large_size);
    if (NULL != ptr) {
        /* 如果分配成功，验证可以写入 */
        uint8_t *byte_ptr = (uint8_t *)ptr;
        byte_ptr[0] = 0xFF;
        byte_ptr[large_size - 1] = 0xFF;
        TEST_ASSERT_EQUAL(0xFF, byte_ptr[0]);
        TEST_ASSERT_EQUAL(0xFF, byte_ptr[large_size - 1]);

        OSAL_free(ptr);
    }
    /* 如果分配失败，也是可接受的（系统内存不足） */
}

/* 测试用例: 内存写入验证 */
static void test_osal_malloc_write_verify(void)
{
    uint8_t *ptr;
    osal_size_t size = 256;

    ptr = (uint8_t *)OSAL_malloc(size);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 写入数据 */
    osal_size_t i;

    for (i = 0; i < size; i++) {
        ptr[i] = (uint8_t)(i & 0xFF);
    }

    /* 验证数据 */

    for (i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL((uint8_t)(i & 0xFF), ptr[i]);
    }

    OSAL_free(ptr);
}

/* 测试用例: 内存统计验证 */
static void test_osal_heap_stats_verify(void)
{
    uint32_t current1, peak1, current2, peak2;
    void *ptr;

    /* 获取初始统计 */
    OSAL_heap_get_stats(&current1, &peak1);

    /* 分配内存 */
    ptr = OSAL_malloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);

    /* 获取新统计 */
    OSAL_heap_get_stats(&current2, &peak2);

    /* 验证统计增加 */
    TEST_ASSERT_TRUE(current2 >= current1);
    TEST_ASSERT_TRUE(peak2 >= peak1);

    /* 释放内存 */
    OSAL_free(ptr);
}

/*===========================================================================
 * 性能测试
 *===========================================================================*/

/* 测试用例: 分配性能 */
static void test_osal_malloc_performance(void)
{
    const int32_t iterations = 1000;
    void *ptrs[1000];
    uint64_t start_time, end_time;

    /* 测试分配性能 */
    start_time = OSAL_get_tick_count();
    int32_t i;

    for (i = 0; i < iterations; i++) {
        ptrs[i] = OSAL_malloc(64);
    }
    end_time = OSAL_get_tick_count();

    /* 验证所有分配成功 */

    for (i = 0; i < iterations; i++) {
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* 平均每次分配应该小于1ms */
    uint64_t elapsed = end_time - start_time;
    TEST_ASSERT_TRUE(elapsed < (uint64_t)iterations);

    /* 释放所有内存 */

    for (i = 0; i < iterations; i++) {
        OSAL_free(ptrs[i]);
    }
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// OSAL内存管理测试
/* 内存分配 */
/* 堆信息 */
/* 内存对齐 */
/* 内存泄漏检测 */
/* 边界条件 */
/* 性能测试 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
    { .name = "test_osal_malloc_success",
      .func = test_osal_malloc_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_zero_size",
      .func = test_osal_malloc_zero_size,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_free_success",
      .func = test_osal_free_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_free_null_pointer",
      .func = test_osal_free_null_pointer,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_get_info_success",
      .func = test_osal_heap_get_info_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_get_info_null_pointer",
      .func = test_osal_heap_get_info_null_pointer,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_get_stats_success",
      .func = test_osal_heap_get_stats_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_get_stats_null_pointer",
      .func = test_osal_heap_get_stats_null_pointer,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_set_threshold_success",
      .func = test_osal_heap_set_threshold_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_set_threshold_invalid",
      .func = test_osal_heap_set_threshold_invalid,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_check_threshold_success",
      .func = test_osal_heap_check_threshold_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_check_threshold_null_pointer",
      .func = test_osal_heap_check_threshold_null_pointer,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_alignment",
      .func = test_osal_malloc_alignment,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_free_multiple",
      .func = test_osal_malloc_free_multiple,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_free_interleaved",
      .func = test_osal_malloc_free_interleaved,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_large",
      .func = test_osal_malloc_large,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_write_verify",
      .func = test_osal_malloc_write_verify,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_heap_stats_verify",
      .func = test_osal_heap_stats_verify,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_malloc_performance",
      .func = test_osal_malloc_performance,
      .setup = NULL,
      .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
    .suite_name = "osal_heap",
    .module_name = "osal_heap",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = { .category = TEST_CATEGORY_UNIT,
                  .tags = TEST_TAG_FAST,
                  .timeout_ms = 100,
                  .description = "OSAL osal_heap tests" }
};

/* 测试套件注册函数 */
__attribute__((constructor)) static void register_osal_heap_tests(void)
{
    libutest_register_suite(&test_suite);
}
