#include <test_framework/test_framework.h>
/**
 * @file test_osal_string.c
 * @brief OSAL字符串和内存操作单元测试
 */

#include "osal.h"

/*===========================================================================
 * 内存操作测试
 *===========================================================================*/

/* 测试用例: Memset - 成功 */
static void test_osal_memset_success(void)
{
    uint32_t i;
    uint8_t buffer[64];
    void *ret;

    /* 填充0 */
    ret = OSAL_memset(buffer, 0, OSAL_sizeof(buffer));
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(buffer, ret);

    for (i = 0; i < OSAL_sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }

    /* 填充0xFF */
    ret = OSAL_memset(buffer, 0xFF, OSAL_sizeof(buffer));
    TEST_ASSERT_NOT_NULL(ret);

    for (i = 0; i < OSAL_sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL(0xFF, buffer[i]);
    }
}

/* 测试用例: Memcpy - 成功 */
static void test_osal_memcpy_success(void)
{
    uint32_t i;
    uint8_t src[32] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t dest[32];
    void *ret;

    OSAL_memset(dest, 0, OSAL_sizeof(dest));

    ret = OSAL_memcpy(dest, src, 8);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);

    for (i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(src[i], dest[i]);
    }
}

/* 测试用例: Memmove - 重叠区域 */
static void test_osal_memmove_overlap(void)
{
    uint8_t buffer[16] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    void *ret;

    /* 向后移动（重叠） */
    ret = OSAL_memmove(buffer + 4, buffer, 8);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(buffer + 4, ret);

    /* 验证数据正确移动 */
    TEST_ASSERT_EQUAL(1, buffer[4]);
    TEST_ASSERT_EQUAL(2, buffer[5]);
    TEST_ASSERT_EQUAL(8, buffer[11]);
}

/* 测试用例: Memcmp - 相等 */
static void test_osal_memcmp_equal(void)
{
    uint8_t buf1[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t buf2[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    int32_t ret = OSAL_memcmp(buf1, buf2, 8);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Memcmp - 不相等 */
static void test_osal_memcmp_not_equal(void)
{
    uint8_t buf1[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t buf2[8] = { 1, 2, 3, 5, 5, 6, 7, 8 }; /* 第4个字节不同 */

    int32_t ret = OSAL_memcmp(buf1, buf2, 8);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_TRUE(ret < 0); /* buf1[3] < buf2[3] */
}

/*===========================================================================
 * 字符串操作测试
 *===========================================================================*/

/* 测试用例: Strlen - 成功 */
static void test_osal_strlen_success(void)
{
    const char *str1 = "Hello";
    const char *str2 = "";
    const char *str3 = "Hello, World!";

    TEST_ASSERT_EQUAL(5, OSAL_strlen(str1));
    TEST_ASSERT_EQUAL(0, OSAL_strlen(str2));
    TEST_ASSERT_EQUAL(13, OSAL_strlen(str3));
}

/* 测试用例: Strcmp - 相等 */
static void test_osal_strcmp_equal(void)
{
    const char *str1 = "Hello";
    const char *str2 = "Hello";

    int32_t ret = OSAL_strcmp(str1, str2);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Strcmp - 不相等 */
static void test_osal_strcmp_not_equal(void)
{
    const char *str1 = "Hello";
    const char *str2 = "World";

    int32_t ret = OSAL_strcmp(str1, str2);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_TRUE(ret < 0); /* "Hello" < "World" */

    ret = OSAL_strcmp(str2, str1);
    TEST_ASSERT_TRUE(ret > 0); /* "World" > "Hello" */
}

/* 测试用例: Strncmp - 成功 */
static void test_osal_strncmp_success(void)
{
    const char *str1 = "Hello World";
    const char *str2 = "Hello Earth";

    /* 前5个字符相同 */
    int32_t ret = OSAL_strncmp(str1, str2, 5);
    TEST_ASSERT_EQUAL(0, ret);

    /* 前7个字符不同 */
    ret = OSAL_strncmp(str1, str2, 7);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

/* 测试用例: Strcasecmp - 忽略大小写 */
static void test_osal_strcasecmp_success(void)
{
    const char *str1 = "Hello";
    const char *str2 = "HELLO";
    const char *str3 = "hello";

    int32_t ret = OSAL_strcasecmp(str1, str2);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_strcasecmp(str1, str3);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_strcasecmp(str2, str3);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Strcpy - 成功 */
static void test_osal_strcpy_success(void)
{
    const char *src = "Hello";
    char dest[32];
    char *ret;

    OSAL_memset(dest, 0, OSAL_sizeof(dest));

    ret = OSAL_strcpy(dest, src);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(dest, src));
}

/* 测试用例: Strncpy - 成功 */
static void test_osal_strncpy_success(void)
{
    const char *src = "Hello World";
    char dest[32];
    char *ret;

    OSAL_memset(dest, 0, OSAL_sizeof(dest));

    /* 拷贝前5个字符 */
    ret = OSAL_strncpy(dest, src, 5);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_strncmp(dest, "Hello", 5));
}

/* 测试用例: Strcat - 成功 */
static void test_osal_strcat_success(void)
{
    char dest[32] = "Hello";
    const char *src = " World";
    char *ret;

    ret = OSAL_strcat(dest, src);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(dest, "Hello World"));
}

/* 测试用例: Strncat - 成功 */
static void test_osal_strncat_success(void)
{
    char dest[32] = "Hello";
    const char *src = " World";
    char *ret;

    /* 只追加3个字符 */
    ret = OSAL_strncat(dest, src, 3);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(dest, "Hello Wo"));
}

/* 测试用例: Strstr - 找到子串 */
static void test_osal_strstr_found(void)
{
    const char *haystack = "Hello World";
    const char *needle = "World";

    char *ret = OSAL_strstr(haystack, needle);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(ret, "World"));
}

/* 测试用例: Strstr - 未找到子串 */
static void test_osal_strstr_not_found(void)
{
    const char *haystack = "Hello World";
    const char *needle = "Earth";

    char *ret = OSAL_strstr(haystack, needle);
    TEST_ASSERT_NULL(ret);
}

/*===========================================================================
 * 字符串格式化测试
 *===========================================================================*/

/* 测试用例: Sprintf - 成功 */
static void test_osal_sprintf_success(void)
{
    char buffer[64];
    int32_t ret;

    ret = OSAL_sprintf(buffer, "Hello %s, number %d", "World", 42);
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(buffer, "Hello World, number 42"));
}

/* 测试用例: Snprintf - 成功 */
static void test_osal_snprintf_success(void)
{
    char buffer[16];
    int32_t ret;

    /* 正常情况 */
    ret = OSAL_snprintf(buffer, OSAL_sizeof(buffer), "Hello %s", "World");
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(buffer, "Hello World"));

    /* 截断情况 */
    ret = OSAL_snprintf(buffer,
                        OSAL_sizeof(buffer),
                        "This is a very long string");
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(15, OSAL_strlen(buffer)); /* 最多15个字符（不含\0） */
}

/* 测试用例: Sscanf - 成功 */
static void test_osal_sscanf_success(void)
{
    const char *str = "Hello 42 3.14";
    char word[32];
    int32_t num;

    int32_t ret = OSAL_sscanf(str, "%s %d", word, &num);
    TEST_ASSERT_EQUAL(2, ret); /* 成功解析2个字段 */
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(word, "Hello"));
    TEST_ASSERT_EQUAL(42, num);
}

/*===========================================================================
 * 字符串转换测试
 *===========================================================================*/

/* 测试用例: Atoi - 成功 */
static void test_osal_atoi_success(void)
{
    TEST_ASSERT_EQUAL(0, OSAL_atoi("0"));
    TEST_ASSERT_EQUAL(42, OSAL_atoi("42"));
    TEST_ASSERT_EQUAL(-42, OSAL_atoi("-42"));
    TEST_ASSERT_EQUAL(123, OSAL_atoi("123"));
    TEST_ASSERT_EQUAL(123, OSAL_atoi("  123")); /* 前导空格 */
}

/* 测试用例: Atoi - 无效输入 */
static void test_osal_atoi_invalid(void)
{
    TEST_ASSERT_EQUAL(0, OSAL_atoi("abc"));
    TEST_ASSERT_EQUAL(0, OSAL_atoi(""));
    TEST_ASSERT_EQUAL(123, OSAL_atoi("123abc")); /* 部分解析 */
}

/* 测试用例: Atol - 成功 */
static void test_osal_atol_success(void)
{
    TEST_ASSERT_EQUAL(0, OSAL_atol("0"));
    TEST_ASSERT_EQUAL(123456789, OSAL_atol("123456789"));
    TEST_ASSERT_EQUAL(-123456789, OSAL_atol("-123456789"));
}

/* 测试用例: Strtol - 不同进制 */
static void test_osal_strtol_base(void)
{
    char *endptr;
    int64_t ret;

    /* 十进制 */
    ret = OSAL_strtol("123", &endptr, 10);
    TEST_ASSERT_EQUAL(123, ret);

    /* 十六进制 */
    ret = OSAL_strtol("FF", &endptr, 16);
    TEST_ASSERT_EQUAL(255, ret);

    /* 八进制 */
    ret = OSAL_strtol("77", &endptr, 8);
    TEST_ASSERT_EQUAL(63, ret);

    /* 二进制 */
    ret = OSAL_strtol("1010", &endptr, 2);
    TEST_ASSERT_EQUAL(10, ret);

    /* 自动检测（0x前缀） */
    ret = OSAL_strtol("0xFF", &endptr, 0);
    TEST_ASSERT_EQUAL(255, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// OSAL字符串和内存操作测试
/* 内存操作 */
/* 字符串操作 */
/* 字符串格式化 */
/* 字符串转换 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
    { .name = "test_osal_memset_success",
      .func = test_osal_memset_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_memcpy_success",
      .func = test_osal_memcpy_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_memmove_overlap",
      .func = test_osal_memmove_overlap,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_memcmp_equal",
      .func = test_osal_memcmp_equal,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_memcmp_not_equal",
      .func = test_osal_memcmp_not_equal,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strlen_success",
      .func = test_osal_strlen_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strcmp_equal",
      .func = test_osal_strcmp_equal,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strcmp_not_equal",
      .func = test_osal_strcmp_not_equal,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strncmp_success",
      .func = test_osal_strncmp_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strcasecmp_success",
      .func = test_osal_strcasecmp_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strcpy_success",
      .func = test_osal_strcpy_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strncpy_success",
      .func = test_osal_strncpy_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strcat_success",
      .func = test_osal_strcat_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strncat_success",
      .func = test_osal_strncat_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strstr_found",
      .func = test_osal_strstr_found,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strstr_not_found",
      .func = test_osal_strstr_not_found,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_sprintf_success",
      .func = test_osal_sprintf_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_snprintf_success",
      .func = test_osal_snprintf_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_sscanf_success",
      .func = test_osal_sscanf_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_atoi_success",
      .func = test_osal_atoi_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_atoi_invalid",
      .func = test_osal_atoi_invalid,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_atol_success",
      .func = test_osal_atol_success,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_osal_strtol_base",
      .func = test_osal_strtol_base,
      .setup = NULL,
      .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
    .suite_name = "osal_string",
    .module_name = "osal_string",
    .layer_name = "OSAL",
    .cases = test_cases,
    .case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = { .category = TEST_CATEGORY_UNIT,
                  .tags = TEST_TAG_FAST,
                  .timeout_ms = 100,
                  .description = "OSAL osal_string tests" }
};

/* 测试套件注册函数 */
__attribute__((constructor)) static void register_osal_string_tests(void)
{
    libutest_register_suite(&test_suite);
}
