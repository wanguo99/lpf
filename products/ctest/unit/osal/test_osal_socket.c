/************************************************************************
 * OSAL测试 - Socket操作测试
 ************************************************************************/

#include <test_framework/test_framework.h>
#include "osal.h"

/*===========================================================================
 * 基础功能测试
 *===========================================================================*/

static void _test_socket_create_close(void)
{
	/* 测试创建TCP socket */
	int32_t sockfd =
		osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 关闭socket */
	int32_t ret = osal_close(sockfd);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_socket_create_udp(void)
{
	/* 测试创建UDP socket */
	int32_t sockfd =
		osal_socket(OSAL_AF_INET, OSAL_SOCK_DGRAM, OSAL_IPPROTO_UDP);
	TEST_ASSERT(sockfd >= 0);

	osal_close(sockfd);
}

static void _test_socket_create_invalid_domain(void)
{
	/* 测试无效的协议族 */
	int32_t sockfd = osal_socket(999, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT_EQUAL(-1, sockfd);
}

static void _test_socket_bind(void)
{
	int32_t sockfd;
	osal_sockaddr_in_t addr;
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 设置地址 */
	osal_memset(&addr, 0, sizeof(addr));
	addr.sin_family = OSAL_AF_INET;
	addr.sin_addr = osal_htonl(0x7F000001); /* 127.0.0.1 */
	addr.sin_port = osal_htons(0); /* 任意端口 */

	/* 绑定地址 */
	ret = osal_bind(sockfd, (osal_sockaddr_t *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL(0, ret);

	osal_close(sockfd);
}

static void _test_socket_bind_null_addr(void)
{
	int32_t sockfd;
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 测试NULL地址 */
	ret = osal_bind(sockfd, NULL, 0);
	TEST_ASSERT_EQUAL(-1, ret);

	osal_close(sockfd);
}

static void _test_socket_listen(void)
{
	int32_t sockfd;
	osal_sockaddr_in_t addr;
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 绑定地址 */
	osal_memset(&addr, 0, sizeof(addr));
	addr.sin_family = OSAL_AF_INET;
	addr.sin_addr = osal_htonl(0x7F000001);
	addr.sin_port = osal_htons(0);
	ret = osal_bind(sockfd, (osal_sockaddr_t *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL(0, ret);

	/* 监听 */
	ret = osal_listen(sockfd, 5);
	TEST_ASSERT_EQUAL(0, ret);

	osal_close(sockfd);
}

static void _test_socket_setsockopt_reuseaddr(void)
{
	int32_t sockfd;
	int32_t optval = 1;
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 设置SO_REUSEADDR */
	ret = osal_setsockopt(sockfd, OSAL_SOL_SOCKET, OSAL_SO_REUSEADDR, &optval,
						  sizeof(optval));
	TEST_ASSERT_EQUAL(0, ret);

	osal_close(sockfd);
}

static void _test_socket_getsockopt(void)
{
	int32_t sockfd;
	int32_t optval = 0;
	osal_size_t optlen = sizeof(optval);
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 获取SO_ERROR */
	ret = osal_getsockopt(sockfd, OSAL_SOL_SOCKET, OSAL_SO_ERROR, &optval,
						  &optlen);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(sizeof(optval), optlen);

	osal_close(sockfd);
}

static void _test_socket_shutdown(void)
{
	int32_t sockfd;
	int32_t ret;

	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(sockfd >= 0);

	/* 关闭读写 */
	ret = osal_shutdown(sockfd, OSAL_SHUT_RDWR);
	TEST_ASSERT_EQUAL(0, ret);

	osal_close(sockfd);
}

/*===========================================================================
 * 字节序转换测试
 *===========================================================================*/

static void _test_htons(void)
{
	uint16_t host_val = 0x1234;
	uint16_t net_val = osal_htons(host_val);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	TEST_ASSERT_EQUAL(0x3412, net_val);
#else
	TEST_ASSERT_EQUAL(0x1234, net_val);
#endif
}

static void _test_htonl(void)
{
	uint32_t host_val = 0x12345678;
	uint32_t net_val = osal_htonl(host_val);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	TEST_ASSERT_EQUAL(0x78563412, net_val);
#else
	TEST_ASSERT_EQUAL(0x12345678, net_val);
#endif
}

static void _test_ntohs(void)
{
	uint16_t net_val = 0x1234;
	uint16_t host_val = osal_ntohs(net_val);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	TEST_ASSERT_EQUAL(0x3412, host_val);
#else
	TEST_ASSERT_EQUAL(0x1234, host_val);
#endif
}

static void _test_ntohl(void)
{
	uint32_t net_val = 0x12345678;
	uint32_t host_val = osal_ntohl(net_val);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	TEST_ASSERT_EQUAL(0x78563412, host_val);
#else
	TEST_ASSERT_EQUAL(0x12345678, host_val);
#endif
}

static void _test_byteorder_round_trip(void)
{
	uint16_t val16 = 0xABCD;
	uint32_t val32 = 0xDEADBEEF;

	/* 测试往返转换 */
	TEST_ASSERT_EQUAL(val16, osal_ntohs(osal_htons(val16)));
	TEST_ASSERT_EQUAL(val32, osal_ntohl(osal_htonl(val32)));
}

/*===========================================================================
 * IP地址转换测试
 *===========================================================================*/

static void _test_inet_pton_ipv4(void)
{
	uint32_t addr;
	int32_t ret;

	/* 测试有效的IPv4地址 */
	ret = osal_inet_pton(OSAL_AF_INET, "192.168.1.1", &addr);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL(osal_htonl(0xC0A80101), addr);

	/* 测试127.0.0.1 */
	ret = osal_inet_pton(OSAL_AF_INET, "127.0.0.1", &addr);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL(osal_htonl(0x7F000001), addr);

	/* 测试0.0.0.0 */
	ret = osal_inet_pton(OSAL_AF_INET, "0.0.0.0", &addr);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL(0, addr);
}

static void _test_inet_pton_ipv4_invalid(void)
{
	uint32_t addr;
	int32_t ret;

	/* 测试无效的IPv4地址 */
	ret = osal_inet_pton(OSAL_AF_INET, "256.1.1.1", &addr);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_inet_pton(OSAL_AF_INET, "1.1.1", &addr);
	TEST_ASSERT_EQUAL(0, ret);

	ret = osal_inet_pton(OSAL_AF_INET, "abc.def.ghi.jkl", &addr);
	TEST_ASSERT_EQUAL(0, ret);
}

static void _test_inet_ntop_ipv4(void)
{
	uint32_t addr;
	char str[32];
	const char *ret;

	/* 测试192.168.1.1 */
	addr = osal_htonl(0xC0A80101);
	ret = osal_inet_ntop(OSAL_AF_INET, &addr, str, sizeof(str));
	TEST_ASSERT_NOT_NULL(ret);
	TEST_ASSERT_EQUAL_STRING("192.168.1.1", str);

	/* 测试127.0.0.1 */
	addr = osal_htonl(0x7F000001);
	ret = osal_inet_ntop(OSAL_AF_INET, &addr, str, sizeof(str));
	TEST_ASSERT_NOT_NULL(ret);
	TEST_ASSERT_EQUAL_STRING("127.0.0.1", str);
}

static void _test_inet_round_trip(void)
{
	uint32_t addr1, addr2;
	char str[32];
	int32_t ret;

	/* 测试往返转换 */
	ret = osal_inet_pton(OSAL_AF_INET, "10.20.30.40", &addr1);
	TEST_ASSERT_EQUAL(1, ret);

	osal_inet_ntop(OSAL_AF_INET, &addr1, str, sizeof(str));

	ret = osal_inet_pton(OSAL_AF_INET, str, &addr2);
	TEST_ASSERT_EQUAL(1, ret);
	TEST_ASSERT_EQUAL(addr1, addr2);
}

/*===========================================================================
 * TCP客户端/服务器测试
 *===========================================================================*/

typedef struct {
	uint16_t port;
	int32_t server_fd;
} tcp_server_ctx_t;

static void *_tcp_server_thread(void *arg)
{
	tcp_server_ctx_t *ctx = (tcp_server_ctx_t *)arg;
	osal_sockaddr_in_t addr;
	int32_t ret;
	int32_t client_fd;
	char buffer[64];

	/* 创建监听socket */
	ctx->server_fd =
		osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	if (ctx->server_fd < 0) {
		return NULL;
	}

	/* 设置SO_REUSEADDR */
	int32_t optval = 1;
	osal_setsockopt(ctx->server_fd, OSAL_SOL_SOCKET, OSAL_SO_REUSEADDR, &optval,
					sizeof(optval));

	/* 绑定地址 */
	osal_memset(&addr, 0, sizeof(addr));
	addr.sin_family = OSAL_AF_INET;
	addr.sin_addr = osal_htonl(0x7F000001);
	addr.sin_port = osal_htons(ctx->port);
	ret = osal_bind(ctx->server_fd, (osal_sockaddr_t *)&addr, sizeof(addr));
	if (ret < 0) {
		osal_close(ctx->server_fd);
		return NULL;
	}

	/* 监听 */
	ret = osal_listen(ctx->server_fd, 1);
	if (ret < 0) {
		osal_close(ctx->server_fd);
		return NULL;
	}

	/* 接受连接 */
	client_fd = osal_accept(ctx->server_fd, NULL, NULL);
	if (client_fd < 0) {
		osal_close(ctx->server_fd);
		return NULL;
	}

	/* 接收数据 */
	osal_ssize_t n = osal_recv(client_fd, buffer, sizeof(buffer), 0);
	if (n > 0) {
		/* 回显数据 */
		osal_send(client_fd, buffer, n, 0);
	}

	osal_close(client_fd);
	osal_close(ctx->server_fd);
	return NULL;
}

static void _test_socket_tcp_client_server(void)
{
	tcp_server_ctx_t ctx;
	osal_thread_t server_thread;
	int32_t client_fd;
	osal_sockaddr_in_t addr;
	char send_buf[] = "Hello, Server!";
	char recv_buf[64];
	int32_t ret;

	/* 选择随机端口 */
	ctx.port = 19999;

	/* 启动服务器线程 */
	ret = osal_thread_create(&server_thread, NULL, _tcp_server_thread, &ctx);
	TEST_ASSERT_EQUAL(0, ret);

	/* 等待服务器启动 */
	osal_msleep(100);

	/* 创建客户端socket */
	client_fd = osal_socket(OSAL_AF_INET, OSAL_SOCK_STREAM, OSAL_IPPROTO_TCP);
	TEST_ASSERT(client_fd >= 0);

	/* 连接到服务器 */
	osal_memset(&addr, 0, sizeof(addr));
	addr.sin_family = OSAL_AF_INET;
	addr.sin_addr = osal_htonl(0x7F000001);
	addr.sin_port = osal_htons(ctx.port);
	ret = osal_connect(client_fd, (osal_sockaddr_t *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送数据 */
	osal_ssize_t n = osal_send(client_fd, send_buf, osal_strlen(send_buf), 0);
	TEST_ASSERT_EQUAL((osal_ssize_t)osal_strlen(send_buf), n);

	/* 接收回显数据 */
	n = osal_recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);
	TEST_ASSERT(n > 0);
	recv_buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING(send_buf, recv_buf);

	osal_close(client_fd);
	osal_thread_join(server_thread, NULL);
}

/*===========================================================================
 * UDP测试
 *===========================================================================*/

static void _test_socket_udp_sendto_recvfrom(void)
{
	int32_t sockfd;
	osal_sockaddr_in_t addr;
	char send_buf[] = "UDP Test";
	char recv_buf[64];
	int32_t ret;

	/* 创建UDP socket */
	sockfd = osal_socket(OSAL_AF_INET, OSAL_SOCK_DGRAM, OSAL_IPPROTO_UDP);
	TEST_ASSERT(sockfd >= 0);

	/* 绑定地址 */
	osal_memset(&addr, 0, sizeof(addr));
	addr.sin_family = OSAL_AF_INET;
	addr.sin_addr = osal_htonl(0x7F000001);
	addr.sin_port = osal_htons(20000);
	ret = osal_bind(sockfd, (osal_sockaddr_t *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL(0, ret);

	/* 发送数据到自己 */
	osal_ssize_t n = osal_sendto(sockfd, send_buf, osal_strlen(send_buf), 0,
								 (osal_sockaddr_t *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL((osal_ssize_t)osal_strlen(send_buf), n);

	/* 接收数据 */
	osal_sockaddr_in_t from_addr;
	osal_size_t from_len = sizeof(from_addr);
	n = osal_recvfrom(sockfd, recv_buf, sizeof(recv_buf) - 1, 0,
					  (osal_sockaddr_t *)&from_addr, &from_len);
	TEST_ASSERT(n > 0);
	recv_buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING(send_buf, recv_buf);

	osal_close(sockfd);
}

/*===========================================================================
 * 网络接口测试
 *===========================================================================*/

static void _test_if_nametoindex(void)
{
	/* 测试loopback接口 */
	uint32_t index = osal_if_nametoindex("lo");
	TEST_ASSERT(index > 0);

	/* 测试不存在的接口 */
	index = osal_if_nametoindex("nonexistent999");
	TEST_ASSERT_EQUAL(0, index);
}

static void _test_if_indextoname(void)
{
	char ifname[16];
	char *ret;

	/* 测试loopback接口索引为1 */
	ret = osal_if_indextoname(1, ifname);
	if (ret != NULL) {
		TEST_ASSERT_EQUAL_STRING("lo", ifname);
	}

	/* 测试不存在的索引 */
	ret = osal_if_indextoname(999999, ifname);
	TEST_ASSERT_NULL(ret);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

void test_osal_socket(void)
{
	/* 基础功能测试 */
	_test_socket_create_close();
	_test_socket_create_udp();
	_test_socket_create_invalid_domain();
	_test_socket_bind();
	_test_socket_bind_null_addr();
	_test_socket_listen();
	_test_socket_setsockopt_reuseaddr();
	_test_socket_getsockopt();
	_test_socket_shutdown();

	/* 字节序转换测试 */
	_test_htons();
	_test_htonl();
	_test_ntohs();
	_test_ntohl();
	_test_byteorder_round_trip();

	/* IP地址转换测试 */
	_test_inet_pton_ipv4();
	_test_inet_pton_ipv4_invalid();
	_test_inet_ntop_ipv4();
	_test_inet_round_trip();

	/* TCP测试 */
	_test_socket_tcp_client_server();

	/* UDP测试 */
	_test_socket_udp_sendto_recvfrom();

	/* 网络接口测试 */
	_test_if_nametoindex();
	_test_if_indextoname();
}

static const test_case_t test_cases[] = {
	{ .name = "test_osal_socket",
	  .func = test_osal_socket,
	  .setup = NULL,
	  .teardown = NULL },
};

static const test_suite_t test_suite = {
	.suite_name = "osal_socket",
	.module_name = "osal_socket",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = sizeof(test_cases) / sizeof(test_cases[0]),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_NETWORK,
				  .timeout_ms = 5000,
				  .description = "OSAL socket tests" }
};

void register_osal_socket_tests(void)
{
	libutest_register_suite(&test_suite);
}
