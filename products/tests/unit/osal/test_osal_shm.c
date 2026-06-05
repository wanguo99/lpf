/************************************************************************
 * OSAL共享内存API单元测试
 ************************************************************************/

#include "test_framework.h"
#include "osal.h"

/* 测试多进程场景需要使用fork，使用OSAL封装接口 */

#define TEST_SHM_NAME   "/ems_test_shm"
#define TEST_SHM_SIZE   4096

/* 测试数据结构 */
struct test_data {
    int32_t magic;
    int32_t counter;
    char message[256];
};

/**
 * 测试1：创建和删除共享内存
 */
TEST_CASE(test_osal_shm_create_unlink) {
    osal_shm_t shm;
    int32_t ret;

    /* 先清理可能存在的旧对象 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(shm);

    /* 关闭句柄 */
    ret = OSAL_ShmClose(shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 删除共享内存 */
    ret = OSAL_ShmUnlink(TEST_SHM_NAME);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 再次删除应该失败 */
    ret = OSAL_ShmUnlink(TEST_SHM_NAME);
    TEST_ASSERT_EQUAL(OSAL_ENOENT, ret);
}

/**
 * 测试2：独占创建模式
 */
TEST_CASE(test_osal_shm_exclusive_create) {
    osal_shm_t shm1, shm2;
    int32_t ret;

    /* 清理 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存（独占模式） */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_EXCL | OSAL_SHM_RDWR, &shm1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 再次创建应该失败 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_EXCL | OSAL_SHM_RDWR, &shm2);
    TEST_ASSERT_EQUAL(OSAL_EEXIST, ret);

    /* 清理 */
    OSAL_ShmClose(shm1);
    OSAL_ShmUnlink(TEST_SHM_NAME);
}

/**
 * 测试3：映射和解除映射
 */
TEST_CASE(test_osal_shm_map_unmap) {
    osal_shm_t shm;
    void *addr;
    int32_t ret;

    /* 清理 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, &addr);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(addr);

    /* 写入数据 */
    struct test_data *data = (struct test_data *)addr;
    data->magic = 0x12345678;
    data->counter = 100;
    OSAL_Strcpy(data->message, "Hello from shared memory");

    /* 验证数据 */
    TEST_ASSERT_EQUAL(0x12345678, data->magic);
    TEST_ASSERT_EQUAL(100, data->counter);
    TEST_ASSERT_STRING_EQUAL("Hello from shared memory", data->message);

    /* 解除映射 */
    ret = OSAL_ShmUnmap(addr, TEST_SHM_SIZE);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 清理 */
    OSAL_ShmClose(shm);
    OSAL_ShmUnlink(TEST_SHM_NAME);
}

/**
 * 测试4：多进程共享（使用fork模拟）
 */
TEST_CASE(test_osal_shm_multiprocess) {
    osal_shm_t shm;
    void *addr;
    int32_t ret;
    osal_id_t pid;

    /* 清理 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 映射共享内存 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, &addr);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 初始化数据 */
    struct test_data *data = (struct test_data *)addr;
    data->magic = 0xABCDEF00;
    data->counter = 0;

    /* Fork子进程 */
    ret = OSAL_Fork(&pid);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    if (pid == 0) {
        /* 子进程：打开已存在的共享内存 */
        osal_shm_t child_shm;
        void *child_addr;

        ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE, OSAL_SHM_RDWR, &child_shm);
        if (ret != OSAL_SUCCESS) {
            OSAL_Exit(1);
        }

        ret = OSAL_ShmMap(child_shm, 0, 0, OSAL_SHM_RDWR, &child_addr);
        if (ret != OSAL_SUCCESS) {
            OSAL_Exit(2);
        }

        /* 修改共享数据 */
        struct test_data *child_data = (struct test_data *)child_addr;
        child_data->counter = 999;
        OSAL_Strcpy(child_data->message, "Modified by child");

        /* 同步到磁盘 */
        OSAL_ShmSync(child_addr, TEST_SHM_SIZE, false);

        /* 清理并退出 */
        OSAL_ShmUnmap(child_addr, TEST_SHM_SIZE);
        OSAL_ShmClose(child_shm);
        OSAL_Exit(0);
    } else {
        /* 父进程：等待子进程完成 */
        int32_t status;
        int32_t wait_ret = OSAL_Waitpid(pid, &status, 0);
        TEST_ASSERT(wait_ret > 0);
        TEST_ASSERT_EQUAL(0, status);

        /* 验证子进程的修改 */
        TEST_ASSERT_EQUAL((int32_t)0xABCDEF00, data->magic);
        TEST_ASSERT_EQUAL(999, data->counter);
        TEST_ASSERT_STRING_EQUAL("Modified by child", data->message);
    }

    /* 清理 */
    OSAL_ShmUnmap(addr, TEST_SHM_SIZE);
    OSAL_ShmClose(shm);
    OSAL_ShmUnlink(TEST_SHM_NAME);
}

/**
 * 测试5：只读映射
 */
TEST_CASE(test_osal_shm_readonly_map) {
    osal_shm_t shm;
    void *addr_rw, *addr_ro;
    int32_t ret;

    /* 清理 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 读写映射 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDWR, &addr_rw);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 写入数据 */
    struct test_data *data_rw = (struct test_data *)addr_rw;
    data_rw->magic = 0x11223344;
    data_rw->counter = 555;

    /* 只读映射 */
    ret = OSAL_ShmMap(shm, 0, 0, OSAL_SHM_RDONLY, &addr_ro);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证可以读取 */
    struct test_data *data_ro = (struct test_data *)addr_ro;
    TEST_ASSERT_EQUAL(0x11223344, data_ro->magic);
    TEST_ASSERT_EQUAL(555, data_ro->counter);

    /* 注意：写入只读映射会导致段错误，这里不测试 */

    /* 清理 */
    OSAL_ShmUnmap(addr_ro, TEST_SHM_SIZE);
    OSAL_ShmUnmap(addr_rw, TEST_SHM_SIZE);
    OSAL_ShmClose(shm);
    OSAL_ShmUnlink(TEST_SHM_NAME);
}

/**
 * 测试6：部分映射
 */
TEST_CASE(test_osal_shm_partial_map) {
    osal_shm_t shm;
    void *addr;
    int32_t ret;
    osal_size_t partial_size = 1024;

    /* 清理 */
    OSAL_ShmUnlink(TEST_SHM_NAME);

    /* 创建共享内存 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE,
                         OSAL_SHM_CREATE | OSAL_SHM_RDWR, &shm);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 部分映射（前1KB） */
    ret = OSAL_ShmMap(shm, 0, partial_size, OSAL_SHM_RDWR, &addr);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 写入数据 */
    int32_t *data = (int32_t *)addr;
    *data = 0xDEADBEEF;

    /* 验证 */
    TEST_ASSERT_EQUAL((int32_t)0xDEADBEEF, *data);

    /* 清理 */
    OSAL_ShmUnmap(addr, partial_size);
    OSAL_ShmClose(shm);
    OSAL_ShmUnlink(TEST_SHM_NAME);
}

/**
 * 测试7：参数校验
 */
TEST_CASE(test_osal_shm_parameter_validation) {
    osal_shm_t shm;
    void *addr;
    int32_t ret;

    /* NULL指针测试 */
    ret = OSAL_ShmCreate(NULL, TEST_SHM_SIZE, OSAL_SHM_CREATE, &shm);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    ret = OSAL_ShmCreate(TEST_SHM_NAME, TEST_SHM_SIZE, OSAL_SHM_CREATE, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* 无效名称（不以'/'开头） */
    ret = OSAL_ShmCreate("invalid_name", TEST_SHM_SIZE, OSAL_SHM_CREATE, &shm);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 大小为0 */
    ret = OSAL_ShmCreate(TEST_SHM_NAME, 0, OSAL_SHM_CREATE, &shm);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 映射NULL句柄 */
    ret = OSAL_ShmMap(NULL, 0, 0, OSAL_SHM_RDWR, &addr);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);

    /* 解除映射NULL地址 */
    ret = OSAL_ShmUnmap(NULL, TEST_SHM_SIZE);
    TEST_ASSERT_EQUAL(OSAL_EINVAL, ret);

    /* 删除NULL名称 */
    ret = OSAL_ShmUnlink(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试模块定义 */
TEST_MODULE_BEGIN(test_osal_shm, "OSAL")
    TEST_CASE_REGISTER(test_osal_shm_create_unlink, "Create and unlink shared memory")
    TEST_CASE_REGISTER(test_osal_shm_exclusive_create, "Exclusive create mode")
    TEST_CASE_REGISTER(test_osal_shm_map_unmap, "Map and unmap shared memory")
    TEST_CASE_REGISTER(test_osal_shm_multiprocess, "Multi-process shared memory")
    TEST_CASE_REGISTER(test_osal_shm_readonly_map, "Read-only mapping")
    TEST_CASE_REGISTER(test_osal_shm_partial_map, "Partial mapping")
    TEST_CASE_REGISTER(test_osal_shm_parameter_validation, "Parameter validation")
TEST_MODULE_END(test_osal_shm, "OSAL")
