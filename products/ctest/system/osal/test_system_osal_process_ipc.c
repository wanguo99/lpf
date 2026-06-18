/**
 * @file test_system_osal_process_ipc.c
 * @brief Process + IPC integration tests
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "osal.h"

/**
 * Test parent-child communication via pipes
 */
static void _test_parent_child_pipe_communication(void)
{
	osal_printf("[ TEST     ] Parent-child pipe communication\n");

	int pipe_fds[2];
	int32_t ret;
	pid_t pid;

	/* Create pipe */
	ret = osal_pipe(pipe_fds);
	if (ret != 0) {
		osal_printf("[ SKIP     ] Pipe not available\n");
		return;
	}

	/* Fork child process */
	pid = osal_fork();
	if (pid < 0) {
		osal_printf("[ SKIP     ] Fork not available\n");
		osal_close(pipe_fds[0]);
		osal_close(pipe_fds[1]);
		return;
	}

	if (pid == 0) {
		/* Child process - reader */
		osal_close(pipe_fds[1]); /* Close write end */

		char buffer[128];
		ssize_t bytes_read = osal_read(pipe_fds[0], buffer, sizeof(buffer) - 1);
		if (bytes_read > 0) {
			buffer[bytes_read] = '\0';
			if (osal_strcmp(buffer, "Hello from parent") == 0) {
				osal_exit(0); /* Success */
			}
		}
		osal_exit(1); /* Failure */
	} else {
		/* Parent process - writer */
		osal_close(pipe_fds[0]); /* Close read end */

		const char *message = "Hello from parent";
		osal_write(pipe_fds[1], message, osal_strlen(message));
		osal_close(pipe_fds[1]);

		/* Wait for child */
		int status;
		osal_waitpid(pid, &status, 0);
		TEST_ASSERT_EQUAL(0, status);
	}

	osal_printf("[ PASS     ] Parent-child pipe communication test passed\n");
}

/**
 * Test shared memory between processes
 */
static void _test_shared_memory_between_processes(void)
{
	osal_printf("[ TEST     ] Shared memory between processes\n");

	char shm_name[64];
	int shm_fd;
	size_t shm_size = 4096;
	void *shm_ptr;
	pid_t pid;

	/* Create shared memory */
	osal_snprintf(shm_name, sizeof(shm_name), "/test_proc_shm_%d",
				  osal_getpid());
	shm_fd = osal_shm_open(shm_name, OSAL_O_CREAT | OSAL_O_RDWR, 0666);
	if (shm_fd < 0) {
		osal_printf("[ SKIP     ] Shared memory not available\n");
		return;
	}

	osal_ftruncate(shm_fd, shm_size);
	shm_ptr = osal_mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
						shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
		osal_close(shm_fd);
		osal_shm_unlink(shm_name);
		osal_printf("[ SKIP     ] mmap failed\n");
		return;
	}

	/* Write initial data */
	osal_strcpy((char *)shm_ptr, "Parent initial data");

	/* Fork child process */
	pid = osal_fork();
	if (pid < 0) {
		osal_printf("[ SKIP     ] Fork not available\n");
		osal_munmap(shm_ptr, shm_size);
		osal_close(shm_fd);
		osal_shm_unlink(shm_name);
		return;
	}

	if (pid == 0) {
		/* Child process - modify shared memory */
		osal_sleep(1); /* Let parent check initial data */

		char *data = (char *)shm_ptr;
		osal_strcpy(data, "Child modified data");

		osal_munmap(shm_ptr, shm_size);
		osal_close(shm_fd);
		osal_exit(0);
	} else {
		/* Parent process - verify initial then final data */
		TEST_ASSERT_STRING_EQUAL("Parent initial data", (char *)shm_ptr);

		/* Wait for child to modify */
		int status;
		osal_waitpid(pid, &status, 0);
		TEST_ASSERT_EQUAL(0, status);

		/* Verify child's modification */
		TEST_ASSERT_STRING_EQUAL("Child modified data", (char *)shm_ptr);

		/* Cleanup */
		osal_munmap(shm_ptr, shm_size);
		osal_close(shm_fd);
		osal_shm_unlink(shm_name);
	}

	osal_printf("[ PASS     ] Shared memory between processes test passed\n");
}

/**
 * Test semaphore-based process synchronization
 */
static void _test_semaphore_process_sync(void)
{
	osal_printf("[ TEST     ] Semaphore process synchronization\n");

	char shm_name[64];
	int shm_fd;
	size_t shm_size = 4096;
	void *shm_ptr;
	osal_sem_t *sem;
	osal_atomic_uint32_t *counter;
	pid_t pids[3];
	uint32_t i;

	/* Create shared memory for semaphore and counter */
	osal_snprintf(shm_name, sizeof(shm_name), "/test_sem_sync_%d",
				  osal_getpid());
	shm_fd = osal_shm_open(shm_name, OSAL_O_CREAT | OSAL_O_RDWR, 0666);
	if (shm_fd < 0) {
		osal_printf("[ SKIP     ] Shared memory not available\n");
		return;
	}

	osal_ftruncate(shm_fd, shm_size);
	shm_ptr = osal_mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED,
						shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
		osal_close(shm_fd);
		osal_shm_unlink(shm_name);
		osal_printf("[ SKIP     ] mmap failed\n");
		return;
	}

	/* Initialize semaphore and counter in shared memory */
	sem = (osal_sem_t *)shm_ptr;
	counter = (osal_atomic_uint32_t *)((char *)shm_ptr + sizeof(osal_sem_t));

	int32_t ret = osal_sem_init(sem, 1, 0); /* pshared=1 for process shared */
	if (ret != 0) {
		osal_munmap(shm_ptr, shm_size);
		osal_close(shm_fd);
		osal_shm_unlink(shm_name);
		osal_printf("[ SKIP     ] Process-shared semaphore not available\n");
		return;
	}

	osal_atomic_store(counter, 0);

	/* Fork child processes */
	for (i = 0; i < 3; i++) {
		pid_t pid = osal_fork();
		if (pid < 0) {
			osal_printf("[ WARN     ] Fork failed for child %u\n", i);
			continue;
		}

		if (pid == 0) {
			/* Child process - wait on semaphore and increment counter */
			osal_sem_wait(sem);
			osal_atomic_inc(counter);
			osal_munmap(shm_ptr, shm_size);
			osal_close(shm_fd);
			osal_exit(0);
		} else {
			pids[i] = pid;
		}
	}

	/* Parent: post semaphore 3 times */
	osal_sleep(1);
	for (i = 0; i < 3; i++) {
		osal_sem_post(sem);
		osal_msleep(100);
	}

	/* Wait for all children */
	for (i = 0; i < 3; i++) {
		if (pids[i] > 0) {
			int status;
			osal_waitpid(pids[i], &status, 0);
		}
	}

	/* Verify counter */
	TEST_ASSERT_EQUAL(3, osal_atomic_load(counter));

	/* Cleanup */
	osal_sem_destroy(sem);
	osal_munmap(shm_ptr, shm_size);
	osal_close(shm_fd);
	osal_shm_unlink(shm_name);

	osal_printf("[ PASS     ] Semaphore process sync test passed\n");
}

/* Signal handler flag */
static volatile int g_signal_received = 0;

static void _signal_handler(int signum)
{
	g_signal_received = signum;
}

/**
 * Test signal-based process coordination
 */
static void _test_signal_process_coordination(void)
{
	osal_printf("[ TEST     ] Signal-based process coordination\n");

	pid_t pid;

	/* Set up signal handler */
	osal_signal(SIGUSR1, _signal_handler);

	/* Fork child process */
	pid = osal_fork();
	if (pid < 0) {
		osal_printf("[ SKIP     ] Fork not available\n");
		return;
	}

	if (pid == 0) {
		/* Child process - send signal to parent */
		osal_sleep(1);
		osal_kill(osal_getppid(), SIGUSR1);
		osal_exit(0);
	} else {
		/* Parent process - wait for signal */
		g_signal_received = 0;

		/* Wait for signal with timeout */
		uint32_t timeout_ms = 3000;
		uint32_t elapsed_ms = 0;
		while (g_signal_received == 0 && elapsed_ms < timeout_ms) {
			osal_msleep(100);
			elapsed_ms += 100;
		}

		TEST_ASSERT_EQUAL(SIGUSR1, g_signal_received);

		/* Wait for child */
		int status;
		osal_waitpid(pid, &status, 0);
		TEST_ASSERT_EQUAL(0, status);

		/* Reset signal handler */
		osal_signal(SIGUSR1, SIG_DFL);
	}

	osal_printf("[ PASS     ] Signal process coordination test passed\n");
}

/**
 * Test process group operations
 */
static void _test_process_group_operations(void)
{
	osal_printf("[ TEST     ] Process group operations\n");

	pid_t pid1, pid2;
	pid_t pgid;

	/* Fork first child */
	pid1 = osal_fork();
	if (pid1 < 0) {
		osal_printf("[ SKIP     ] Fork not available\n");
		return;
	}

	if (pid1 == 0) {
		/* First child - create new process group */
		osal_setpgid(0, 0);
		osal_sleep(2);
		osal_exit(0);
	}

	/* Fork second child */
	pid2 = osal_fork();
	if (pid2 < 0) {
		int status;
		osal_waitpid(pid1, &status, 0);
		osal_printf("[ SKIP     ] Second fork failed\n");
		return;
	}

	if (pid2 == 0) {
		/* Second child - join first child's process group */
		osal_sleep(1);
		osal_setpgid(0, pid1);
		osal_sleep(1);
		osal_exit(0);
	}

	/* Parent - verify process groups */
	osal_sleep(1);

	pgid = osal_getpgid(pid1);
	TEST_ASSERT_EQUAL(pid1, pgid); /* First child is its own group leader */

	/* Wait for children */
	int status;
	osal_waitpid(pid1, &status, 0);
	TEST_ASSERT_EQUAL(0, status);

	osal_waitpid(pid2, &status, 0);
	TEST_ASSERT_EQUAL(0, status);

	osal_printf("[ PASS     ] Process group operations test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
	{ .name = "test_parent_child_pipe_communication",
	  .func = _test_parent_child_pipe_communication,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_shared_memory_between_processes",
	  .func = _test_shared_memory_between_processes,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_semaphore_process_sync",
	  .func = _test_semaphore_process_sync,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_signal_process_coordination",
	  .func = _test_signal_process_coordination,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_process_group_operations",
	  .func = _test_process_group_operations,
	  .setup = NULL,
	  .teardown = NULL },
};

/* Test suite definition */
static const test_suite_t test_suite = {
	.suite_name = "system_osal_process_ipc",
	.module_name = "system_osal",
	.layer_name = "OSAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_SYSTEM,
				  .tags = TEST_TAG_SLOW | TEST_TAG_IPC | TEST_TAG_PROCESS,
				  .timeout_ms = 20000,
				  .description = "Process + IPC integration tests" }
};

/* Register test suite */
void register_system_osal_process_ipc_tests(void)
{
	libutest_register_suite(&test_suite);
}
