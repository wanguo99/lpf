/**
 * @file test_system_osal_file_thread.c
 * @brief File I/O + Threading integration tests
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"

/* File read context */
typedef struct {
	const char *filename;
	osal_atomic_uint32_t read_count;
	osal_atomic_uint32_t error_count;
} file_read_ctx_t;

static void* file_reader_func(void *arg) {
	file_read_ctx_t *ctx = (file_read_ctx_t*)arg;
	int fd;
	char buffer[256];
	ssize_t bytes_read;

	fd = OSAL_open(ctx->filename, OSAL_O_RDONLY, 0);
	if (fd < 0) {
		OSAL_atomic_inc(&ctx->error_count);
		return NULL;
	}

	bytes_read = OSAL_read(fd, buffer, sizeof(buffer) - 1);
	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		OSAL_atomic_inc(&ctx->read_count);
	} else {
		OSAL_atomic_inc(&ctx->error_count);
	}

	OSAL_close(fd);
	return NULL;
}

/**
 * Test multiple threads reading same file
 */
static void test_concurrent_file_read(void) {
	OSAL_printf("[ TEST     ] Concurrent file read\n");

	file_read_ctx_t ctx;
	osal_thread_t threads[5];
	uint32_t i;
	int32_t ret;
	int fd;

	/* Create test file */
	const char *test_file = "/tmp/test_concurrent_read.txt";
	fd = OSAL_open(test_file, OSAL_O_CREAT | OSAL_O_WRONLY | OSAL_O_TRUNC, 0644);
	if (fd < 0) {
		OSAL_printf("[ SKIP     ] Cannot create test file\n");
		return;
	}

	const char *test_data = "This is test data for concurrent reading.\n";
	OSAL_write(fd, test_data, OSAL_strlen(test_data));
	OSAL_close(fd);

	/* Initialize context */
	ctx.filename = test_file;
	OSAL_atomic_store(&ctx.read_count, 0);
	OSAL_atomic_store(&ctx.error_count, 0);

	/* Create reader threads */
	for (i = 0; i < 5; i++) {
		ret = OSAL_pthread_create(&threads[i], NULL, file_reader_func, &ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Wait for completion */
	for (i = 0; i < 5; i++) {
		OSAL_pthread_join(threads[i], NULL);
	}

	/* Verify results */
	TEST_ASSERT_EQUAL(5, OSAL_atomic_load(&ctx.read_count));
	TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&ctx.error_count));

	/* Cleanup */
	OSAL_unlink(test_file);

	OSAL_printf("[ PASS     ] Concurrent file read test passed\n");
}

/* File write context */
typedef struct {
	osal_atomic_uint32_t write_count;
	osal_atomic_uint32_t error_count;
} file_write_ctx_t;

static void* file_writer_func(void *arg) {
	file_write_ctx_t *ctx = (file_write_ctx_t*)arg;
	char filename[128];
	int fd;
	char buffer[128];
	ssize_t bytes_written;

	/* Each thread writes to its own file */
	OSAL_snprintf(filename, sizeof(filename), "/tmp/test_write_%lu.txt",
	             (unsigned long)OSAL_pthread_self());

	fd = OSAL_open(filename, OSAL_O_CREAT | OSAL_O_WRONLY | OSAL_O_TRUNC, 0644);
	if (fd < 0) {
		OSAL_atomic_inc(&ctx->error_count);
		return NULL;
	}

	OSAL_snprintf(buffer, sizeof(buffer), "Thread %lu data\n",
	             (unsigned long)OSAL_pthread_self());

	bytes_written = OSAL_write(fd, buffer, OSAL_strlen(buffer));
	if (bytes_written > 0) {
		OSAL_atomic_inc(&ctx->write_count);
	} else {
		OSAL_atomic_inc(&ctx->error_count);
	}

	OSAL_close(fd);

	/* Cleanup our file */
	OSAL_unlink(filename);

	return NULL;
}

/**
 * Test multiple threads writing to separate files
 */
static void test_concurrent_file_write(void) {
	OSAL_printf("[ TEST     ] Concurrent file write\n");

	file_write_ctx_t ctx;
	osal_thread_t threads[5];
	uint32_t i;
	int32_t ret;

	/* Initialize context */
	OSAL_atomic_store(&ctx.write_count, 0);
	OSAL_atomic_store(&ctx.error_count, 0);

	/* Create writer threads */
	for (i = 0; i < 5; i++) {
		ret = OSAL_pthread_create(&threads[i], NULL, file_writer_func, &ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Wait for completion */
	for (i = 0; i < 5; i++) {
		OSAL_pthread_join(threads[i], NULL);
	}

	/* Verify results */
	TEST_ASSERT_EQUAL(5, OSAL_atomic_load(&ctx.write_count));
	TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&ctx.error_count));

	OSAL_printf("[ PASS     ] Concurrent file write test passed\n");
}

/* File lock context */
typedef struct {
	const char *filename;
	osal_mutex_t file_mutex;
	osal_atomic_uint32_t access_count;
	int shared_fd;
} file_lock_ctx_t;

static void* file_lock_worker_func(void *arg) {
	file_lock_ctx_t *ctx = (file_lock_ctx_t*)arg;
	uint32_t i;

	for (i = 0; i < 10; i++) {
		/* Lock before file access */
		OSAL_pthread_mutex_lock(&ctx->file_mutex);

		/* Critical section: file I/O */
		(void)OSAL_lseek(ctx->shared_fd, 0, OSAL_SEEK_END);
		char buffer[64];
		OSAL_snprintf(buffer, sizeof(buffer), "Thread %lu entry %u\n",
		             (unsigned long)OSAL_pthread_self(), i);
		OSAL_write(ctx->shared_fd, buffer, OSAL_strlen(buffer));

		OSAL_atomic_inc(&ctx->access_count);

		/* Unlock */
		OSAL_pthread_mutex_unlock(&ctx->file_mutex);

		OSAL_usleep(100);
	}

	return NULL;
}

/**
 * Test file locking coordination
 */
static void test_file_locking_coordination(void) {
	OSAL_printf("[ TEST     ] File locking coordination\n");

	file_lock_ctx_t ctx;
	osal_thread_t threads[3];
	uint32_t i;
	int32_t ret;

	/* Create shared file */
	const char *test_file = "/tmp/test_file_lock.txt";
	ctx.filename = test_file;
	ctx.shared_fd = OSAL_open(test_file, OSAL_O_CREAT | OSAL_O_WRONLY | OSAL_O_TRUNC, 0644);
	if (ctx.shared_fd < 0) {
		OSAL_printf("[ SKIP     ] Cannot create test file\n");
		return;
	}

	/* Initialize */
	ret = OSAL_pthread_mutex_init(&ctx.file_mutex, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	OSAL_atomic_store(&ctx.access_count, 0);

	/* Create worker threads */
	for (i = 0; i < 3; i++) {
		ret = OSAL_pthread_create(&threads[i], NULL, file_lock_worker_func, &ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Wait for completion */
	for (i = 0; i < 3; i++) {
		OSAL_pthread_join(threads[i], NULL);
	}

	/* Verify all accesses completed */
	TEST_ASSERT_EQUAL(30, OSAL_atomic_load(&ctx.access_count));

	/* Cleanup */
	OSAL_close(ctx.shared_fd);
	OSAL_pthread_mutex_destroy(&ctx.file_mutex);
	OSAL_unlink(test_file);

	OSAL_printf("[ PASS     ] File locking coordination test passed\n");
}

/* File position tracking context */
typedef struct {
	const char *filename;
	osal_atomic_uint32_t position_errors;
} file_position_ctx_t;

static void* file_position_worker_func(void *arg) {
	file_position_ctx_t *ctx = (file_position_ctx_t*)arg;
	int fd;
	char buffer[16];
	off_t pos1, pos2;

	fd = OSAL_open(ctx->filename, OSAL_O_RDONLY, 0);
	if (fd < 0) {
		return NULL;
	}

	/* Read and verify position tracking */
	pos1 = OSAL_lseek(fd, 0, OSAL_SEEK_CUR);
	if (pos1 != 0) {
		OSAL_atomic_inc(&ctx->position_errors);
		OSAL_close(fd);
		return NULL;
	}

	OSAL_read(fd, buffer, 10);
	pos2 = OSAL_lseek(fd, 0, OSAL_SEEK_CUR);

	if (pos2 != 10) {
		OSAL_atomic_inc(&ctx->position_errors);
	}

	/* Seek to different positions */
	OSAL_lseek(fd, 5, OSAL_SEEK_SET);
	pos1 = OSAL_lseek(fd, 0, OSAL_SEEK_CUR);
	if (pos1 != 5) {
		OSAL_atomic_inc(&ctx->position_errors);
	}

	OSAL_close(fd);
	return NULL;
}

/**
 * Test file position tracking per thread
 */
static void test_file_position_tracking(void) {
	OSAL_printf("[ TEST     ] File position tracking per thread\n");

	file_position_ctx_t ctx;
	osal_thread_t threads[4];
	uint32_t i;
	int32_t ret;
	int fd;

	/* Create test file with known content */
	const char *test_file = "/tmp/test_position.txt";
	fd = OSAL_open(test_file, OSAL_O_CREAT | OSAL_O_WRONLY | OSAL_O_TRUNC, 0644);
	if (fd < 0) {
		OSAL_printf("[ SKIP     ] Cannot create test file\n");
		return;
	}

	const char *test_data = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	OSAL_write(fd, test_data, OSAL_strlen(test_data));
	OSAL_close(fd);

	/* Initialize context */
	ctx.filename = test_file;
	OSAL_atomic_store(&ctx.position_errors, 0);

	/* Create worker threads */
	for (i = 0; i < 4; i++) {
		ret = OSAL_pthread_create(&threads[i], NULL, file_position_worker_func, &ctx);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Wait for completion */
	for (i = 0; i < 4; i++) {
		OSAL_pthread_join(threads[i], NULL);
	}

	/* Verify no position errors */
	TEST_ASSERT_EQUAL(0, OSAL_atomic_load(&ctx.position_errors));

	/* Cleanup */
	OSAL_unlink(test_file);

	OSAL_printf("[ PASS     ] File position tracking test passed\n");
}

/**
 * Test proper file descriptor cleanup
 */
static void test_fd_cleanup(void) {
	OSAL_printf("[ TEST     ] File descriptor cleanup\n");

	const char *test_file = "/tmp/test_fd_cleanup.txt";
	int fd;
	uint32_t i;

	/* Test repeated open/close cycles */
	for (i = 0; i < 100; i++) {
		fd = OSAL_open(test_file, OSAL_O_CREAT | OSAL_O_WRONLY | OSAL_O_TRUNC, 0644);
		TEST_ASSERT_TRUE(fd >= 0);

		char buffer[32];
		OSAL_snprintf(buffer, sizeof(buffer), "Iteration %u\n", i);
		OSAL_write(fd, buffer, OSAL_strlen(buffer));

		int32_t ret = OSAL_close(fd);
		TEST_ASSERT_EQUAL(0, ret);
	}

	/* Cleanup */
	OSAL_unlink(test_file);

	OSAL_printf("[ PASS     ] FD cleanup test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
	{
		.name = "test_concurrent_file_read",
		.func = test_concurrent_file_read,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_concurrent_file_write",
		.func = test_concurrent_file_write,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_file_locking_coordination",
		.func = test_file_locking_coordination,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_file_position_tracking",
		.func = test_file_position_tracking,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_fd_cleanup",
		.func = test_fd_cleanup,
		.setup = NULL,
		.teardown = NULL
	},
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_osal_file_thread",
	.module_name = "system_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_SYSTEM,
		.tags = TEST_TAG_SLOW | TEST_TAG_FILESYSTEM,
		.timeout_ms = 15000,
		.description = "File I/O + Threading integration tests"
	}
};

/* Register test suite */
__attribute__((constructor))
static void register_system_osal_file_thread_tests(void)
{
	libutest_register_suite(&test_suite);
}
