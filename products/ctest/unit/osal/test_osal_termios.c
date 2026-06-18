/************************************************************************
 * OSAL测试 - Termios终端控制测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"
#include <pty.h>
#include <unistd.h>

/*===========================================================================
 * 辅助函数
 *===========================================================================*/

/* 创建伪终端对用于测试 */
static int32_t _create_pty(int32_t *master_fd, int32_t *slave_fd)
{
	char name[256];

	if (openpty(master_fd, slave_fd, name, NULL, NULL) < 0) {
		return -1;
	}
	return 0;
}

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

static void _test_tcgetattr_tcsetattr(void)
{
	int32_t master_fd, slave_fd;
	osal_termios_t term;
	int32_t ret;

	/* 创建伪终端 */
	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 获取终端属性 */
	ret = osal_tcgetattr(slave_fd, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 修改属性：设置为原始模式 */
	term.c_lflag &= ~(OSAL_ICANON | OSAL_ECHO | OSAL_ISIG);
	term.c_iflag &= ~(OSAL_ICRNL | OSAL_INLCR);
	term.c_oflag &= ~OSAL_OPOST;

	/* 设置终端属性 */
	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证设置成功 */
	osal_termios_t term2;
	ret = osal_tcgetattr(slave_fd, &term2);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(term.c_lflag, term2.c_lflag);

	close(master_fd);
	close(slave_fd);
}

static void _test_tcgetattr_invalid_fd(void)
{
	osal_termios_t term;
	int32_t ret;

	/* 测试无效的文件描述符 */
	ret = osal_tcgetattr(-1, &term);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = osal_tcgetattr(9999, &term);
	TEST_ASSERT_EQUAL(-1, ret);
}

static void _test_tcgetattr_null_pointer(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 测试NULL指针 */
	ret = osal_tcgetattr(slave_fd, NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	close(master_fd);
	close(slave_fd);
}

static void _test_tcsetattr_null_pointer(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 测试NULL指针 */
	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, NULL);
	TEST_ASSERT_EQUAL(-1, ret);

	close(master_fd);
	close(slave_fd);
}

/*===========================================================================
 * 波特率测试
 *===========================================================================*/

static void _test_cfsetispeed_cfgetispeed(void)
{
	osal_termios_t term;
	int32_t ret;
	uint32_t speed;

	osal_memset(&term, 0, sizeof(term));

	/* 设置9600波特率 */
	ret = osal_cfsetispeed(&term, OSAL_B9600);
	TEST_ASSERT_EQUAL(0, ret);

	/* 获取波特率 */
	speed = osal_cfgetispeed(&term);
	TEST_ASSERT_EQUAL(OSAL_B9600, speed);

	/* 设置115200波特率 */
	ret = osal_cfsetispeed(&term, OSAL_B115200);
	TEST_ASSERT_EQUAL(0, ret);

	speed = osal_cfgetispeed(&term);
	TEST_ASSERT_EQUAL(OSAL_B115200, speed);
}

static void _test_cfsetospeed_cfgetospeed(void)
{
	osal_termios_t term;
	int32_t ret;
	uint32_t speed;

	osal_memset(&term, 0, sizeof(term));

	/* 设置9600波特率 */
	ret = osal_cfsetospeed(&term, OSAL_B9600);
	TEST_ASSERT_EQUAL(0, ret);

	/* 获取波特率 */
	speed = osal_cfgetospeed(&term);
	TEST_ASSERT_EQUAL(OSAL_B9600, speed);

	/* 设置115200波特率 */
	ret = osal_cfsetospeed(&term, OSAL_B115200);
	TEST_ASSERT_EQUAL(0, ret);

	speed = osal_cfgetospeed(&term);
	TEST_ASSERT_EQUAL(OSAL_B115200, speed);
}

static void _test_baudrate_common_values(void)
{
	osal_termios_t term;
	int32_t ret;
	uint32_t speed;

	osal_memset(&term, 0, sizeof(term));

	/* 测试常用波特率 */
	uint32_t baud_rates[] = { OSAL_B9600,	OSAL_B19200,  OSAL_B38400,
							  OSAL_B57600,	OSAL_B115200, OSAL_B230400,
							  OSAL_B460800, OSAL_B921600 };

	for (size_t i = 0; i < sizeof(baud_rates) / sizeof(baud_rates[0]); i++) {
		ret = osal_cfsetispeed(&term, baud_rates[i]);
		TEST_ASSERT_EQUAL(0, ret);
		speed = osal_cfgetispeed(&term);
		TEST_ASSERT_EQUAL(baud_rates[i], speed);

		ret = osal_cfsetospeed(&term, baud_rates[i]);
		TEST_ASSERT_EQUAL(0, ret);
		speed = osal_cfgetospeed(&term);
		TEST_ASSERT_EQUAL(baud_rates[i], speed);
	}
}

static void _test_cfsetispeed_null_pointer(void)
{
	int32_t ret;

	/* 测试NULL指针 */
	ret = osal_cfsetispeed(NULL, OSAL_B9600);
	TEST_ASSERT_EQUAL(-1, ret);
}

static void _test_cfsetospeed_null_pointer(void)
{
	int32_t ret;

	/* 测试NULL指针 */
	ret = osal_cfsetospeed(NULL, OSAL_B9600);
	TEST_ASSERT_EQUAL(-1, ret);
}

/*===========================================================================
 * 控制模式测试
 *===========================================================================*/

static void _test_set_raw_mode(void)
{
	int32_t master_fd, slave_fd;
	osal_termios_t term;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 获取当前属性 */
	ret = osal_tcgetattr(slave_fd, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 设置原始模式 */
	term.c_iflag &= ~(OSAL_IGNBRK | OSAL_BRKINT | OSAL_PARMRK | OSAL_ISTRIP |
					  OSAL_INLCR | OSAL_IGNCR | OSAL_ICRNL | OSAL_IXON);
	term.c_oflag &= ~OSAL_OPOST;
	term.c_lflag &=
		~(OSAL_ECHO | OSAL_ECHONL | OSAL_ICANON | OSAL_ISIG | OSAL_IEXTEN);
	term.c_cflag &= ~(OSAL_CSIZE | OSAL_PARENB);
	term.c_cflag |= OSAL_CS8;

	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证设置 */
	osal_termios_t term2;
	ret = osal_tcgetattr(slave_fd, &term2);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, term2.c_lflag & OSAL_ICANON);
	TEST_ASSERT_EQUAL(0, term2.c_lflag & OSAL_ECHO);

	close(master_fd);
	close(slave_fd);
}

static void _test_set_8n1_mode(void)
{
	int32_t master_fd, slave_fd;
	osal_termios_t term;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	ret = osal_tcgetattr(slave_fd, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 设置8N1: 8位数据位，无奇偶校验，1个停止位 */
	term.c_cflag &= ~OSAL_PARENB; /* 无奇偶校验 */
	term.c_cflag &= ~OSAL_CSTOPB; /* 1个停止位 */
	term.c_cflag &= ~OSAL_CSIZE;
	term.c_cflag |= OSAL_CS8; /* 8位数据位 */

	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证设置 */
	osal_termios_t term2;
	ret = osal_tcgetattr(slave_fd, &term2);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, term2.c_cflag & OSAL_PARENB);
	TEST_ASSERT_EQUAL(0, term2.c_cflag & OSAL_CSTOPB);
	TEST_ASSERT_EQUAL(OSAL_CS8, term2.c_cflag & OSAL_CSIZE);

	close(master_fd);
	close(slave_fd);
}

/*===========================================================================
 * 队列控制测试
 *===========================================================================*/

static void _test_tcflush(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 写入一些数据 */
	const char *test_data = "test data";
	write(master_fd, test_data, osal_strlen(test_data));

	/* 刷新输入队列 */
	ret = osal_tcflush(slave_fd, OSAL_TCIFLUSH);
	TEST_ASSERT_EQUAL(0, ret);

	/* 刷新输出队列 */
	ret = osal_tcflush(slave_fd, OSAL_TCOFLUSH);
	TEST_ASSERT_EQUAL(0, ret);

	/* 刷新输入和输出队列 */
	ret = osal_tcflush(slave_fd, OSAL_TCIOFLUSH);
	TEST_ASSERT_EQUAL(0, ret);

	close(master_fd);
	close(slave_fd);
}

static void _test_tcflush_invalid_fd(void)
{
	int32_t ret;

	/* 测试无效的文件描述符 */
	ret = osal_tcflush(-1, OSAL_TCIFLUSH);
	TEST_ASSERT_EQUAL(-1, ret);
}

static void _test_tcdrain(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 写入数据 */
	const char *test_data = "test data for drain";
	write(slave_fd, test_data, osal_strlen(test_data));

	/* 等待输出完成 */
	ret = osal_tcdrain(slave_fd);
	TEST_ASSERT_EQUAL(0, ret);

	close(master_fd);
	close(slave_fd);
}

static void _test_tcsendbreak(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 发送BREAK信号 */
	ret = osal_tcsendbreak(slave_fd, 0);
	TEST_ASSERT_EQUAL(0, ret);

	close(master_fd);
	close(slave_fd);
}

static void _test_tcflow(void)
{
	int32_t master_fd, slave_fd;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 挂起输出 */
	ret = osal_tcflow(slave_fd, OSAL_TCOOFF);
	TEST_ASSERT_EQUAL(0, ret);

	/* 恢复输出 */
	ret = osal_tcflow(slave_fd, OSAL_TCOON);
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送STOP字符 */
	ret = osal_tcflow(slave_fd, OSAL_TCIOFF);
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送START字符 */
	ret = osal_tcflow(slave_fd, OSAL_TCION);
	TEST_ASSERT_EQUAL(0, ret);

	close(master_fd);
	close(slave_fd);
}

/*===========================================================================
 * 串口配置场景测试
 *===========================================================================*/

static void _test_configure_serial_115200_8n1(void)
{
	int32_t master_fd, slave_fd;
	osal_termios_t term;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	/* 获取当前属性 */
	ret = osal_tcgetattr(slave_fd, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 配置为115200 8N1 */
	osal_cfsetispeed(&term, OSAL_B115200);
	osal_cfsetospeed(&term, OSAL_B115200);

	term.c_cflag &= ~OSAL_PARENB; /* 无校验 */
	term.c_cflag &= ~OSAL_CSTOPB; /* 1个停止位 */
	term.c_cflag &= ~OSAL_CSIZE;
	term.c_cflag |= OSAL_CS8; /* 8位数据位 */
	term.c_cflag |= OSAL_CREAD | OSAL_CLOCAL;

	term.c_lflag &= ~(OSAL_ICANON | OSAL_ECHO | OSAL_ECHOE | OSAL_ISIG);
	term.c_iflag &= ~(OSAL_IXON | OSAL_IXOFF | OSAL_IXANY);
	term.c_oflag &= ~OSAL_OPOST;

	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证配置 */
	osal_termios_t term2;
	ret = osal_tcgetattr(slave_fd, &term2);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(OSAL_B115200, osal_cfgetispeed(&term2));
	TEST_ASSERT_EQUAL(OSAL_B115200, osal_cfgetospeed(&term2));

	close(master_fd);
	close(slave_fd);
}

static void _test_configure_serial_9600_7e1(void)
{
	int32_t master_fd, slave_fd;
	osal_termios_t term;
	int32_t ret;

	ret = _create_pty(&master_fd, &slave_fd);
	if (ret < 0) {
		TEST_SKIP("Cannot create pty");
		return;
	}

	ret = osal_tcgetattr(slave_fd, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 配置为9600 7E1 (7位数据位，偶校验，1个停止位) */
	osal_cfsetispeed(&term, OSAL_B9600);
	osal_cfsetospeed(&term, OSAL_B9600);

	term.c_cflag |= OSAL_PARENB; /* 启用校验 */
	term.c_cflag &= ~OSAL_PARODD; /* 偶校验 */
	term.c_cflag &= ~OSAL_CSTOPB; /* 1个停止位 */
	term.c_cflag &= ~OSAL_CSIZE;
	term.c_cflag |= OSAL_CS7; /* 7位数据位 */

	ret = osal_tcsetattr(slave_fd, OSAL_TCSANOW, &term);
	TEST_ASSERT_EQUAL(0, ret);

	/* 验证配置 */
	osal_termios_t term2;
	ret = osal_tcgetattr(slave_fd, &term2);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(OSAL_B9600, osal_cfgetispeed(&term2));
	TEST_ASSERT(term2.c_cflag & OSAL_PARENB);
	TEST_ASSERT_EQUAL(0, term2.c_cflag & OSAL_PARODD);
	TEST_ASSERT_EQUAL(OSAL_CS7, term2.c_cflag & OSAL_CSIZE);

	close(master_fd);
	close(slave_fd);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_termios(void)
{
	/* 基础功能测试 */
	_test_tcgetattr_tcsetattr();
	_test_tcgetattr_invalid_fd();
	_test_tcgetattr_null_pointer();
	_test_tcsetattr_null_pointer();

	/* 波特率测试 */
	_test_cfsetispeed_cfgetispeed();
	_test_cfsetospeed_cfgetospeed();
	_test_baudrate_common_values();
	_test_cfsetispeed_null_pointer();
	_test_cfsetospeed_null_pointer();

	/* 控制模式测试 */
	_test_set_raw_mode();
	_test_set_8n1_mode();

	/* 队列控制测试 */
	_test_tcflush();
	_test_tcflush_invalid_fd();
	_test_tcdrain();
	_test_tcsendbreak();
	_test_tcflow();

	/* 串口配置场景测试 */
	_test_configure_serial_115200_8n1();
	_test_configure_serial_9600_7e1();
}

static const test_case_t test_cases[] = {
	{ .name = "test_osal_termios",
	  .func = test_osal_termios,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_termios",
	.module_name = "osal_termios",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 3000,
				  .description = "OSAL termios tests" }
};

void register_osal_termios_tests(void)
{
	libutest_register_suite(&test_suite);
}
