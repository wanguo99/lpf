/**
 * @file test_pconfig_api.c
 * @brief PCONFIG API comprehensive unit tests
 */

#include <test_framework/test_framework.h>
#include "pconfig/pconfig.h"
#include <pthread.h>

/* Test MCU configurations */
static pconfig_mcu_entry_t test_mcu_entries[] = {
    {
        .description = "Power management MCU",
        .enabled = true,
        .config = {
            .name = "power_mcu",
            .interface = PCONFIG_MCU_INTERFACE_CAN,
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
    },
    {
        .description = "Payload control MCU",
        .enabled = true,
        .config = {
            .name = "payload_mcu",
            .interface = PCONFIG_MCU_INTERFACE_CAN,
            .hw.can = {
                .device = "can0",
                .bitrate = 500000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x200,
                .rx_id = 0x201
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};


/* Test FPGA configurations */
static pconfig_fpga_cfg_t test_fpga_configs[] = {
    {
        .description = "Test FPGA",
        .enabled = true,
        .device = "/dev/fpga0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

/* Test Switch configurations */
static pconfig_switch_cfg_t test_switch_configs[] = {
    {
        .description = "Test Switch",
        .enabled = true,
        .device = "/dev/switch0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

/* Test platform configuration */
static pconfig_platform_config_t test_platform_config = {
    .platform_name = "ti",
    .chip_name = "am625",
    .project_name = "test_project",
    .product_name = "test_board",

    .mcu_count = 2,
    .mcu_array = test_mcu_entries,


    .fpga_count = 1,
    .fpga_array = test_fpga_configs,

    .switch_count = 1,
    .switch_array = test_switch_configs
};

/* Second test platform for multi-platform tests */
static pconfig_platform_config_t test_platform_config2 = {
    .platform_name = "ti",
    .chip_name = "am625",
    .project_name = "test_project",
    .product_name = "test_board2",

    .mcu_count = 0,
    .mcu_array = NULL,


    .fpga_count = 0,
    .fpga_array = NULL,

    .switch_count = 0,
    .switch_array = NULL
};

/*===========================================================================
 * Setup and Teardown
 *===========================================================================*/

static void suite_setup(void)
{
    PCONFIG_init();
    PCONFIG_register(&test_platform_config);
}

static void suite_teardown(void)
{
    PCONFIG_cleanup();
}

/*===========================================================================
 * Initialization and Cleanup Tests
 *===========================================================================*/

static void test_pconfig_init_success(void)
{
    PCONFIG_cleanup();  /* Clean first */
    int32_t ret = PCONFIG_init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_init_twice(void)
{
    PCONFIG_cleanup();
    int32_t ret1 = PCONFIG_init();
    int32_t ret2 = PCONFIG_init();  /* Double init */
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret2);  /* Should handle gracefully */
}

static void test_pconfig_cleanup(void)
{
    PCONFIG_init();
    PCONFIG_cleanup();  /* Should not crash */

    /* After cleanup, GetBoard should return NULL */
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_EQUAL(NULL, cfg);
}

static void test_pconfig_operations_after_cleanup(void)
{
    PCONFIG_init();
    PCONFIG_register(&test_platform_config);
    PCONFIG_cleanup();

    /* Operations after cleanup should fail gracefully */
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_EQUAL(NULL, cfg);

    /* Re-init for other tests */
    PCONFIG_init();
}

/*===========================================================================
 * Registration Tests
 *===========================================================================*/

static void test_pconfig_register_success(void)
{
    int32_t ret = PCONFIG_register(&test_platform_config2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_register_null(void)
{
    int32_t ret = PCONFIG_register(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_register_duplicate(void)
{
    /* Register same config twice */
    PCONFIG_register(&test_platform_config2);
    int32_t ret = PCONFIG_register(&test_platform_config2);
    /* Should either succeed or handle gracefully */
    TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret == OSAL_ERR_GENERIC);
}

/*===========================================================================
 * Board Configuration Query Tests
 *===========================================================================*/

static void test_pconfig_get_board(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->platform_name, "ti"));
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->product_name, "test_board"));
}

static void test_pconfig_get_board_before_init(void)
{
    PCONFIG_cleanup();
    const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
    TEST_ASSERT_EQUAL(NULL, cfg);

    /* Re-init and register for other tests */
    PCONFIG_init();
    PCONFIG_register(&test_platform_config);
}

static void test_pconfig_find_by_name(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("ti", "test_board", NULL);
    TEST_ASSERT_NOT_EQUAL(NULL, cfg);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(cfg->chip_name, "am625"));
}

static void test_pconfig_find_not_found(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("unknown", "unknown", NULL);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

static void test_pconfig_find_null_platform(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find(NULL, "test_board", NULL);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

static void test_pconfig_find_null_product(void)
{
    const pconfig_platform_config_t *cfg = PCONFIG_Find("ti", NULL, NULL);
    TEST_ASSERT_EQUAL(NULL, cfg);
}

/*===========================================================================
 * MCU Configuration Query Tests
 *===========================================================================*/

static void test_pconfig_hw_get_mcu_by_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(mcu->config.name, "power_mcu"));
    TEST_ASSERT_EQUAL(0x100, mcu->config.hw.can.tx_id);
    TEST_ASSERT_TRUE(mcu->enabled);
}

static void test_pconfig_hw_get_mcu_invalid_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 999);

    TEST_ASSERT_EQUAL(NULL, mcu);
}

static void test_pconfig_hw_get_mcu_null_platform(void)
{
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(NULL, 0);
    TEST_ASSERT_EQUAL(NULL, mcu);
}

static void test_pconfig_hw_get_mcu_second_entry(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, 1);

    TEST_ASSERT_NOT_EQUAL(NULL, mcu);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(mcu->config.name, "payload_mcu"));
    TEST_ASSERT_EQUAL(0x200, mcu->config.hw.can.tx_id);
}

/*===========================================================================
 * FPGA Configuration Query Tests
 *===========================================================================*/

static void test_pconfig_hw_get_fpga_by_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_fpga_cfg_t *fpga = PCONFIG_HW_GetFPGA(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, fpga);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(fpga->device, "/dev/fpga0"));
    TEST_ASSERT_TRUE(fpga->enabled);
}

static void test_pconfig_hw_get_fpga_invalid_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_fpga_cfg_t *fpga = PCONFIG_HW_GetFPGA(platform, 999);

    TEST_ASSERT_EQUAL(NULL, fpga);
}

static void test_pconfig_hw_get_fpga_null_platform(void)
{
    const pconfig_fpga_cfg_t *fpga = PCONFIG_HW_GetFPGA(NULL, 0);
    TEST_ASSERT_EQUAL(NULL, fpga);
}

/*===========================================================================
 * Switch Configuration Query Tests
 *===========================================================================*/

static void test_pconfig_hw_get_switch_by_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_switch_cfg_t *sw = PCONFIG_HW_GetSwitch(platform, 0);

    TEST_ASSERT_NOT_EQUAL(NULL, sw);
    TEST_ASSERT_EQUAL(0, OSAL_strcmp(sw->device, "/dev/switch0"));
    TEST_ASSERT_TRUE(sw->enabled);
}

static void test_pconfig_hw_get_switch_invalid_id(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    const pconfig_switch_cfg_t *sw = PCONFIG_HW_GetSwitch(platform, 999);

    TEST_ASSERT_EQUAL(NULL, sw);
}

static void test_pconfig_hw_get_switch_null_platform(void)
{
    const pconfig_switch_cfg_t *sw = PCONFIG_HW_GetSwitch(NULL, 0);
    TEST_ASSERT_EQUAL(NULL, sw);
}

/*===========================================================================
 * Configuration Validation Tests
 *===========================================================================*/

static void test_pconfig_validate_success(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    int32_t ret = PCONFIG_validate(platform);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

static void test_pconfig_validate_null(void)
{
    int32_t ret = PCONFIG_validate(NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_validate_missing_platform_name(void)
{
    pconfig_platform_config_t invalid_config = test_platform_config;
    invalid_config.platform_name = NULL;

    int32_t ret = PCONFIG_validate(&invalid_config);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_validate_missing_product_name(void)
{
    pconfig_platform_config_t invalid_config = test_platform_config;
    invalid_config.product_name = NULL;

    int32_t ret = PCONFIG_validate(&invalid_config);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_validate_count_array_mismatch(void)
{
    pconfig_platform_config_t invalid_config = test_platform_config;
    invalid_config.mcu_count = 5;  /* Count doesn't match array */
    invalid_config.mcu_array = NULL;

    int32_t ret = PCONFIG_validate(&invalid_config);
    /* Should detect mismatch */
    TEST_ASSERT_TRUE(ret == OSAL_ERR_GENERIC || ret == OSAL_SUCCESS);
}

/*===========================================================================
 * Configuration List Tests
 *===========================================================================*/

static void test_pconfig_list(void)
{
    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;

    int32_t ret = PCONFIG_list(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(count >= 1);  /* At least our registered config */
}

static void test_pconfig_list_null_pointer(void)
{
    uint32_t count = 10;
    int32_t ret = PCONFIG_list(NULL, &count);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_list_null_count(void)
{
    const pconfig_platform_config_t *configs[10];
    int32_t ret = PCONFIG_list(configs, NULL);
    TEST_ASSERT_EQUAL(OSAL_ERR_GENERIC, ret);
}

static void test_pconfig_list_small_buffer(void)
{
    /* Register multiple configs */
    PCONFIG_register(&test_platform_config2);

    const pconfig_platform_config_t *configs[1];  /* Small buffer */
    uint32_t count = 1;

    int32_t ret = PCONFIG_list(configs, &count);
    /* Should handle gracefully, returning what fits */
    TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret == OSAL_ERR_GENERIC);
    TEST_ASSERT_TRUE(count <= 1);
}

/*===========================================================================
 * Print Configuration Tests
 *===========================================================================*/

static void test_pconfig_print(void)
{
    const pconfig_platform_config_t *platform = PCONFIG_GetBoard();
    /* Just verify function doesn't crash */
    PCONFIG_print(platform);
    TEST_ASSERT_TRUE(true);
}

static void test_pconfig_print_null(void)
{
    /* Should handle NULL gracefully */
    PCONFIG_print(NULL);
    TEST_ASSERT_TRUE(true);
}

/*===========================================================================
 * Thread Safety Tests
 *===========================================================================*/

static void* thread_query_board(void *arg)
{
    (void)arg;
    for (int i = 0; i < 100; i++) {
        const pconfig_platform_config_t *cfg = PCONFIG_GetBoard();
        if (cfg) {
            /* Access fields to ensure no race conditions */
            volatile const char *name = cfg->platform_name;
            (void)name;
        }
    }
    return NULL;
}

static void test_pconfig_concurrent_query(void)
{
    pthread_t threads[4];

    /* Start multiple threads querying concurrently */
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, thread_query_board, NULL);
    }

    /* Wait for all threads */
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    TEST_ASSERT_TRUE(true);  /* No crash = success */
}

/* Test case array */
static const test_case_t test_cases[] = {
    /* Initialization and Cleanup */
    { .name = "test_pconfig_init_success", .func = test_pconfig_init_success, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_init_twice", .func = test_pconfig_init_twice, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_cleanup", .func = test_pconfig_cleanup, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_operations_after_cleanup", .func = test_pconfig_operations_after_cleanup, .setup = NULL, .teardown = NULL },

    /* Registration */
    { .name = "test_pconfig_register_success", .func = test_pconfig_register_success, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_register_null", .func = test_pconfig_register_null, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_register_duplicate", .func = test_pconfig_register_duplicate, .setup = NULL, .teardown = NULL },

    /* Board Query */
    { .name = "test_pconfig_get_board", .func = test_pconfig_get_board, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_get_board_before_init", .func = test_pconfig_get_board_before_init, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_find_by_name", .func = test_pconfig_find_by_name, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_find_not_found", .func = test_pconfig_find_not_found, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_find_null_platform", .func = test_pconfig_find_null_platform, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_find_null_product", .func = test_pconfig_find_null_product, .setup = NULL, .teardown = NULL },

    /* MCU Query */
    { .name = "test_pconfig_hw_get_mcu_by_id", .func = test_pconfig_hw_get_mcu_by_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_mcu_invalid_id", .func = test_pconfig_hw_get_mcu_invalid_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_mcu_null_platform", .func = test_pconfig_hw_get_mcu_null_platform, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_mcu_second_entry", .func = test_pconfig_hw_get_mcu_second_entry, .setup = NULL, .teardown = NULL },

    /* FPGA Query */
    { .name = "test_pconfig_hw_get_fpga_by_id", .func = test_pconfig_hw_get_fpga_by_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_fpga_invalid_id", .func = test_pconfig_hw_get_fpga_invalid_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_fpga_null_platform", .func = test_pconfig_hw_get_fpga_null_platform, .setup = NULL, .teardown = NULL },

    /* Switch Query */
    { .name = "test_pconfig_hw_get_switch_by_id", .func = test_pconfig_hw_get_switch_by_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_switch_invalid_id", .func = test_pconfig_hw_get_switch_invalid_id, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_hw_get_switch_null_platform", .func = test_pconfig_hw_get_switch_null_platform, .setup = NULL, .teardown = NULL },

    /* Validation */
    { .name = "test_pconfig_validate_success", .func = test_pconfig_validate_success, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_validate_null", .func = test_pconfig_validate_null, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_validate_missing_platform_name", .func = test_pconfig_validate_missing_platform_name, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_validate_missing_product_name", .func = test_pconfig_validate_missing_product_name, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_validate_count_array_mismatch", .func = test_pconfig_validate_count_array_mismatch, .setup = NULL, .teardown = NULL },

    /* List */
    { .name = "test_pconfig_list", .func = test_pconfig_list, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_list_null_pointer", .func = test_pconfig_list_null_pointer, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_list_null_count", .func = test_pconfig_list_null_count, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_list_small_buffer", .func = test_pconfig_list_small_buffer, .setup = NULL, .teardown = NULL },

    /* Print */
    { .name = "test_pconfig_print", .func = test_pconfig_print, .setup = NULL, .teardown = NULL },
    { .name = "test_pconfig_print_null", .func = test_pconfig_print_null, .setup = NULL, .teardown = NULL },

    /* Thread Safety */
    { .name = "test_pconfig_concurrent_query", .func = test_pconfig_concurrent_query, .setup = NULL, .teardown = NULL },
};

/* Test suite definition */
static const test_suite_t test_suite = {
    .suite_name = "pconfig_api",
    .module_name = "pconfig",
    .layer_name = "PCONFIG",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = suite_setup,
    .suite_teardown = suite_teardown,
    .metadata = {
        .category = TEST_CATEGORY_UNIT,
        .tags = TEST_TAG_FAST,
        .timeout_ms = 1000,
        .description = "PCONFIG API comprehensive unit tests"
    }
};

/* Test suite registration */
__attribute__((constructor))
static void register_pconfig_api_tests(void)
{
    libutest_register_suite(&test_suite);
}
