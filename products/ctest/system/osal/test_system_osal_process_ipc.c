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
static void test_parent_child_pipe_communication(void)
{
    OSAL_printf("[ TEST     ] Parent-child pipe communication\n");

    int pipe_fds[2];
    int32_t ret;
    pid_t pid;

    /* Create pipe */
    ret = OSAL_pipe(pipe_fds);
    if (ret != 0) {
        OSAL_printf("[ SKIP     ] Pipe not available\n");
        return;
    }

    /* Fork child process */
    pid = OSAL_fork();
    if (pid < 0) {
        OSAL_printf("[ SKIP     ] Fork not available\n");
        OSAL_close(pipe_fds[0]);
        OSAL_close(pipe_fds[1]);
        return;
    }

    if (pid == 0) {
        /* Child process - reader */
        OSAL_close(pipe_fds[1]); /* Close write end */

        char buffer[128];
        ssize_t bytes_read = OSAL_read(pipe_fds[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if (OSAL_strcmp(buffer, "Hello from parent") == 0) {
                OSAL_exit(0); /* Success */
            }
        }
        OSAL_exit(1); /* Failure */
    } else {
        /* Parent process - writer */
        OSAL_close(pipe_fds[0]); /* Close read end */

        const char *message = "Hello from parent";
        OSAL_write(pipe_fds[1], message, OSAL_strlen(message));
        OSAL_close(pipe_fds[1]);

        /* Wait for child */
        int status;
        OSAL_waitpid(pid, &status, 0);
        TEST_ASSERT_EQUAL(0, OSAL_WEXITSTATUS(status));
    }

    OSAL_printf("[ PASS     ] Parent-child pipe communication test passed\n");
}

/**
 * Test shared memory between processes
 */
static void test_shared_memory_between_processes(void)
{
    OSAL_printf("[ TEST     ] Shared memory between processes\n");

    char shm_name[64];
    int shm_fd;
    size_t shm_size = 4096;
    void *shm_ptr;
    pid_t pid;

    /* Create shared memory */
    OSAL_snprintf(shm_name,
                  sizeof(shm_name),
                  "/test_proc_shm_%d",
                  OSAL_getpid());
    shm_fd = OSAL_shm_open(shm_name, OSAL_O_CREAT | OSAL_O_RDWR, 0666);
    if (shm_fd < 0) {
        OSAL_printf("[ SKIP     ] Shared memory not available\n");
        return;
    }

    OSAL_ftruncate(shm_fd, shm_size);
    shm_ptr = OSAL_mmap(NULL,
                        shm_size,
                        OSAL_PROT_READ | OSAL_PROT_WRITE,
                        OSAL_MAP_SHARED,
                        shm_fd,
                        0);
    if (shm_ptr == OSAL_MAP_FAILED) {
        OSAL_close(shm_fd);
        OSAL_shm_unlink(shm_name);
        OSAL_printf("[ SKIP     ] mmap failed\n");
        return;
    }

    /* Write initial data */
    OSAL_strcpy((char *)shm_ptr, "Parent initial data");

    /* Fork child process */
    pid = OSAL_fork();
    if (pid < 0) {
        OSAL_printf("[ SKIP     ] Fork not available\n");
        OSAL_munmap(shm_ptr, shm_size);
        OSAL_close(shm_fd);
        OSAL_shm_unlink(shm_name);
        return;
    }

    if (pid == 0) {
        /* Child process - modify shared memory */
        OSAL_sleep(1); /* Let parent check initial data */

        char *data = (char *)shm_ptr;
        OSAL_strcpy(data, "Child modified data");

        OSAL_munmap(shm_ptr, shm_size);
        OSAL_close(shm_fd);
        OSAL_exit(0);
    } else {
        /* Parent process - verify initial then final data */
        TEST_ASSERT_STRING_EQUAL("Parent initial data", (char *)shm_ptr);

        /* Wait for child to modify */
        int status;
        OSAL_waitpid(pid, &status, 0);
        TEST_ASSERT_EQUAL(0, OSAL_WEXITSTATUS(status));

        /* Verify child's modification */
        TEST_ASSERT_STRING_EQUAL("Child modified data", (char *)shm_ptr);

        /* Cleanup */
        OSAL_munmap(shm_ptr, shm_size);
        OSAL_close(shm_fd);
        OSAL_shm_unlink(shm_name);
    }

    OSAL_printf("[ PASS     ] Shared memory between processes test passed\n");
}

/**
 * Test semaphore-based process synchronization
 */
static void test_semaphore_process_sync(void)
{
    OSAL_printf("[ TEST     ] Semaphore process synchronization\n");

    char shm_name[64];
    int shm_fd;
    size_t shm_size = 4096;
    void *shm_ptr;
    osal_sem_t *sem;
    osal_atomic_uint32_t *counter;
    pid_t pids[3];
    uint32_t i;

    /* Create shared memory for semaphore and counter */
    OSAL_snprintf(shm_name,
                  sizeof(shm_name),
                  "/test_sem_sync_%d",
                  OSAL_getpid());
    shm_fd = OSAL_shm_open(shm_name, OSAL_O_CREAT | OSAL_O_RDWR, 0666);
    if (shm_fd < 0) {
        OSAL_printf("[ SKIP     ] Shared memory not available\n");
        return;
    }

    OSAL_ftruncate(shm_fd, shm_size);
    shm_ptr = OSAL_mmap(NULL,
                        shm_size,
                        OSAL_PROT_READ | OSAL_PROT_WRITE,
                        OSAL_MAP_SHARED,
                        shm_fd,
                        0);
    if (shm_ptr == OSAL_MAP_FAILED) {
        OSAL_close(shm_fd);
        OSAL_shm_unlink(shm_name);
        OSAL_printf("[ SKIP     ] mmap failed\n");
        return;
    }

    /* Initialize semaphore and counter in shared memory */
    sem = (osal_sem_t *)shm_ptr;
    counter = (osal_atomic_uint32_t *)((char *)shm_ptr + sizeof(osal_sem_t));

    int32_t ret = OSAL_sem_init(sem, 1, 0); /* pshared=1 for process shared */
    if (ret != 0) {
        OSAL_munmap(shm_ptr, shm_size);
        OSAL_close(shm_fd);
        OSAL_shm_unlink(shm_name);
        OSAL_printf("[ SKIP     ] Process-shared semaphore not available\n");
        return;
    }

    OSAL_atomic_store(counter, 0);

    /* Fork child processes */
    for (i = 0; i < 3; i++) {
        pid_t pid = OSAL_fork();
        if (pid < 0) {
            OSAL_printf("[ WARN     ] Fork failed for child %u\n", i);
            continue;
        }

        if (pid == 0) {
            /* Child process - wait on semaphore and increment counter */
            OSAL_sem_wait(sem);
            OSAL_atomic_inc(counter);
            OSAL_munmap(shm_ptr, shm_size);
            OSAL_close(shm_fd);
            OSAL_exit(0);
        } else {
            pids[i] = pid;
        }
    }

    /* Parent: post semaphore 3 times */
    OSAL_sleep(1);
    for (i = 0; i < 3; i++) {
        OSAL_sem_post(sem);
        OSAL_msleep(100);
    }

    /* Wait for all children */
    for (i = 0; i < 3; i++) {
        if (pids[i] > 0) {
            int status;
            OSAL_waitpid(pids[i], &status, 0);
        }
    }

    /* Verify counter */
    TEST_ASSERT_EQUAL(3, OSAL_atomic_load(counter));

    /* Cleanup */
    OSAL_sem_destroy(sem);
    OSAL_munmap(shm_ptr, shm_size);
    OSAL_close(shm_fd);
    OSAL_shm_unlink(shm_name);

    OSAL_printf("[ PASS     ] Semaphore process sync test passed\n");
}

/* Signal handler flag */
static volatile int g_signal_received = 0;

static void signal_handler(int signum)
{
    g_signal_received = signum;
}

/**
 * Test signal-based process coordination
 */
static void test_signal_process_coordination(void)
{
    OSAL_printf("[ TEST     ] Signal-based process coordination\n");

    pid_t pid;

    /* Set up signal handler */
    OSAL_signal(OSAL_SIGUSR1, signal_handler);

    /* Fork child process */
    pid = OSAL_fork();
    if (pid < 0) {
        OSAL_printf("[ SKIP     ] Fork not available\n");
        return;
    }

    if (pid == 0) {
        /* Child process - send signal to parent */
        OSAL_sleep(1);
        OSAL_kill(OSAL_getppid(), OSAL_SIGUSR1);
        OSAL_exit(0);
    } else {
        /* Parent process - wait for signal */
        g_signal_received = 0;

        /* Wait for signal with timeout */
        uint32_t timeout_ms = 3000;
        uint32_t elapsed_ms = 0;
        while (g_signal_received == 0 && elapsed_ms < timeout_ms) {
            OSAL_msleep(100);
            elapsed_ms += 100;
        }

        TEST_ASSERT_EQUAL(OSAL_SIGUSR1, g_signal_received);

        /* Wait for child */
        int status;
        OSAL_waitpid(pid, &status, 0);
        TEST_ASSERT_EQUAL(0, OSAL_WEXITSTATUS(status));

        /* Reset signal handler */
        OSAL_signal(OSAL_SIGUSR1, OSAL_SIG_DFL);
    }

    OSAL_printf("[ PASS     ] Signal process coordination test passed\n");
}

/**
 * Test process group operations
 */
static void test_process_group_operations(void)
{
    OSAL_printf("[ TEST     ] Process group operations\n");

    pid_t pid1, pid2;
    pid_t pgid;

    /* Fork first child */
    pid1 = OSAL_fork();
    if (pid1 < 0) {
        OSAL_printf("[ SKIP     ] Fork not available\n");
        return;
    }

    if (pid1 == 0) {
        /* First child - create new process group */
        OSAL_setpgid(0, 0);
        OSAL_sleep(2);
        OSAL_exit(0);
    }

    /* Fork second child */
    pid2 = OSAL_fork();
    if (pid2 < 0) {
        int status;
        OSAL_waitpid(pid1, &status, 0);
        OSAL_printf("[ SKIP     ] Second fork failed\n");
        return;
    }

    if (pid2 == 0) {
        /* Second child - join first child's process group */
        OSAL_sleep(1);
        OSAL_setpgid(0, pid1);
        OSAL_sleep(1);
        OSAL_exit(0);
    }

    /* Parent - verify process groups */
    OSAL_sleep(1);

    pgid = OSAL_getpgid(pid1);
    TEST_ASSERT_EQUAL(pid1, pgid); /* First child is its own group leader */

    /* Wait for children */
    int status;
    OSAL_waitpid(pid1, &status, 0);
    TEST_ASSERT_EQUAL(0, OSAL_WEXITSTATUS(status));

    OSAL_waitpid(pid2, &status, 0);
    TEST_ASSERT_EQUAL(0, OSAL_WEXITSTATUS(status));

    OSAL_printf("[ PASS     ] Process group operations test passed\n");
}

/* Test cases array */
static const test_case_t test_cases[] = {
    { .name = "test_parent_child_pipe_communication",
      .func = test_parent_child_pipe_communication,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_shared_memory_between_processes",
      .func = test_shared_memory_between_processes,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_semaphore_process_sync",
      .func = test_semaphore_process_sync,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_signal_process_coordination",
      .func = test_signal_process_coordination,
      .setup = NULL,
      .teardown = NULL },
    { .name = "test_process_group_operations",
      .func = test_process_group_operations,
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
__attribute__((constructor)) static void
register_system_osal_process_ipc_tests(void)
{
    libutest_register_suite(&test_suite);
}
