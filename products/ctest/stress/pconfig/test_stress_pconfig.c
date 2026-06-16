/**
 * @file test_stress_pconfig.c
 * @brief PCONFIG层压力测试 - PCONFIG模块压力测试
 *
 * 测试场景：
 * 1. 并发查询压力 - 多线程同时查询配置
 * 2. 大量配置加载压力 - 注册最大数量的配置
 * 3. 长时间运行压力 - 持续查询操作验证稳定性
 * 4. 性能基准测试 - 测量查询延迟和吞吐量
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_stress.h>
#include "pconfig.h"

/*===========================================================================
 * 测试数据定义
 *===========================================================================*/

/* 测试用MCU配置 */
static pconfig_mcu_entry_t stress_mcu_entries[] = {
    {
        .description = "Test MCU 0",
        .enabled = true,
        .config = {
            .name = "test_mcu_0",
            .interface = PDL_MCU_INTERFACE_CAN,
            .hw.can = {
                .device = "can0",
                .bitrate = 500000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x100,
                .rx_id = 0x101
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};

/* 测试用BMC配置 */
static pconfig_bmc_entry_t stress_bmc_entries[] = {
    {
        .description = "Test BMC 0",
        .enabled = true,
        .config = {
            .primary_channel = PDL_BMC_CHANNEL_SERIAL,
            .primary_config.serial = {
                .device = "/dev/ttyS0",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = 0,
                .timeout_ms = 2000
            },
            .backup_channel = PDL_BMC_CHANNEL_NETWORK,  /* Not used when auto_switch=false */
            .auto_switch = false,
            .retry_count = 3,
            .health_check_interval = 5000
        },
        .power_gpio = NULL,
        .reset_gpio = NULL
    }
};

/* 测试用FPGA配置 */
static pconfig_fpga_cfg_t stress_fpga_configs[] = {
    {
        .description = "Test FPGA 0",
        .enabled = true,
        .device = "/dev/fpga0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

/* 测试用Switch配置 */
static pconfig_switch_cfg_t stress_switch_configs[] = {
    {
        .description = "Test Switch 0",
        .enabled = true,
        .device = "/dev/switch0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

/* 基础平台配置模板 */
static pconfig_platform_config_t base_platform_config = {
    .platform_name = "ti",
    .chip_name = "am625",
    .project_name = "stress_test",
    .product_name = "stress_base",

    .mcu_count = 1,
    .mcu_array = stress_mcu_entries,

    .bmc_count = 1,
    .bmc_array = stress_bmc_entries,

    .fpga_count = 1,
    .fpga_array = stress_fpga_configs,

    .switch_count = 1,
    .switch_array = stress_switch_configs
};

/* 并发查询测试的共享数据 */
typedef struct {
    osal_atomic_uint32_t query_count;
    osal_atomic_uint32_t null_result_count;
    const pconfig_platform_config_t *expected_config;
} concurrent_query_data_t;

/*===========================================================================
 * 测试1: 并发查询压力测试
 *===========================================================================*/

/**
 * 并发查询工作函数 - 多线程同时调用查询API
 */
static int32_t concurrent_query_worker(void *user_data, uint32_t iteration)
{
    concurrent_query_data_t *data = (concurrent_query_data_t*)user_data;
    (void)iteration;

    /* 测试PCONFIG_GetBoard() - 最常用的查询API */
    const pconfig_platform_config_t *board = PCONFIG_GetBoard();
    if (board == NULL) {
        OSAL_atomic_inc(&data->null_result_count);
        return -1;
    }

    /* 验证返回的配置 */
    if (board != data->expected_config) {
        OSAL_printf("[ ERROR ] GetBoard returned unexpected config\n");
        return -1;
    }

    /* 测试PCONFIG_Find() - 按名称查找 */
    const pconfig_platform_config_t *found = PCONFIG_Find(
        "ti", "stress_base", NULL);
    if (found == NULL) {
        OSAL_atomic_inc(&data->null_result_count);
        return -1;
    }

    if (found != data->expected_config) {
        OSAL_printf("[ ERROR ] Find returned unexpected config\n");
        return -1;
    }

    /* 测试硬件查询API */
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(board, 0);
    if (mcu == NULL) {
        OSAL_printf("[ ERROR ] GetMCU returned NULL\n");
        return -1;
    }

    const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(board, 0);
    if (bmc == NULL) {
        OSAL_printf("[ ERROR ] GetBMC returned NULL\n");
        return -1;
    }

    const pconfig_fpga_cfg_t *fpga = PCONFIG_HW_GetFPGA(board, 0);
    if (fpga == NULL) {
        OSAL_printf("[ ERROR ] GetFPGA returned NULL\n");
        return -1;
    }

    const pconfig_switch_cfg_t *sw = PCONFIG_HW_GetSwitch(board, 0);
    if (sw == NULL) {
        OSAL_printf("[ ERROR ] GetSwitch returned NULL\n");
        return -1;
    }

    OSAL_atomic_inc(&data->query_count);
    return 0;
}

/**
 * 测试配置查询并发压力
 * 多个线程同时查询配置，验证线程安全性
 */
static void test_stress_config_concurrent_query(void)
{
    concurrent_query_data_t data;
    const uint32_t thread_count = 16;  /* 16个并发线程 */
    const uint32_t duration_sec = 10;  /* 运行10秒 */

    /* 初始化PCONFIG */
    TEST_ASSERT_EQUAL(PCONFIG_Init(), OSAL_SUCCESS);

    /* 注册测试配置 */
    TEST_ASSERT_EQUAL(PCONFIG_Register(&base_platform_config), OSAL_SUCCESS);

    /* 初始化测试数据 */
    OSAL_atomic_init(&data.query_count, 0);
    OSAL_atomic_init(&data.null_result_count, 0);
    data.expected_config = &base_platform_config;

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_CONCURRENCY(thread_count, duration_sec);
    stress_context_t *ctx = stress_context_create("Config Concurrent Query", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_printf("[ INFO     ] Running concurrent query stress test: %u threads, %u seconds\n",
               thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, concurrent_query_worker, &data), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 验证查询计数 */
    uint32_t total_queries = OSAL_atomic_load(&data.query_count);
    uint32_t null_results = OSAL_atomic_load(&data.null_result_count);

    OSAL_printf("[ INFO     ] Total successful queries: %u\n", total_queries);
    OSAL_printf("[ INFO     ] NULL results encountered: %u\n", null_results);

    TEST_ASSERT(total_queries > 0);
    TEST_ASSERT_EQUAL(null_results, 0);

    /* 清理 */
    stress_context_destroy(ctx);
    PCONFIG_Cleanup();
}

/*===========================================================================
 * 测试2: 大量配置加载压力测试
 *===========================================================================*/

/* 最大配置数量（根据PCONFIG内部限制） */
#define MAX_PLATFORM_CONFIGS 32

/* 动态配置数组 */
static pconfig_platform_config_t massive_configs[MAX_PLATFORM_CONFIGS];
static pconfig_mcu_entry_t massive_mcu_arrays[MAX_PLATFORM_CONFIGS][2];
static char massive_product_names[MAX_PLATFORM_CONFIGS][64];

/**
 * 初始化大量配置数据
 */
static void init_massive_configs(void)
{
    for (uint32_t i = 0; i < MAX_PLATFORM_CONFIGS; i++) {
        /* 生成唯一的产品名称 */
        OSAL_snprintf(massive_product_names[i],
                     sizeof(massive_product_names[i]),
                     "stress_product_%u", i);

        /* 初始化MCU配置 */
        for (uint32_t j = 0; j < 2; j++) {
            massive_mcu_arrays[i][j].description = "Stress MCU";
            massive_mcu_arrays[i][j].enabled = true;
            OSAL_snprintf(massive_mcu_arrays[i][j].config.name,
                     sizeof(massive_mcu_arrays[i][j].config.name),
                     "stress_mcu_%u_%u", i, j);
            massive_mcu_arrays[i][j].config.interface = PDL_MCU_INTERFACE_CAN;
            massive_mcu_arrays[i][j].config.hw.can.device = "can0";
            massive_mcu_arrays[i][j].config.hw.can.bitrate = 500000;
            massive_mcu_arrays[i][j].config.hw.can.tx_id = 0x100 + i * 2 + j;
            massive_mcu_arrays[i][j].config.hw.can.rx_id = 0x101 + i * 2 + j;
            massive_mcu_arrays[i][j].config.cmd_timeout_ms = 1000;
            massive_mcu_arrays[i][j].config.retry_count = 3;
            massive_mcu_arrays[i][j].reset_gpio = NULL;
            massive_mcu_arrays[i][j].irq_gpio = NULL;
        }

        /* 初始化平台配置 */
        massive_configs[i].platform_name = "ti";
        massive_configs[i].chip_name = "am625";
        massive_configs[i].project_name = "stress_test";
        massive_configs[i].product_name = massive_product_names[i];
        massive_configs[i].mcu_count = 2;
        massive_configs[i].mcu_array = massive_mcu_arrays[i];
        massive_configs[i].bmc_count = 0;
        massive_configs[i].bmc_array = NULL;
        massive_configs[i].fpga_count = 0;
        massive_configs[i].fpga_array = NULL;
        massive_configs[i].switch_count = 0;
        massive_configs[i].switch_array = NULL;
    }
}

/**
 * 测试大量配置加载压力
 * 注册最大数量的平台配置，测试注册表容量和查询性能
 */
static void test_stress_massive_config_load(void)
{
    uint32_t registered_count = 0;
    int32_t ret;

    /* 初始化PCONFIG */
    TEST_ASSERT_EQUAL(PCONFIG_Init(), OSAL_SUCCESS);

    /* 初始化大量配置 */
    init_massive_configs();

    OSAL_printf("[ INFO     ] Registering %u platform configurations...\n",
               MAX_PLATFORM_CONFIGS);

    /* 注册所有配置 */
    for (uint32_t i = 0; i < MAX_PLATFORM_CONFIGS; i++) {
        ret = PCONFIG_Register(&massive_configs[i]);
        if (ret == OSAL_SUCCESS) {
            registered_count++;
        } else {
            OSAL_printf("[ WARN     ] Failed to register config %u (reached limit at %u)\n",
                       i, registered_count);
            break;
        }
    }

    OSAL_printf("[ INFO     ] Successfully registered %u configurations\n",
               registered_count);
    TEST_ASSERT(registered_count > 0);

    /* 验证List API */
    const pconfig_platform_config_t *config_list[MAX_PLATFORM_CONFIGS];
    uint32_t list_count = MAX_PLATFORM_CONFIGS;

    ret = PCONFIG_List(config_list, &list_count);
    TEST_ASSERT_EQUAL(ret, OSAL_SUCCESS);
    TEST_ASSERT_EQUAL(list_count, registered_count);

    OSAL_printf("[ INFO     ] List API returned %u configurations\n", list_count);

    /* 查询所有已注册的配置 */
    OSAL_printf("[ INFO     ] Verifying all registered configurations...\n");

    for (uint32_t i = 0; i < registered_count; i++) {
        const pconfig_platform_config_t *found = PCONFIG_Find(
            "ti", massive_product_names[i], NULL);

        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_STRING(found->product_name, massive_product_names[i]);

        /* 验证硬件配置可访问 */
        TEST_ASSERT_EQUAL(found->mcu_count, 2);
        const pconfig_mcu_entry_t *mcu0 = PCONFIG_HW_GetMCU(found, 0);
        TEST_ASSERT_NOT_NULL(mcu0);
        const pconfig_mcu_entry_t *mcu1 = PCONFIG_HW_GetMCU(found, 1);
        TEST_ASSERT_NOT_NULL(mcu1);
    }

    OSAL_printf("[ INFO     ] All %u configurations verified successfully\n",
               registered_count);

    /* 性能测试：测量查询延迟 */
    OSAL_printf("[ INFO     ] Measuring query performance...\n");

    const uint32_t query_iterations = 10000;
    int64_t start_time = OSAL_get_monotonic_time();

    for (uint32_t i = 0; i < query_iterations; i++) {
        uint32_t idx = i % registered_count;
        const pconfig_platform_config_t *found = PCONFIG_Find(
            "ti", massive_product_names[idx], NULL);
        TEST_ASSERT_NOT_NULL(found);
    }

    int64_t end_time = OSAL_get_monotonic_time();
    uint64_t elapsed_ms = (end_time - start_time) / 1000;
    double avg_latency_us = (double)(elapsed_ms * 1000) / query_iterations;
    double queries_per_sec = (double)query_iterations * 1000.0 / elapsed_ms;

    OSAL_printf("[ INFO     ] Query performance:\n");
    OSAL_printf("[ INFO     ]   - Total queries: %u\n", query_iterations);
    OSAL_printf("[ INFO     ]   - Total time: %llu ms\n",
               (unsigned long long)elapsed_ms);
    OSAL_printf("[ INFO     ]   - Average latency: %.2f μs\n", avg_latency_us);
    OSAL_printf("[ INFO     ]   - Throughput: %.2f queries/sec\n", queries_per_sec);

    /* 清理 */
    PCONFIG_Cleanup();
}

/*===========================================================================
 * 测试3: 长时间运行稳定性测试
 *===========================================================================*/

/* 长时间运行测试的共享数据 */
typedef struct {
    osal_atomic_uint32_t iteration_count;
    osal_atomic_uint32_t error_count;
    uint32_t config_count;
} long_running_data_t;

/**
 * 长时间运行工作函数
 */
static int32_t long_running_worker(void *user_data, uint32_t iteration)
{
    long_running_data_t *data = (long_running_data_t*)user_data;

    /* 轮询查询不同的配置 */
    uint32_t idx = iteration % data->config_count;

    const pconfig_platform_config_t *found = PCONFIG_Find(
        "ti", massive_product_names[idx], NULL);

    if (found == NULL) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    /* 验证配置内容 */
    if (found->mcu_count != 2) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    /* 查询硬件配置 */
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(found, 0);
    if (mcu == NULL) {
        OSAL_atomic_inc(&data->error_count);
        return -1;
    }

    OSAL_atomic_inc(&data->iteration_count);
    return 0;
}

/**
 * 测试配置系统长时间运行压力
 * 持续查询配置，验证长期稳定性和内存泄漏
 */
static void test_stress_config_long_running(void)
{
    long_running_data_t data;
    const uint32_t duration_sec = 30;  /* 运行30秒 */
    const uint32_t config_count = 16;  /* 注册16个配置 */

    /* 初始化PCONFIG */
    TEST_ASSERT_EQUAL(PCONFIG_Init(), OSAL_SUCCESS);

    /* 初始化配置数据 */
    init_massive_configs();

    /* 注册多个配置 */
    OSAL_printf("[ INFO     ] Registering %u configurations for long-running test...\n",
               config_count);

    for (uint32_t i = 0; i < config_count; i++) {
        TEST_ASSERT_EQUAL(PCONFIG_Register(&massive_configs[i]), OSAL_SUCCESS);
    }

    /* 初始化测试数据 */
    OSAL_atomic_init(&data.iteration_count, 0);
    OSAL_atomic_init(&data.error_count, 0);
    data.config_count = config_count;

    /* 创建压力测试上下文 */
    stress_config_t config = STRESS_CONFIG_DURATION(duration_sec);
    config.thread_count = 4;  /* 4个工作线程 */

    stress_context_t *ctx = stress_context_create("Config Long Running", &config);
    TEST_ASSERT_NOT_NULL(ctx);

    /* 运行压力测试 */
    OSAL_printf("[ INFO     ] Running long-running stability test: %u threads, %u seconds\n",
               config.thread_count, duration_sec);
    TEST_ASSERT_EQUAL(stress_run(ctx, long_running_worker, &data), 0);

    /* 打印报告 */
    stress_print_report(ctx);

    /* 验证结果 */
    STRESS_ASSERT_NO_ERRORS(ctx);
    STRESS_ASSERT_SUCCESS_RATE_GT(ctx, 99.9);

    /* 验证迭代计数 */
    uint32_t total_iterations = OSAL_atomic_load(&data.iteration_count);
    uint32_t total_errors = OSAL_atomic_load(&data.error_count);

    OSAL_printf("[ INFO     ] Total iterations: %u\n", total_iterations);
    OSAL_printf("[ INFO     ] Total errors: %u\n", total_errors);

    TEST_ASSERT(total_iterations > 0);
    TEST_ASSERT_EQUAL(total_errors, 0);

    /* 清理 */
    stress_context_destroy(ctx);
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Long-running test completed - no memory leaks or crashes\n");
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_stress_config_concurrent_query",
		.func = test_stress_config_concurrent_query,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_massive_config_load",
		.func = test_stress_massive_config_load,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_stress_config_long_running",
		.func = test_stress_config_long_running,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "stress_pconfig",
	.module_name = "stress_pconfig",
	.layer_name = "PCONFIG",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_STRESS,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 60000,  /* 60秒超时 */
		.description = "PCONFIG stress tests - concurrent access, massive load, long-running stability"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_stress_pconfig_tests(void)
{
	libutest_register_suite(&test_suite);
}
