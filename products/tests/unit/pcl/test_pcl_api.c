#include "test_framework.h"
/**
 * @file test_pcl_api.c
 * @brief PCL API单元测试
 *
 * 测试覆盖：
 * - 配置库初始化和清理
 * - 配置注册和查询
 * - 硬件外设配置查询（MCU/BMC/FPGA/Switch）
 * - 配置验证
 */

#include "pconfig/pconfig.h"
#include "osal.h"

/* 测试用的配置数据 */
static pconfig_mcu_cfg_t test_mcu = {
    .name = "test_mcu",
    .description = "Test MCU",
    .enabled = true,
    .interface = PCONFIG_MCU_INTERFACE_SERIAL,
    .serial = {
        .device = "/dev/ttyS0",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE
    },
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = false,
    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static pconfig_bmc_cfg_t test_bmc = {
    .name = "test_bmc",
    .description = "Test BMC",
    .enabled = true,

    /* 网络配置 */
    .network = {
        .enabled = true,
        .ip_addr = "192.168.1.100",
        .port = 623,
        .username = "admin",
        .password = "admin",
        .timeout_ms = 5000
    },

    /* 串口配置 */
    .serial = {
        .enabled = true,
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .timeout_ms = 5000
    },

    .primary_channel = PCONFIG_BMC_CHANNEL_NETWORK,
    .auto_switch = true,
    .retry_count = 3,
    .health_check_interval = 5000,
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pconfig_fpga_cfg_t test_fpga = {
    .name = "test_fpga",
    .description = "Test FPGA",
    .enabled = true,
    .interface_type = PCONFIG_HW_INTERFACE_SPI,
    .interface_cfg = {
        .spi = {
            .device = "/dev/spidev0.0",
            .speed_hz = 1000000,
            .mode = 0,
            .bits_per_word = 8
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3
};

static pconfig_switch_cfg_t test_switch = {
    .name = "test_switch",
    .description = "Test Switch",
    .enabled = true,
    .interface_type = PCONFIG_HW_INTERFACE_I2C,
    .interface_cfg = {
        .i2c = {
            .device = "/dev/i2c-0",
            .slave_addr = 0x50,
            .speed_hz = 100000
        }
    },
    .cmd_timeout_ms = 1000,
    .retry_count = 3
};

static pconfig_mcu_cfg_t *test_mcu_arr[] = { &test_mcu, NULL };
static pconfig_bmc_cfg_t *test_bmc_arr[] = { &test_bmc, NULL };
static pconfig_fpga_cfg_t *test_fpga_arr[] = { &test_fpga, NULL };
static pconfig_switch_cfg_t *test_switch_arr[] = { &test_switch, NULL };

static const pconfig_platform_config_t test_platform_v1 = {
    .platform_name = "test_platform",
    .chip_name = "test_chip",
    .project_name = "test_project",
    .product_name = "test_product_v1",
    .mcu_arr = test_mcu_arr,
    .bmc_arr = test_bmc_arr,
    .fpga_arr = test_fpga_arr,
    .switch_arr = test_switch_arr
};

static const pconfig_platform_config_t test_platform_v2 = {
    .platform_name = "test_platform",
    .chip_name = "test_chip",
    .project_name = "test_project",
    .product_name = "test_product_v2",
    .mcu_arr = test_mcu_arr,
    .bmc_arr = test_bmc_arr,
    .fpga_arr = NULL,
    .switch_arr = NULL
};

/* ========== 初始化和清理测试 ========== */

static void test_pcl_init_success(void)
{
    int32_t ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 重复初始化应该成功 */
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCONFIG_Cleanup();
}

static void test_pcl_cleanup(void)
{
    PCONFIG_Init();
    PCONFIG_Cleanup();

    /* 重复清理应该安全 */
    PCONFIG_Cleanup();
}

/* ========== 配置注册测试 ========== */

static void test_pcl_register_success(void)
{
    PCONFIG_Init();

    int32_t ret = PCONFIG_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCONFIG_Cleanup();
}

static void test_pcl_register_null_pointer(void)
{
    PCONFIG_Init();

    int32_t ret = PCONFIG_Register(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCONFIG_Cleanup();
}

static void test_pcl_register_not_initialized(void)
{
    /* 未初始化时注册应该失败 */
    int32_t ret = PCONFIG_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pcl_register_duplicate(void)
{
    PCONFIG_Init();

    int32_t ret = PCONFIG_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 重复注册相同配置应该失败 */
    ret = PCONFIG_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCONFIG_Cleanup();
}

static void test_pcl_register_multiple_versions(void)
{
    PCONFIG_Init();

    int32_t ret = PCONFIG_Register(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 注册不同版本应该成功 */
    ret = PCONFIG_Register(&test_platform_v2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    PCONFIG_Cleanup();
}

/* ========== 配置查询测试 ========== */

static void test_pcl_find_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_platform_config_t *found = PCONFIG_Find("test_platform", "test_product_v1", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_product_v1", found->product_name);

    PCONFIG_Cleanup();
}

static void test_pcl_find_without_version(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);
    PCONFIG_Register(&test_platform_v2);

    /* 不指定版本应该返回第一个匹配的 */
    const pconfig_platform_config_t *found = PCONFIG_Find("test_platform", "test_product_v1", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("test_platform", found->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_product_v1", found->product_name);

    PCONFIG_Cleanup();
}

static void test_pcl_find_not_found(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_platform_config_t *found = PCONFIG_Find("nonexistent", "product", NULL);
    TEST_ASSERT_NULL(found);

    PCONFIG_Cleanup();
}

static void test_pcl_find_null_parameters(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_platform_config_t *found = PCONFIG_Find(NULL, "test_product_v1", NULL);
    TEST_ASSERT_NULL(found);

    found = PCONFIG_Find("test_platform", NULL, NULL);
    TEST_ASSERT_NULL(found);

    PCONFIG_Cleanup();
}

static void test_pcl_list_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);
    PCONFIG_Register(&test_platform_v2);

    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCONFIG_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_NOT_NULL(configs[0]);
    TEST_ASSERT_NOT_NULL(configs[1]);

    PCONFIG_Cleanup();
}

static void test_pcl_list_null_parameters(void)
{
    PCONFIG_Init();

    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCONFIG_List(NULL, &count);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    ret = PCONFIG_List(configs, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);

    PCONFIG_Cleanup();
}

/* ========== 硬件外设查询测试 ========== */

static void test_pcl_hw_find_mcu_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_mcu_cfg_t *mcu = PCONFIG_HW_FindMCU(&test_platform_v1, "test_mcu");
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);
    TEST_ASSERT_EQUAL(PCONFIG_MCU_INTERFACE_SERIAL, mcu->interface);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_find_mcu_not_found(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_mcu_cfg_t *mcu = PCONFIG_HW_FindMCU(&test_platform_v1, "nonexistent");
    TEST_ASSERT_NULL(mcu);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_find_mcu_null_parameters(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_mcu_cfg_t *mcu = PCONFIG_HW_FindMCU(NULL, "test_mcu");
    TEST_ASSERT_NULL(mcu);

    mcu = PCONFIG_HW_FindMCU(&test_platform_v1, NULL);
    TEST_ASSERT_NULL(mcu);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_get_mcu_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_mcu_cfg_t *mcu = PCONFIG_HW_GetMCU(&test_platform_v1, 0);
    TEST_ASSERT_NOT_NULL(mcu);
    TEST_ASSERT_STRING_EQUAL("test_mcu", mcu->name);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_get_mcu_invalid_id(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_mcu_cfg_t *mcu = PCONFIG_HW_GetMCU(&test_platform_v1, 999);
    TEST_ASSERT_NULL(mcu);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_find_bmc_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_bmc_cfg_t *bmc = PCONFIG_HW_FindBMC(&test_platform_v1, "test_bmc");
    TEST_ASSERT_NOT_NULL(bmc);
    TEST_ASSERT_STRING_EQUAL("test_bmc", bmc->name);
    TEST_ASSERT_EQUAL(PCONFIG_BMC_CHANNEL_NETWORK, bmc->primary_channel);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_find_fpga_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_fpga_cfg_t *fpga = PCONFIG_HW_FindFPGA(&test_platform_v1, "test_fpga");
    TEST_ASSERT_NOT_NULL(fpga);
    TEST_ASSERT_STRING_EQUAL("test_fpga", fpga->name);
    TEST_ASSERT_EQUAL(PCONFIG_HW_INTERFACE_SPI, fpga->interface_type);

    PCONFIG_Cleanup();
}

static void test_pcl_hw_find_switch_success(void)
{
    PCONFIG_Init();
    PCONFIG_Register(&test_platform_v1);

    const pconfig_switch_cfg_t *sw = PCONFIG_HW_FindSwitch(&test_platform_v1, "test_switch");
    TEST_ASSERT_NOT_NULL(sw);
    TEST_ASSERT_STRING_EQUAL("test_switch", sw->name);
    TEST_ASSERT_EQUAL(PCONFIG_HW_INTERFACE_I2C, sw->interface_type);

    PCONFIG_Cleanup();
}

/* ========== 配置验证测试 ========== */

static void test_pcl_validate_success(void)
{
    int32_t ret = PCONFIG_Validate(&test_platform_v1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pcl_validate_null_pointer(void)
{
    int32_t ret = PCONFIG_Validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

/* ========== 多线程并发测试 ========== */

#define NUM_THREADS 10
#define NUM_ITERATIONS 100

/* 线程测试数据 */
typedef struct {
    int thread_id;
    int success_count;
    int error_count;
} thread_test_data_t;

/* 测试用的多个配置 */
static pconfig_platform_config_t test_configs[NUM_THREADS];
static pconfig_mcu_cfg_t test_mcus[NUM_THREADS];
static pconfig_mcu_cfg_t *test_mcu_arrays[NUM_THREADS][2];

/* 初始化测试配置数组 */
static void init_test_configs(void)
{
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        /* 初始化MCU配置 */
        OSAL_memset(&test_mcus[i], 0, OSAL_sizeof(pconfig_mcu_cfg_t));
        OSAL_snprintf((char *)test_mcus[i].name, 32, "mcu_%d", i);
        test_mcus[i].description = "Thread test MCU";
        test_mcus[i].enabled = true;
        test_mcus[i].interface = PCONFIG_MCU_INTERFACE_SERIAL;

        /* 设置MCU数组 */
        test_mcu_arrays[i][0] = &test_mcus[i];
        test_mcu_arrays[i][1] = NULL;

        /* 初始化平台配置 */
        OSAL_memset(&test_configs[i], 0, OSAL_sizeof(pconfig_platform_config_t));
        test_configs[i].platform_name = "test_platform";
        test_configs[i].chip_name = "test_chip";
        test_configs[i].project_name = "test_project";

        /* 为每个线程创建唯一的产品名 */
        static char product_names[NUM_THREADS][32];
        OSAL_snprintf(product_names[i], 32, "product_%d", i);
        test_configs[i].product_name = product_names[i];
        test_configs[i].mcu_arr = test_mcu_arrays[i];
    }
}

/* 线程1：并发注册配置 */
static void* thread_register_func(void *arg)
{
    thread_test_data_t *data = (thread_test_data_t *)arg;
    int i;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        int32_t ret = PCONFIG_Register(&test_configs[data->thread_id]);
        if (ret == OSAL_SUCCESS) {
            data->success_count++;
        } else {
            data->error_count++;
        }

        /* 短暂休眠，增加并发冲突概率 */
        OSAL_Sleep(1);
    }

    return NULL;
}

/* 线程2：并发查询配置 */
static void* thread_find_func(void *arg)
{
    thread_test_data_t *data = (thread_test_data_t *)arg;
    int i;
    char product_name[32];

    OSAL_snprintf(product_name, 32, "product_%d", data->thread_id % NUM_THREADS);

    for (i = 0; i < NUM_ITERATIONS; i++) {
        const pconfig_platform_config_t *found = PCONFIG_Find("test_platform", product_name, NULL);
        if (found != NULL) {
            data->success_count++;
        } else {
            data->error_count++;
        }

        OSAL_Sleep(1);
    }

    return NULL;
}

/* 线程3：并发列举配置 */
static void* thread_list_func(void *arg)
{
    thread_test_data_t *data = (thread_test_data_t *)arg;
    int i;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        const pconfig_platform_config_t *configs[32];
        uint32_t count = 32;

        int32_t ret = PCONFIG_List(configs, &count);
        if (ret == OSAL_SUCCESS) {
            data->success_count++;
        } else {
            data->error_count++;
        }

        OSAL_Sleep(1);
    }

    return NULL;
}

/* 线程4：并发获取当前配置 */
static void* thread_getboard_func(void *arg)
{
    thread_test_data_t *data = (thread_test_data_t *)arg;
    int i;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        const pconfig_platform_config_t *board = PCONFIG_GetBoard();
        /* GetBoard可能返回NULL（如果没有设置current），这是正常的 */
        data->success_count++;
        (void)board;  /* 避免未使用变量警告 */

        OSAL_Sleep(1);
    }

    return NULL;
}

static void test_pcl_concurrent_register(void)
{
    osal_thread_t threads[NUM_THREADS];
    thread_test_data_t thread_data[NUM_THREADS];
    int i;
    int32_t ret;

    /* 初始化测试配置 */
    init_test_configs();

    /* 初始化PCL */
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 创建多个线程并发注册配置 */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].success_count = 0;
        thread_data[i].error_count = 0;

        ret = OSAL_pthread_create(&threads[i], NULL, thread_register_func, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证结果：每个配置应该只被成功注册一次 */
    for (i = 0; i < NUM_THREADS; i++) {
        TEST_ASSERT_EQUAL(1, thread_data[i].success_count);
        TEST_ASSERT_EQUAL(NUM_ITERATIONS - 1, thread_data[i].error_count);
    }

    /* 验证注册表中有正确数量的配置 */
    const pconfig_platform_config_t *configs[32];
    uint32_t count = 32;
    ret = PCONFIG_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(NUM_THREADS, count);

    PCONFIG_Cleanup();
}

static void test_pcl_concurrent_find(void)
{
    osal_thread_t threads[NUM_THREADS];
    thread_test_data_t thread_data[NUM_THREADS];
    int i;
    int32_t ret;

    /* 初始化测试配置 */
    init_test_configs();

    /* 初始化PCL并注册配置 */
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    for (i = 0; i < NUM_THREADS; i++) {
        ret = PCONFIG_Register(&test_configs[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 创建多个线程并发查询配置 */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].success_count = 0;
        thread_data[i].error_count = 0;

        ret = OSAL_pthread_create(&threads[i], NULL, thread_find_func, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (i = 0; i < NUM_THREADS; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证结果：所有查询都应该成功 */
    for (i = 0; i < NUM_THREADS; i++) {
        TEST_ASSERT_EQUAL(NUM_ITERATIONS, thread_data[i].success_count);
        TEST_ASSERT_EQUAL(0, thread_data[i].error_count);
    }

    PCONFIG_Cleanup();
}

static void test_pcl_concurrent_mixed_operations(void)
{
    osal_thread_t threads[NUM_THREADS * 3];
    thread_test_data_t thread_data[NUM_THREADS * 3];
    int i;
    int32_t ret;

    /* 初始化测试配置 */
    init_test_configs();

    /* 初始化PCL并注册一些配置 */
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    for (i = 0; i < NUM_THREADS / 2; i++) {
        ret = PCONFIG_Register(&test_configs[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 创建多个线程执行不同操作 */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].success_count = 0;
        thread_data[i].error_count = 0;

        /* 线程组1：查询操作 */
        ret = OSAL_pthread_create(&threads[i], NULL, thread_find_func, &thread_data[i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        /* 线程组2：列举操作 */
        thread_data[NUM_THREADS + i].thread_id = i;
        thread_data[NUM_THREADS + i].success_count = 0;
        thread_data[NUM_THREADS + i].error_count = 0;
        ret = OSAL_pthread_create(&threads[NUM_THREADS + i], NULL, thread_list_func,
                                &thread_data[NUM_THREADS + i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        /* 线程组3：获取当前配置 */
        thread_data[NUM_THREADS * 2 + i].thread_id = i;
        thread_data[NUM_THREADS * 2 + i].success_count = 0;
        thread_data[NUM_THREADS * 2 + i].error_count = 0;
        ret = OSAL_pthread_create(&threads[NUM_THREADS * 2 + i], NULL, thread_getboard_func,
                                &thread_data[NUM_THREADS * 2 + i]);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    }

    /* 等待所有线程完成 */
    for (i = 0; i < NUM_THREADS * 3; i++) {
        OSAL_pthread_join(threads[i], NULL);
    }

    /* 验证没有发生崩溃或死锁 */
    TEST_ASSERT_TRUE(true);

    PCONFIG_Cleanup();
}

/* ========== 测试套件注册 ========== */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_pcl_init_success",
		.func = test_pcl_init_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_cleanup",
		.func = test_pcl_cleanup,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_register_success",
		.func = test_pcl_register_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_register_null_pointer",
		.func = test_pcl_register_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_register_not_initialized",
		.func = test_pcl_register_not_initialized,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_register_duplicate",
		.func = test_pcl_register_duplicate,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_register_multiple_versions",
		.func = test_pcl_register_multiple_versions,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_find_success",
		.func = test_pcl_find_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_find_without_version",
		.func = test_pcl_find_without_version,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_find_not_found",
		.func = test_pcl_find_not_found,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_find_null_parameters",
		.func = test_pcl_find_null_parameters,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_list_success",
		.func = test_pcl_list_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_list_null_parameters",
		.func = test_pcl_list_null_parameters,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_mcu_success",
		.func = test_pcl_hw_find_mcu_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_mcu_not_found",
		.func = test_pcl_hw_find_mcu_not_found,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_mcu_null_parameters",
		.func = test_pcl_hw_find_mcu_null_parameters,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_get_mcu_success",
		.func = test_pcl_hw_get_mcu_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_get_mcu_invalid_id",
		.func = test_pcl_hw_get_mcu_invalid_id,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_bmc_success",
		.func = test_pcl_hw_find_bmc_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_fpga_success",
		.func = test_pcl_hw_find_fpga_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_hw_find_switch_success",
		.func = test_pcl_hw_find_switch_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_validate_success",
		.func = test_pcl_validate_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_validate_null_pointer",
		.func = test_pcl_validate_null_pointer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_concurrent_register",
		.func = test_pcl_concurrent_register,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_concurrent_find",
		.func = test_pcl_concurrent_find,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_pcl_concurrent_mixed_operations",
		.func = test_pcl_concurrent_mixed_operations,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "pcl_api",
	.module_name = "pcl_api",
	.layer_name = "PCL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "PCL pcl_api tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_pcl_api_tests(void)
{
	libutest_register_suite(&test_suite);
}
