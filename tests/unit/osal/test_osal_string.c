/**
 * @file test_osal_string.c
 * @brief OSAL字符串和内存操作单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"

/*===========================================================================
 * 内存操作测试
 *===========================================================================*/

/* 测试用例: Memset - 成功 */
TEST_CASE(test_osal_memset_success)
{
    uint8_t buffer[64];
    void *ret;

    /* 填充0 */
    ret = OSAL_Memset(buffer, 0, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(buffer, ret);

    for (uint32_t i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }

    /* 填充0xFF */
    ret = OSAL_Memset(buffer, 0xFF, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(ret);

    for (uint32_t i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL(0xFF, buffer[i]);
    }
}

/* 测试用例: Memcpy - 成功 */
TEST_CASE(test_osal_memcpy_success)
{
    uint8_t src[32] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t dest[32];
    void *ret;

    OSAL_Memset(dest, 0, sizeof(dest));

    ret = OSAL_Memcpy(dest, src, 8);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);

    for (uint32_t i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(src[i], dest[i]);
    }
}

/* 测试用例: Memmove - 重叠区域 */
TEST_CASE(test_osal_memmove_overlap)
{
    uint8_t buffer[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    void *ret;

    /* 向后移动（重叠） */
    ret = OSAL_Memmove(buffer + 4, buffer, 8);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(buffer + 4, ret);

    /* 验证数据正确移动 */
    TEST_ASSERT_EQUAL(1, buffer[4]);
    TEST_ASSERT_EQUAL(2, buffer[5]);
    TEST_ASSERT_EQUAL(8, buffer[11]);
}

/* 测试用例: Memcmp - 相等 */
TEST_CASE(test_osal_memcmp_equal)
{
    uint8_t buf1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t buf2[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    int32_t ret = OSAL_Memcmp(buf1, buf2, 8);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Memcmp - 不相等 */
TEST_CASE(test_osal_memcmp_not_equal)
{
    uint8_t buf1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t buf2[8] = {1, 2, 3, 5, 5, 6, 7, 8};  /* 第4个字节不同 */

    int32_t ret = OSAL_Memcmp(buf1, buf2, 8);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_TRUE(ret < 0);  /* buf1[3] < buf2[3] */
}

/*===========================================================================
 * 字符串操作测试
 *===========================================================================*/

/* 测试用例: Strlen - 成功 */
TEST_CASE(test_osal_strlen_success)
{
    const char *str1 = "Hello";
    const char *str2 = "";
    const char *str3 = "Hello, World!";

    TEST_ASSERT_EQUAL(5, OSAL_Strlen(str1));
    TEST_ASSERT_EQUAL(0, OSAL_Strlen(str2));
    TEST_ASSERT_EQUAL(13, OSAL_Strlen(str3));
}

/* 测试用例: Strcmp - 相等 */
TEST_CASE(test_osal_strcmp_equal)
{
    const char *str1 = "Hello";
    const char *str2 = "Hello";

    int32_t ret = OSAL_Strcmp(str1, str2);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Strcmp - 不相等 */
TEST_CASE(test_osal_strcmp_not_equal)
{
    const char *str1 = "Hello";
    const char *str2 = "World";

    int32_t ret = OSAL_Strcmp(str1, str2);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_TRUE(ret < 0);  /* "Hello" < "World" */

    ret = OSAL_Strcmp(str2, str1);
    TEST_ASSERT_TRUE(ret > 0);  /* "World" > "Hello" */
}

/* 测试用例: Strncmp - 成功 */
TEST_CASE(test_osal_strncmp_success)
{
    const char *str1 = "Hello World";
    const char *str2 = "Hello Earth";

    /* 前5个字符相同 */
    int32_t ret = OSAL_Strncmp(str1, str2, 5);
    TEST_ASSERT_EQUAL(0, ret);

    /* 前7个字符不同 */
    ret = OSAL_Strncmp(str1, str2, 7);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

/* 测试用例: Strcasecmp - 忽略大小写 */
TEST_CASE(test_osal_strcasecmp_success)
{
    const char *str1 = "Hello";
    const char *str2 = "HELLO";
    const char *str3 = "hello";

    int32_t ret = OSAL_Strcasecmp(str1, str2);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_Strcasecmp(str1, str3);
    TEST_ASSERT_EQUAL(0, ret);

    ret = OSAL_Strcasecmp(str2, str3);
    TEST_ASSERT_EQUAL(0, ret);
}

/* 测试用例: Strcpy - 成功 */
TEST_CASE(test_osal_strcpy_success)
{
    const char *src = "Hello";
    char dest[32];
    char *ret;

    OSAL_Memset(dest, 0, sizeof(dest));

    ret = OSAL_Strcpy(dest, src);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(dest, src));
}

/* 测试用例: Strncpy - 成功 */
TEST_CASE(test_osal_strncpy_success)
{
    const char *src = "Hello World";
    char dest[32];
    char *ret;

    OSAL_Memset(dest, 0, sizeof(dest));

    /* 拷贝前5个字符 */
    ret = OSAL_Strncpy(dest, src, 5);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_Strncmp(dest, "Hello", 5));
}

/* 测试用例: Strcat - 成功 */
TEST_CASE(test_osal_strcat_success)
{
    char dest[32] = "Hello";
    const char *src = " World";
    char *ret;

    ret = OSAL_Strcat(dest, src);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(dest, "Hello World"));
}

/* 测试用例: Strncat - 成功 */
TEST_CASE(test_osal_strncat_success)
{
    char dest[32] = "Hello";
    const char *src = " World";
    char *ret;

    /* 只追加3个字符 */
    ret = OSAL_Strncat(dest, src, 3);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(dest, ret);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(dest, "Hello Wo"));
}

/* 测试用例: Strstr - 找到子串 */
TEST_CASE(test_osal_strstr_found)
{
    const char *haystack = "Hello World";
    const char *needle = "World";

    char *ret = OSAL_Strstr(haystack, needle);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(ret, "World"));
}

/* 测试用例: Strstr - 未找到子串 */
TEST_CASE(test_osal_strstr_not_found)
{
    const char *haystack = "Hello World";
    const char *needle = "Earth";

    char *ret = OSAL_Strstr(haystack, needle);
    TEST_ASSERT_NULL(ret);
}

/*===========================================================================
 * 字符串格式化测试
 *===========================================================================*/

/* 测试用例: Sprintf - 成功 */
TEST_CASE(test_osal_sprintf_success)
{
    char buffer[64];
    int32_t ret;

    ret = OSAL_Sprintf(buffer, "Hello %s, number %d", "World", 42);
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(buffer, "Hello World, number 42"));
}

/* 测试用例: Snprintf - 成功 */
TEST_CASE(test_osal_snprintf_success)
{
    char buffer[16];
    int32_t ret;

    /* 正常情况 */
    ret = OSAL_Snprintf(buffer, sizeof(buffer), "Hello %s", "World");
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(buffer, "Hello World"));

    /* 截断情况 */
    ret = OSAL_Snprintf(buffer, sizeof(buffer), "This is a very long string");
    TEST_ASSERT_TRUE(ret > 0);
    TEST_ASSERT_EQUAL(15, OSAL_Strlen(buffer));  /* 最多15个字符（不含\0） */
}

/* 测试用例: Sscanf - 成功 */
TEST_CASE(test_osal_sscanf_success)
{
    const char *str = "Hello 42 3.14";
    char word[32];
    int32_t num;

    int32_t ret = OSAL_Sscanf(str, "%s %d", word, &num);
    TEST_ASSERT_EQUAL(2, ret);  /* 成功解析2个字段 */
    TEST_ASSERT_EQUAL(0, OSAL_Strcmp(word, "Hello"));
    TEST_ASSERT_EQUAL(42, num);
}

/*===========================================================================
 * 字符串转换测试
 *===========================================================================*/

/* 测试用例: Atoi - 成功 */
TEST_CASE(test_osal_atoi_success)
{
    TEST_ASSERT_EQUAL(0, OSAL_Atoi("0"));
    TEST_ASSERT_EQUAL(42, OSAL_Atoi("42"));
    TEST_ASSERT_EQUAL(-42, OSAL_Atoi("-42"));
    TEST_ASSERT_EQUAL(123, OSAL_Atoi("123"));
    TEST_ASSERT_EQUAL(123, OSAL_Atoi("  123"));  /* 前导空格 */
}

/* 测试用例: Atoi - 无效输入 */
TEST_CASE(test_osal_atoi_invalid)
{
    TEST_ASSERT_EQUAL(0, OSAL_Atoi("abc"));
    TEST_ASSERT_EQUAL(0, OSAL_Atoi(""));
    TEST_ASSERT_EQUAL(123, OSAL_Atoi("123abc"));  /* 部分解析 */
}

/* 测试用例: Atol - 成功 */
TEST_CASE(test_osal_atol_success)
{
    TEST_ASSERT_EQUAL(0, OSAL_Atol("0"));
    TEST_ASSERT_EQUAL(123456789, OSAL_Atol("123456789"));
    TEST_ASSERT_EQUAL(-123456789, OSAL_Atol("-123456789"));
}

/* 测试用例: Strtol - 不同进制 */
TEST_CASE(test_osal_strtol_base)
{
    char *endptr;
    int64_t ret;

    /* 十进制 */
    ret = OSAL_Strtol("123", &endptr, 10);
    TEST_ASSERT_EQUAL(123, ret);

    /* 十六进制 */
    ret = OSAL_Strtol("FF", &endptr, 16);
    TEST_ASSERT_EQUAL(255, ret);

    /* 八进制 */
    ret = OSAL_Strtol("77", &endptr, 8);
    TEST_ASSERT_EQUAL(63, ret);

    /* 二进制 */
    ret = OSAL_Strtol("1010", &endptr, 2);
    TEST_ASSERT_EQUAL(10, ret);

    /* 自动检测（0x前缀） */
    ret = OSAL_Strtol("0xFF", &endptr, 0);
    TEST_ASSERT_EQUAL(255, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_osal_string, "osal_string", "OSAL")
    // OSAL字符串和内存操作测试
    /* 内存操作 */
    TEST_CASE_REF(test_osal_memset_success)
    TEST_CASE_REF(test_osal_memcpy_success)
    TEST_CASE_REF(test_osal_memmove_overlap)
    TEST_CASE_REF(test_osal_memcmp_equal)
    TEST_CASE_REF(test_osal_memcmp_not_equal)

    /* 字符串操作 */
    TEST_CASE_REF(test_osal_strlen_success)
    TEST_CASE_REF(test_osal_strcmp_equal)
    TEST_CASE_REF(test_osal_strcmp_not_equal)
    TEST_CASE_REF(test_osal_strncmp_success)
    TEST_CASE_REF(test_osal_strcasecmp_success)
    TEST_CASE_REF(test_osal_strcpy_success)
    TEST_CASE_REF(test_osal_strncpy_success)
    TEST_CASE_REF(test_osal_strcat_success)
    TEST_CASE_REF(test_osal_strncat_success)
    TEST_CASE_REF(test_osal_strstr_found)
    TEST_CASE_REF(test_osal_strstr_not_found)

    /* 字符串格式化 */
    TEST_CASE_REF(test_osal_sprintf_success)
    TEST_CASE_REF(test_osal_snprintf_success)
    TEST_CASE_REF(test_osal_sscanf_success)

    /* 字符串转换 */
    TEST_CASE_REF(test_osal_atoi_success)
    TEST_CASE_REF(test_osal_atoi_invalid)
    TEST_CASE_REF(test_osal_atol_success)
    TEST_CASE_REF(test_osal_strtol_base)
TEST_SUITE_END(test_osal_string, "test_osal_string", "OSAL")
