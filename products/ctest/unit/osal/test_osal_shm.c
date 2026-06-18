/************************************************************************
 * OSAL共享内存单元测试
 ************************************************************************/

#include "osal.h"
#include <test_framework/test_framework.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/*===========================================================================
 * 测试常量
 *===========================================================================*/

#define TEST_SHM_NAME "/osal_test_shm"
#define TEST_SHM_SIZE 4096

/*===========================================================================
 * 共享内存基础测试
 *===========================================================================*/

static void _test_shm_open_close_success(void)
{
	/* 先清理可能存在的共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);

	/* 创建共享内存 */
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);

	/* 设置大小 */
	int32_t ret = ftruncate(fd, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 关闭 */
	ret = osal_close(fd);
	TEST_ASSERT_EQUAL(0, ret);

	/* 清理 */
	ret = osal_shm_unlink(TEST_SHM_NAME);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_shm_unlink_success(void)
{
	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);
	osal_close(fd);

	/* 删除共享内存 */
	int32_t ret = osal_shm_unlink(TEST_SHM_NAME);
	TEST_ASSERT_EQUAL(0, ret);

	/* 再次删除应该失败 */
	ret = osal_shm_unlink(TEST_SHM_NAME);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}

static void _test_shm_open_existing(void)
{
	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd1 = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd1 >= 0);
	int32_t ret = ftruncate(fd1, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);
	osal_close(fd1);

	/* 打开现有共享内存 */
	int32_t fd2 = osal_shm_open(TEST_SHM_NAME, O_RDWR, 0);
	TEST_ASSERT_TRUE(fd2 >= 0);

	/* 清理 */
	osal_close(fd2);
	osal_shm_unlink(TEST_SHM_NAME);
}

/*===========================================================================
 * 共享内存映射测试
 *===========================================================================*/

static void _test_shm_mmap_munmap_success(void)
{
	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);
	int32_t ret = ftruncate(fd, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 映射到内存 */
	void *addr = osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ | PROT_WRITE,
						   MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, addr);

	/* 解除映射 */
	ret = osal_munmap(addr, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 清理 */
	osal_close(fd);
	osal_shm_unlink(TEST_SHM_NAME);
}

static void _test_shm_write_read_data(void)
{
	const char *test_data = "Hello, Shared Memory!";
	char read_buffer[64];

	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);
	int32_t ret = ftruncate(fd, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 映射到内存 */
	void *addr = osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ | PROT_WRITE,
						   MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, addr);

	/* 写入数据 */
	osal_memcpy(addr, test_data, osal_strlen(test_data) + 1);

	/* 读取数据 */
	osal_memcpy(read_buffer, addr, sizeof(read_buffer));
	TEST_ASSERT_EQUAL(0, osal_strcmp(read_buffer, test_data));

	/* 清理 */
	osal_munmap(addr, TEST_SHM_SIZE);
	osal_close(fd);
	osal_shm_unlink(TEST_SHM_NAME);
}

static void _test_shm_multiple_mappings(void)
{
	const int32_t test_value = 12345;

	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);
	int32_t ret = ftruncate(fd, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 第一次映射 */
	int32_t *addr1 = osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ | PROT_WRITE,
							   MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, addr1);

	/* 第二次映射 */
	int32_t *addr2 = osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ | PROT_WRITE,
							   MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, addr2);

	/* 通过第一个映射写入 */
	*addr1 = test_value;

	/* 通过第二个映射读取 */
	TEST_ASSERT_EQUAL(test_value, *addr2);

	/* 清理 */
	osal_munmap(addr1, TEST_SHM_SIZE);
	osal_munmap(addr2, TEST_SHM_SIZE);
	osal_close(fd);
	osal_shm_unlink(TEST_SHM_NAME);
}

/*===========================================================================
 * 内存保护测试
 *===========================================================================*/

static void _test_shm_readonly_mapping(void)
{
	const int32_t test_value = 99;

	/* 创建共享内存 */
	osal_shm_unlink(TEST_SHM_NAME);
	int32_t fd = osal_shm_open(TEST_SHM_NAME, O_CREAT | O_RDWR, 0666);
	TEST_ASSERT_TRUE(fd >= 0);
	int32_t ret = ftruncate(fd, TEST_SHM_SIZE);
	TEST_ASSERT_EQUAL(0, ret);

	/* 先用读写映射写入数据 */
	int32_t *write_addr = osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ | PROT_WRITE,
									MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, write_addr);
	*write_addr = test_value;
	osal_munmap(write_addr, TEST_SHM_SIZE);

	/* 用只读映射读取数据 */
	int32_t *read_addr =
		osal_mmap(NULL, TEST_SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, read_addr);
	TEST_ASSERT_EQUAL(test_value, *read_addr);

	/* 清理 */
	osal_munmap(read_addr, TEST_SHM_SIZE);
	osal_close(fd);
	osal_shm_unlink(TEST_SHM_NAME);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

static const test_case_t test_cases[] = {
	{ .name = "test_shm_open_close_success",
	  .func = _test_shm_open_close_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_unlink_success",
	  .func = _test_shm_unlink_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_open_existing",
	  .func = _test_shm_open_existing,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_mmap_munmap_success",
	  .func = _test_shm_mmap_munmap_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_write_read_data",
	  .func = _test_shm_write_read_data,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_multiple_mappings",
	  .func = _test_shm_multiple_mappings,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shm_readonly_mapping",
	  .func = _test_shm_readonly_mapping,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_shm",
	.module_name = "osal_shm",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 500,
				  .description = "OSAL shared memory tests" }
};

void register_osal_shm_tests(void)
{
	libutest_register_suite(&test_suite);
}
