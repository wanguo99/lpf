#include <test/test_framework.h>
/**
 * @file test_osal_file.c
 * @brief OSAL文件I/O操作单元测试
 */

#include "osal.h"

/* 测试文件路径 */
#define TEST_FILE_PATH "/tmp/osal_test_file.txt"
#define TEST_FILE_PATH2 "/tmp/osal_test_file2.txt"

/*===========================================================================
 * 文件打开/关闭测试
 *===========================================================================*/

/* 测试用例: open/close - 成功 */
static void test_osal_file_open_close_success(void)
{
    int32_t fd;

    /* 创建新文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);

    /* 关闭文件 */
    int32_t ret = OSAL_close(fd);
    TEST_ASSERT_EQUAL(0, ret);

    /* 清理 */
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: open - 只读模式 */
static void test_osal_file_open_readonly(void)
{
    int32_t fd;

    /* 先创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_close(fd);

    /* 以只读模式打开 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDONLY, 0);
    TEST_ASSERT_TRUE(fd >= 0);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: open - 文件不存在 */
static void test_osal_file_open_not_exist(void)
{
    int32_t fd;

    /* 确保文件不存在 */
    OSAL_unlink(TEST_FILE_PATH);

    /* 尝试打开不存在的文件（不创建） */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDONLY, 0);
    TEST_ASSERT_TRUE(fd < 0);
}

/* 测试用例: open - O_EXCL标志 */
static void test_osal_file_open_excl(void)
{
    int32_t fd1, fd2;

    /* 清理 */
    OSAL_unlink(TEST_FILE_PATH);

    /* 创建新文件 */
    fd1 = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_EXCL,
                    OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd1 >= 0);
    OSAL_close(fd1);

    /* 尝试再次创建（应该失败） */
    fd2 = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_EXCL,
                    OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd2 < 0);

    OSAL_unlink(TEST_FILE_PATH);
}

/*===========================================================================
 * 文件读写测试
 *===========================================================================*/

/* 测试用例: write/read - 成功 */
static void test_osal_file_write_read_success(void)
{
    int32_t fd;
    osal_ssize_t ret;
    const char *write_data = "Hello, OSAL!";
    char read_buffer[64];

    /* 创建并写入文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);

    ret = OSAL_write(fd, write_data, OSAL_strlen(write_data));
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(write_data), ret);

    /* 移动到文件开头 */
    osal_off_t pos = OSAL_lseek(fd, 0, OSAL_SEEK_SET);
    TEST_ASSERT_EQUAL(0, pos);

    /* 读取数据 */
    OSAL_memset(read_buffer, 0, OSAL_sizeof(read_buffer));
    ret = OSAL_read(fd, read_buffer, OSAL_sizeof(read_buffer));
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(write_data), ret);
    TEST_ASSERT_EQUAL(0, OSAL_strncmp(read_buffer, write_data, OSAL_strlen(write_data)));

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: write - 追加模式 */
static void test_osal_file_write_append(void)
{
    int32_t fd;
    osal_ssize_t ret;
    const char *data1 = "First line\n";
    const char *data2 = "Second line\n";
    char read_buffer[128];

    /* 创建文件并写入第一行 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    ret = OSAL_write(fd, data1, OSAL_strlen(data1));
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(data1), ret);
    OSAL_close(fd);

    /* 以追加模式打开并写入第二行 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_APPEND, 0);
    TEST_ASSERT_TRUE(fd >= 0);
    ret = OSAL_write(fd, data2, OSAL_strlen(data2));
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(data2), ret);
    OSAL_close(fd);

    /* 读取并验证 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDONLY, 0);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_memset(read_buffer, 0, OSAL_sizeof(read_buffer));
    ret = OSAL_read(fd, read_buffer, OSAL_sizeof(read_buffer));
    TEST_ASSERT_EQUAL((osal_ssize_t)(OSAL_strlen(data1) + OSAL_strlen(data2)), ret);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: read - EOF */
static void test_osal_file_read_eof(void)
{
    int32_t fd;
    osal_ssize_t ret;
    const char *data = "Test";
    char buffer[64];

    /* 创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_write(fd, data, OSAL_strlen(data));

    /* 移动到文件开头并读取 */
    OSAL_lseek(fd, 0, OSAL_SEEK_SET);
    ret = OSAL_read(fd, buffer, OSAL_sizeof(buffer));
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(data), ret);

    /* 再次读取（应该返回0，表示EOF） */
    ret = OSAL_read(fd, buffer, OSAL_sizeof(buffer));
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/*===========================================================================
 * 文件定位测试
 *===========================================================================*/

/* 测试用例: lseek - SEEK_SET */
static void test_osal_file_lseek_set(void)
{
    int32_t fd;
    osal_ssize_t pos;
    const char *data = "0123456789";
    char buffer[2];

    /* 创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_write(fd, data, OSAL_strlen(data));

    /* 移动到位置5 */
    pos = OSAL_lseek(fd, 5, OSAL_SEEK_SET);
    TEST_ASSERT_EQUAL(5, pos);

    /* 读取1个字节 */
    OSAL_memset(buffer, 0, OSAL_sizeof(buffer));
    OSAL_read(fd, buffer, 1);
    TEST_ASSERT_EQUAL('5', buffer[0]);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: lseek - SEEK_CUR */
static void test_osal_file_lseek_cur(void)
{
    int32_t fd;
    osal_ssize_t pos;
    const char *data = "0123456789";
    char buffer[2];

    /* 创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_write(fd, data, OSAL_strlen(data));

    /* 移动到开头 */
    OSAL_lseek(fd, 0, OSAL_SEEK_SET);

    /* 读取3个字节（当前位置=3） */
    OSAL_read(fd, buffer, 1);
    OSAL_read(fd, buffer, 1);
    OSAL_read(fd, buffer, 1);

    /* 从当前位置向前移动2个字节 */
    pos = OSAL_lseek(fd, 2, OSAL_SEEK_CUR);
    TEST_ASSERT_EQUAL(5, pos);

    /* 读取1个字节 */
    OSAL_memset(buffer, 0, OSAL_sizeof(buffer));
    OSAL_read(fd, buffer, 1);
    TEST_ASSERT_EQUAL('5', buffer[0]);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: lseek - SEEK_END */
static void test_osal_file_lseek_end(void)
{
    int32_t fd;
    osal_ssize_t pos;
    const char *data = "0123456789";

    /* 创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_write(fd, data, OSAL_strlen(data));

    /* 移动到文件末尾 */
    pos = OSAL_lseek(fd, 0, OSAL_SEEK_END);
    TEST_ASSERT_EQUAL((osal_ssize_t)OSAL_strlen(data), pos);

    /* 从末尾向前移动5个字节 */
    pos = OSAL_lseek(fd, -5, OSAL_SEEK_END);
    TEST_ASSERT_EQUAL((osal_ssize_t)(OSAL_strlen(data) - 5), pos);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/*===========================================================================
 * 文件控制测试
 *===========================================================================*/

/* 测试用例: fcntl - 获取/设置标志 */
static void test_osal_file_fcntl_flags(void)
{
    int32_t fd;
    int32_t flags;

    /* 创建文件 */
    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);

    /* 获取文件状态标志 */
    flags = OSAL_fcntl(fd, OSAL_F_GETFL, 0);
    TEST_ASSERT_TRUE(flags >= 0);

    /* 设置非阻塞标志 */
    int32_t ret = OSAL_fcntl(fd, OSAL_F_SETFL, flags | OSAL_O_NONBLOCK);
    TEST_ASSERT_EQUAL(0, ret);

    /* 验证标志已设置 */
    flags = OSAL_fcntl(fd, OSAL_F_GETFL, 0);
    TEST_ASSERT_TRUE((flags & OSAL_O_NONBLOCK) != 0);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/*===========================================================================
 * 边界条件测试
 *===========================================================================*/

/* 测试用例: write - 空数据 */
static void test_osal_file_write_empty(void)
{
    int32_t fd;
    osal_ssize_t ret;

    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);

    /* 写入0字节 */
    ret = OSAL_write(fd, "test", 0);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: read - 空缓冲区 */
static void test_osal_file_read_empty(void)
{
    int32_t fd;
    osal_ssize_t ret;
    char buffer[64];

    fd = OSAL_open(TEST_FILE_PATH, OSAL_O_RDWR | OSAL_O_CREAT | OSAL_O_TRUNC,
                   OSAL_S_IRUSR | OSAL_S_IWUSR);
    TEST_ASSERT_TRUE(fd >= 0);
    OSAL_write(fd, "test", 4);
    OSAL_lseek(fd, 0, OSAL_SEEK_SET);

    /* 读取0字节 */
    ret = OSAL_read(fd, buffer, 0);
    TEST_ASSERT_EQUAL(0, ret);

    OSAL_close(fd);
    OSAL_unlink(TEST_FILE_PATH);
}

/* 测试用例: close - 无效文件描述符 */
static void test_osal_file_close_invalid_fd(void)
{
    int32_t ret;

    /* 关闭无效的文件描述符 */
    ret = OSAL_close(-1);
    TEST_ASSERT_TRUE(ret < 0);

    ret = OSAL_close(9999);
    TEST_ASSERT_TRUE(ret < 0);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

// OSAL文件I/O操作测试
    /* 文件打开/关闭 */
    /* 文件读写 */
    /* 文件定位 */
    /* 文件控制 */
    /* 边界条件 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_osal_file_open_close_success",
		.func = test_osal_file_open_close_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_open_readonly",
		.func = test_osal_file_open_readonly,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_open_not_exist",
		.func = test_osal_file_open_not_exist,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_open_excl",
		.func = test_osal_file_open_excl,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_write_read_success",
		.func = test_osal_file_write_read_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_write_append",
		.func = test_osal_file_write_append,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_read_eof",
		.func = test_osal_file_read_eof,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_lseek_set",
		.func = test_osal_file_lseek_set,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_lseek_cur",
		.func = test_osal_file_lseek_cur,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_lseek_end",
		.func = test_osal_file_lseek_end,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_fcntl_flags",
		.func = test_osal_file_fcntl_flags,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_write_empty",
		.func = test_osal_file_write_empty,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_read_empty",
		.func = test_osal_file_read_empty,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_osal_file_close_invalid_fd",
		.func = test_osal_file_close_invalid_fd,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "osal_file",
	.module_name = "osal_file",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "OSAL osal_file tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_osal_file_tests(void)
{
	libutest_register_suite(&test_suite);
}
