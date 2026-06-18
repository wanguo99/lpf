/************************************************************************
 * OSAL测试 - Poll和Select I/O多路复用测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * Poll 测试
 *===========================================================================*/

static void _test_poll_immediate_return(void)
{
	osal_pollfd_t fds[1];
	int32_t ret;

	/* 创建一个管道 */
	int pipe_fds[2];
	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 写入数据，使其可读 */
	write(pipe_fds[1], "test", 4);

	/* 设置poll监听读事件 */
	fds[0].fd = pipe_fds[0];
	fds[0].events = OSAL_POLLIN;
	fds[0].revents = 0;

	/* 立即返回（有数据可读） */
	ret = osal_poll(fds, 1, 0);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT(fds[0].revents & OSAL_POLLIN);

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_poll_timeout(void)
{
	osal_pollfd_t fds[1];
	int32_t ret;

	/* 创建一个管道 */
	int pipe_fds[2];
	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 监听读事件（但没有数据） */
	fds[0].fd = pipe_fds[0];
	fds[0].events = OSAL_POLLIN;
	fds[0].revents = 0;

	/* 测试超时 */
	uint64_t start_time = osal_get_tick_count();
	ret = osal_poll(fds, 1, 100);
	uint64_t elapsed = osal_get_tick_count() - start_time;

	TEST_ASSERT_EQUAL(0, ret); /* 超时返回0 */
	TEST_ASSERT(elapsed >= 90); /* 至少接近100ms */
	TEST_ASSERT(elapsed < 200); /* 不应该等太久 */

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_poll_multiple_fds(void)
{
	osal_pollfd_t fds[2];
	int pipe_fds1[2], pipe_fds2[2];
	int32_t ret;

	/* 创建两个管道 */
	ret = pipe(pipe_fds1);
	TEST_ASSERT_EQUAL(0, ret);
	ret = pipe(pipe_fds2);
	TEST_ASSERT_EQUAL(0, ret);

	/* 只向第一个管道写入数据 */
	write(pipe_fds1[1], "data1", 5);

	/* 监听两个管道的读事件 */
	fds[0].fd = pipe_fds1[0];
	fds[0].events = OSAL_POLLIN;
	fds[0].revents = 0;

	fds[1].fd = pipe_fds2[0];
	fds[1].events = OSAL_POLLIN;
	fds[1].revents = 0;

	/* poll应该返回1（只有一个fd就绪） */
	ret = osal_poll(fds, 2, 0);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT(fds[0].revents & OSAL_POLLIN);
	TEST_ASSERT_EQUAL(0, fds[1].revents);

	close(pipe_fds1[0]);
	close(pipe_fds1[1]);
	close(pipe_fds2[0]);
	close(pipe_fds2[1]);
}

static void _test_poll_write_ready(void)
{
	osal_pollfd_t fds[1];
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 监听写事件（管道通常立即可写） */
	fds[0].fd = pipe_fds[1];
	fds[0].events = OSAL_POLLOUT;
	fds[0].revents = 0;

	ret = osal_poll(fds, 1, 0);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT(fds[0].revents & OSAL_POLLOUT);

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_poll_invalid_fd(void)
{
	osal_pollfd_t fds[1];
	int32_t ret;

	/* 使用无效的文件描述符 */
	fds[0].fd = -1;
	fds[0].events = OSAL_POLLIN;
	fds[0].revents = 0;

	ret = osal_poll(fds, 1, 0);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT(fds[0].revents & OSAL_POLLNVAL);
}

static void _test_poll_null_pointer(void)
{
	int32_t ret;

	/* 测试NULL指针 */
	ret = osal_poll(NULL, 0, 0);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_poll_hangup(void)
{
	osal_pollfd_t fds[1];
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 关闭写端，会导致读端检测到POLLHUP */
	close(pipe_fds[1]);

	fds[0].fd = pipe_fds[0];
	fds[0].events = OSAL_POLLIN;
	fds[0].revents = 0;

	ret = osal_poll(fds, 1, 0);
	TEST_ASSERT(ret >= 0);
	TEST_ASSERT(fds[0].revents & (OSAL_POLLHUP | OSAL_POLLIN));

	close(pipe_fds[0]);
}

/*===========================================================================
 * Select 测试
 *===========================================================================*/

static void _test_fd_set_operations(void)
{
	osal_fd_set_t set;
	int32_t ret;

	/* 清空集合 */
	osal_fd_zero(&set);

	/* 测试文件描述符3不在集合中 */
	ret = osal_fd_isset(3, &set);
	TEST_ASSERT_EQUAL(0, ret);

	/* 添加文件描述符3 */
	osal_fd_set(3, &set);
	ret = osal_fd_isset(3, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* 添加文件描述符5 */
	osal_fd_set(5, &set);
	ret = osal_fd_isset(5, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* 3应该仍然在集合中 */
	ret = osal_fd_isset(3, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* 移除文件描述符3 */
	osal_fd_clr(3, &set);
	ret = osal_fd_isset(3, &set);
	TEST_ASSERT_EQUAL(0, ret);

	/* 5应该仍然在集合中 */
	ret = osal_fd_isset(5, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}

static void _test_select_immediate_return(void)
{
	osal_fd_set_t readfds;
	osal_timeval_t timeout;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 写入数据 */
	write(pipe_fds[1], "test", 4);

	/* 设置文件描述符集合 */
	osal_fd_zero(&readfds);
	osal_fd_set(pipe_fds[0], &readfds);

	/* 立即返回 */
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	ret = osal_select(pipe_fds[0] + 1, &readfds, NULL, NULL, &timeout);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds[0], &readfds));

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_select_timeout(void)
{
	osal_fd_set_t readfds;
	osal_timeval_t timeout;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 不写入数据 */
	osal_fd_zero(&readfds);
	osal_fd_set(pipe_fds[0], &readfds);

	/* 设置100ms超时 */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	uint64_t start_time = osal_get_tick_count();
	ret = osal_select(pipe_fds[0] + 1, &readfds, NULL, NULL, &timeout);
	uint64_t elapsed = osal_get_tick_count() - start_time;

	TEST_ASSERT_EQUAL(0, ret); /* 超时返回0 */
	TEST_ASSERT(elapsed >= 90);
	TEST_ASSERT(elapsed < 200);

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_select_multiple_fds(void)
{
	osal_fd_set_t readfds;
	osal_timeval_t timeout;
	int pipe_fds1[2], pipe_fds2[2];
	int32_t ret;

	ret = pipe(pipe_fds1);
	TEST_ASSERT_EQUAL(0, ret);
	ret = pipe(pipe_fds2);
	TEST_ASSERT_EQUAL(0, ret);

	/* 向两个管道都写入数据 */
	write(pipe_fds1[1], "data1", 5);
	write(pipe_fds2[1], "data2", 5);

	/* 设置文件描述符集合 */
	osal_fd_zero(&readfds);
	osal_fd_set(pipe_fds1[0], &readfds);
	osal_fd_set(pipe_fds2[0], &readfds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int max_fd = (pipe_fds1[0] > pipe_fds2[0]) ? pipe_fds1[0] : pipe_fds2[0];
	ret = osal_select(max_fd + 1, &readfds, NULL, NULL, &timeout);

	TEST_ASSERT_EQUAL(2, ret);
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds1[0], &readfds));
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds2[0], &readfds));

	close(pipe_fds1[0]);
	close(pipe_fds1[1]);
	close(pipe_fds2[0]);
	close(pipe_fds2[1]);
}

static void _test_select_write_ready(void)
{
	osal_fd_set_t writefds;
	osal_timeval_t timeout;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	/* 监听写事件 */
	osal_fd_zero(&writefds);
	osal_fd_set(pipe_fds[1], &writefds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	ret = osal_select(pipe_fds[1] + 1, NULL, &writefds, NULL, &timeout);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds[1], &writefds));

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_select_read_and_write(void)
{
	osal_fd_set_t readfds, writefds;
	osal_timeval_t timeout;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	write(pipe_fds[1], "test", 4);

	/* 同时监听读和写 */
	osal_fd_zero(&readfds);
	osal_fd_zero(&writefds);
	osal_fd_set(pipe_fds[0], &readfds);
	osal_fd_set(pipe_fds[1], &writefds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int max_fd = (pipe_fds[0] > pipe_fds[1]) ? pipe_fds[0] : pipe_fds[1];
	ret = osal_select(max_fd + 1, &readfds, &writefds, NULL, &timeout);

	TEST_ASSERT_EQUAL(2, ret);
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds[0], &readfds));
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds[1], &writefds));

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_select_null_sets(void)
{
	osal_timeval_t timeout;
	int32_t ret;

	/* 所有fd_set都是NULL，只用于延时 */
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000; /* 10ms */

	uint64_t start_time = osal_get_tick_count();
	ret = osal_select(0, NULL, NULL, NULL, &timeout);
	uint64_t elapsed = osal_get_tick_count() - start_time;

	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT(elapsed >= 5);
	TEST_ASSERT(elapsed < 50);
}

static void _test_select_null_timeout(void)
{
	osal_fd_set_t readfds;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	write(pipe_fds[1], "test", 4);

	osal_fd_zero(&readfds);
	osal_fd_set(pipe_fds[0], &readfds);

	/* NULL timeout表示无限等待，但因为有数据所以会立即返回 */
	ret = osal_select(pipe_fds[0] + 1, &readfds, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(1, ret);

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

static void _test_pselect_basic(void)
{
	osal_fd_set_t readfds;
	osal_timespec_t timeout;
	int pipe_fds[2];
	int32_t ret;

	ret = pipe(pipe_fds);
	TEST_ASSERT_EQUAL(0, ret);

	write(pipe_fds[1], "test", 4);

	osal_fd_zero(&readfds);
	osal_fd_set(pipe_fds[0], &readfds);

	/* pselect使用纳秒精度 */
	timeout.tv_sec = 0;
	timeout.tv_nsec = 100000000; /* 100ms */

	ret = osal_pselect(pipe_fds[0] + 1, &readfds, NULL, NULL, &timeout, NULL);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_NOT_EQUAL(0, osal_fd_isset(pipe_fds[0], &readfds));

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

/*===========================================================================
 * 边界测试
 *===========================================================================*/

static void _test_fd_set_large_fd(void)
{
	osal_fd_set_t set;
	int32_t ret;

	osal_fd_zero(&set);

	/* 测试较大的文件描述符 */
	int fd = 100;
	osal_fd_set(fd, &set);
	ret = osal_fd_isset(fd, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	osal_fd_clr(fd, &set);
	ret = osal_fd_isset(fd, &set);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_fd_set_boundary(void)
{
	osal_fd_set_t set;
	int32_t ret;

	osal_fd_zero(&set);

	/* 测试边界值 */
	osal_fd_set(0, &set);
	ret = osal_fd_isset(0, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	osal_fd_set(OSAL_FD_SETSIZE - 1, &set);
	ret = osal_fd_isset(OSAL_FD_SETSIZE - 1, &set);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_poll_select(void)
{
	/* Poll 测试 */
	_test_poll_immediate_return();
	_test_poll_timeout();
	_test_poll_multiple_fds();
	_test_poll_write_ready();
	_test_poll_invalid_fd();
	_test_poll_null_pointer();
	_test_poll_hangup();

	/* Select 测试 */
	_test_fd_set_operations();
	_test_select_immediate_return();
	_test_select_timeout();
	_test_select_multiple_fds();
	_test_select_write_ready();
	_test_select_read_and_write();
	_test_select_null_sets();
	_test_select_null_timeout();
	_test_pselect_basic();

	/* 边界测试 */
	_test_fd_set_large_fd();
	_test_fd_set_boundary();
}

static const test_case_t test_cases[] = {
	{ .name = "test_osal_poll_select",
	  .func = test_osal_poll_select,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_poll_select",
	.module_name = "osal_poll_select",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 3000,
				  .description = "OSAL poll/select tests" }
};

void register_osal_poll_select_tests(void)
{
	libutest_register_suite(&test_suite);
}
