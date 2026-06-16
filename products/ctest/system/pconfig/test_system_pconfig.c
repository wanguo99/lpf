/**
 * @file test_system_pconfig.c
 * @brief PCONFIG layer system integration tests
 *
 * Tests:
 * - End-to-end configuration loading and validation
 * - Multi-platform configuration switching
 * - Configuration hot reload scenarios
 * - Integration with dependent modules
 * - Real-world usage scenarios
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_system.h>
#include "pconfig.h"
#include <pthread.h>

/*===========================================================================
 * Test Platform Configurations
 *===========================================================================*/

/* Test MCU configurations */
static pconfig_mcu_entry_t test_platform1_mcu[] = {
    {
        .description = "Platform1 MCU0 on CAN",
        .enabled = true,
        .config = {
            .name = "platform1_mcu0",
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
    },
    {
        .description = "Platform1 MCU1 on UART",
        .enabled = true,
        .config = {
            .name = "platform1_mcu1",
            .interface = PDL_MCU_INTERFACE_SERIAL,
            .hw.serial = {
                .device = "/dev/ttyS1",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
                .flow_control = PDL_MCU_FLOW_NONE
            },
            .cmd_timeout_ms = 500,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};

static pconfig_bmc_entry_t test_platform1_bmc[] = {
    {
        .description = "Platform1 BMC on Network+Serial",
        .enabled = true,
        .config = {
            .primary_channel = PDL_BMC_CHANNEL_NETWORK,
            .primary_config.network = {
                .ip_addr = "192.168.1.100",
                .port = 623,
                .username = "admin",
                .password = "test123",
                .timeout_ms = 2000
            },
            .backup_channel = PDL_BMC_CHANNEL_SERIAL,
            .backup_config.serial = {
                .device = "/dev/ttyS2",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
                .timeout_ms = 2000
            },
            .auto_switch = true,
            .retry_count = 3,
            .health_check_interval = 5000
        },
        .power_gpio = NULL,
        .reset_gpio = NULL
    }
};

static pconfig_fpga_cfg_t test_platform1_fpga[] = {
    {
        .description = "Platform1 FPGA",
        .enabled = true,
        .device = "/dev/fpga0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

static pconfig_switch_cfg_t test_platform1_switch[] = {
    {
        .description = "Platform1 Switch",
        .enabled = true,
        .device = "/dev/switch0",
        .cmd_timeout_ms = 500,
        .retry_count = 3
    }
};

static pconfig_platform_config_t test_platform1 = {
    .platform_name = "ti/am625",
    .chip_name = "am625",
    .project_name = "TEST_PLATFORM1",
    .product_name = "test_platform1",
    .mcu_count = sizeof(test_platform1_mcu) / sizeof(test_platform1_mcu[0]),
    .mcu_array = test_platform1_mcu,
    .bmc_count = sizeof(test_platform1_bmc) / sizeof(test_platform1_bmc[0]),
    .bmc_array = test_platform1_bmc,
    .fpga_count = sizeof(test_platform1_fpga) / sizeof(test_platform1_fpga[0]),
    .fpga_array = test_platform1_fpga,
    .switch_count = sizeof(test_platform1_switch) / sizeof(test_platform1_switch[0]),
    .switch_array = test_platform1_switch
};

/* Test platform 2 - Different configuration */
static pconfig_mcu_entry_t test_platform2_mcu[] = {
    {
        .description = "Platform2 MCU on CAN",
        .enabled = true,
        .config = {
            .name = "platform2_mcu0",
            .interface = PDL_MCU_INTERFACE_CAN,
            .hw.can = {
                .device = "can1",
                .bitrate = 1000000,
                .rx_timeout = 50,
                .tx_timeout = 50,
                .tx_id = 0x200,
                .rx_id = 0x201
            },
            .cmd_timeout_ms = 800,
            .retry_count = 5
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};

static pconfig_bmc_entry_t test_platform2_bmc[] = {
    {
        .description = "Platform2 BMC on Serial only",
        .enabled = true,
        .config = {
            .primary_channel = PDL_BMC_CHANNEL_SERIAL,
            .primary_config.serial = {
                .device = "/dev/ttyS3",
                .baudrate = 57600,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
                .timeout_ms = 1500
            },
            .backup_channel = PDL_BMC_CHANNEL_NETWORK,
            .backup_config = {{0}},
            .auto_switch = false,
            .retry_count = 5,
            .health_check_interval = 10000
        },
        .power_gpio = NULL,
        .reset_gpio = NULL
    }
};

static pconfig_platform_config_t test_platform2 = {
    .platform_name = "ti/am62b",
    .chip_name = "am62b",
    .project_name = "TEST_PLATFORM2",
    .product_name = "test_platform2",
    .mcu_count = sizeof(test_platform2_mcu) / sizeof(test_platform2_mcu[0]),
    .mcu_array = test_platform2_mcu,
    .bmc_count = sizeof(test_platform2_bmc) / sizeof(test_platform2_bmc[0]),
    .bmc_array = test_platform2_bmc,
    .fpga_count = 0,
    .fpga_array = NULL,
    .switch_count = 0,
    .switch_array = NULL
};

/* Invalid platform for testing validation */
static pconfig_platform_config_t test_platform_invalid = {
    .platform_name = NULL,  /* Invalid: missing platform name */
    .chip_name = "test_chip",
    .project_name = "TEST_INVALID",
    .product_name = NULL,   /* Invalid: missing product name */
    .mcu_count = 0,
    .mcu_array = NULL,
    .bmc_count = 0,
    .bmc_array = NULL,
    .fpga_count = 0,
    .fpga_array = NULL,
    .switch_count = 0,
    .switch_array = NULL
};

/*===========================================================================
 * Test: End-to-end configuration loading
 *===========================================================================*/

/**
 * Test complete configuration loading workflow:
 * 1. Initialize PCONFIG
 * 2. Register platform configurations
 * 3. Validate configurations
 * 4. Query and verify hardware configurations
 * 5. Cleanup
 */
static void test_system_config_end_to_end_load(void) {
    OSAL_printf("[ TEST     ] Configuration end-to-end load system test\n");
    int32_t ret;

    /* Phase 1: Initialize PCONFIG */
    OSAL_printf("[ PHASE 1  ] Initializing PCONFIG\n");
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 2: Register test platform 1 */
    OSAL_printf("[ PHASE 2  ] Registering platform 1\n");
    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 3: Validate registered configuration */
    OSAL_printf("[ PHASE 3  ] Validating platform 1 configuration\n");
    const pconfig_platform_config_t *board = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_NULL(board);

    ret = PCONFIG_Validate(board);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 4: Query and verify MCU configurations */
    OSAL_printf("[ PHASE 4  ] Querying MCU configurations\n");
    TEST_ASSERT_EQUAL(2, board->mcu_count);

    const pconfig_mcu_entry_t *mcu0 = PCONFIG_HW_GetMCU(board, 0);
    TEST_ASSERT_NOT_NULL(mcu0);
    TEST_ASSERT_TRUE(mcu0->enabled);
    TEST_ASSERT_EQUAL(PDL_MCU_INTERFACE_CAN, mcu0->config.interface);
    TEST_ASSERT_EQUAL(0x100, mcu0->config.hw.can.tx_id);

    const pconfig_mcu_entry_t *mcu1 = PCONFIG_HW_GetMCU(board, 1);
    TEST_ASSERT_NOT_NULL(mcu1);
    TEST_ASSERT_TRUE(mcu1->enabled);
    TEST_ASSERT_EQUAL(PDL_MCU_INTERFACE_SERIAL, mcu1->config.interface);
    TEST_ASSERT_EQUAL(115200, mcu1->config.hw.serial.baudrate);

    /* Phase 5: Query and verify BMC configurations */
    OSAL_printf("[ PHASE 5  ] Querying BMC configurations\n");
    TEST_ASSERT_EQUAL(1, board->bmc_count);

    const pconfig_bmc_entry_t *bmc0 = PCONFIG_HW_GetBMC(board, 0);
    TEST_ASSERT_NOT_NULL(bmc0);
    TEST_ASSERT_TRUE(bmc0->enabled);
    TEST_ASSERT_EQUAL(PDL_BMC_CHANNEL_NETWORK, bmc0->config.primary_channel);
    TEST_ASSERT_EQUAL(623, bmc0->config.primary_config.network.port);

    /* Phase 6: Query and verify FPGA configurations */
    OSAL_printf("[ PHASE 6  ] Querying FPGA configurations\n");
    TEST_ASSERT_EQUAL(1, board->fpga_count);

    const pconfig_fpga_cfg_t *fpga0 = PCONFIG_HW_GetFPGA(board, 0);
    TEST_ASSERT_NOT_NULL(fpga0);
    TEST_ASSERT_TRUE(fpga0->enabled);

    /* Phase 7: Query and verify Switch configurations */
    OSAL_printf("[ PHASE 7  ] Querying Switch configurations\n");
    TEST_ASSERT_EQUAL(1, board->switch_count);

    const pconfig_switch_cfg_t *switch0 = PCONFIG_HW_GetSwitch(board, 0);
    TEST_ASSERT_NOT_NULL(switch0);
    TEST_ASSERT_TRUE(switch0->enabled);

    /* Phase 8: Print configuration for debugging */
    OSAL_printf("[ PHASE 8  ] Printing configuration\n");
    PCONFIG_Print(board);

    /* Phase 9: Cleanup */
    OSAL_printf("[ PHASE 9  ] Cleaning up PCONFIG\n");
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Configuration end-to-end load test passed\n");
}

/*===========================================================================
 * Test: Multi-platform configuration switching
 *===========================================================================*/

/**
 * Test runtime platform switching:
 * 1. Register multiple platforms
 * 2. Query and list all platforms
 * 3. Switch between platforms
 * 4. Verify configuration isolation
 */
static void test_system_multi_platform_switch(void) {
    OSAL_printf("[ TEST     ] Multi-platform switch system test\n");
    int32_t ret;

    /* Phase 1: Initialize */
    OSAL_printf("[ PHASE 1  ] Initializing PCONFIG\n");
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 2: Register platform 1 */
    OSAL_printf("[ PHASE 2  ] Registering platform 1 (ti/am625)\n");
    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Verify platform 1 is active */
    const pconfig_platform_config_t *board = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_NULL(board);
    TEST_ASSERT_STRING_EQUAL("ti/am625", board->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_platform1", board->product_name);
    TEST_ASSERT_EQUAL(2, board->mcu_count);

    /* Phase 3: Register platform 2 */
    OSAL_printf("[ PHASE 3  ] Registering platform 2 (ti/am62b)\n");
    ret = PCONFIG_Register(&test_platform2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Verify platform 2 is now active */
    board = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_NULL(board);
    TEST_ASSERT_STRING_EQUAL("ti/am62b", board->platform_name);
    TEST_ASSERT_STRING_EQUAL("test_platform2", board->product_name);
    TEST_ASSERT_EQUAL(1, board->mcu_count);

    /* Phase 4: List all registered platforms */
    OSAL_printf("[ PHASE 4  ] Listing all registered platforms\n");
    const pconfig_platform_config_t *configs[10];
    uint32_t count = 10;
    ret = PCONFIG_List(configs, &count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(2, count);

    OSAL_printf("[ INFO     ] Found %u registered platforms\n", count);
    uint32_t i;
    for (i = 0; i < count; i++) {
        OSAL_printf("[ INFO     ]   [%u] %s - %s\n",
                   i, configs[i]->platform_name, configs[i]->product_name);
    }

    /* Phase 5: Find platform 1 by name */
    OSAL_printf("[ PHASE 5  ] Finding platform 1 by name\n");
    const pconfig_platform_config_t *found = PCONFIG_Find("ti/am625", "test_platform1", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("ti/am625", found->platform_name);
    TEST_ASSERT_EQUAL(2, found->mcu_count);

    /* Phase 6: Find platform 2 by name */
    OSAL_printf("[ PHASE 6  ] Finding platform 2 by name\n");
    found = PCONFIG_Find("ti/am62b", "test_platform2", NULL);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_STRING_EQUAL("ti/am62b", found->platform_name);
    TEST_ASSERT_EQUAL(1, found->mcu_count);

    /* Phase 7: Verify configuration isolation */
    OSAL_printf("[ PHASE 7  ] Verifying configuration isolation\n");
    const pconfig_platform_config_t *p1 = PCONFIG_Find("ti/am625", "test_platform1", NULL);
    const pconfig_platform_config_t *p2 = PCONFIG_Find("ti/am62b", "test_platform2", NULL);

    TEST_ASSERT_NOT_EQUAL((uintptr_t)p1, (uintptr_t)p2);
    TEST_ASSERT_NOT_EQUAL(p1->mcu_count, p2->mcu_count);

    /* Phase 8: Cleanup */
    OSAL_printf("[ PHASE 8  ] Cleaning up PCONFIG\n");
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Multi-platform switch test passed\n");
}

/*===========================================================================
 * Test: Configuration hot reload
 *===========================================================================*/

/**
 * Test configuration hot reload scenario:
 * 1. Register initial configuration
 * 2. Simulate configuration change
 * 3. Re-register updated configuration
 * 4. Verify changes are reflected
 */
static void test_system_config_hot_reload(void) {
    OSAL_printf("[ TEST     ] Configuration hot reload system test\n");
    int32_t ret;

    /* Phase 1: Initialize and register initial config */
    OSAL_printf("[ PHASE 1  ] Registering initial configuration\n");
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    const pconfig_platform_config_t *board = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_NULL(board);
    TEST_ASSERT_EQUAL(2, board->mcu_count);

    /* Phase 2: Simulate configuration change - disable MCU1 */
    OSAL_printf("[ PHASE 2  ] Simulating configuration change (disable MCU1)\n");
    test_platform1_mcu[1].enabled = false;

    /* Phase 3: Re-register updated configuration */
    OSAL_printf("[ PHASE 3  ] Re-registering updated configuration\n");
    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 4: Verify changes are reflected */
    OSAL_printf("[ PHASE 4  ] Verifying configuration changes\n");
    board = PCONFIG_GetBoard();
    TEST_ASSERT_NOT_NULL(board);

    const pconfig_mcu_entry_t *mcu1 = PCONFIG_HW_GetMCU(board, 1);
    TEST_ASSERT_NOT_NULL(mcu1);
    TEST_ASSERT_FALSE(mcu1->enabled);
    OSAL_printf("[ INFO     ] MCU1 enabled status: %s\n",
               mcu1->enabled ? "true" : "false");

    /* Phase 5: Re-enable MCU1 and reload */
    OSAL_printf("[ PHASE 5  ] Re-enabling MCU1 and reloading\n");
    test_platform1_mcu[1].enabled = true;

    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    board = PCONFIG_GetBoard();
    mcu1 = PCONFIG_HW_GetMCU(board, 1);
    TEST_ASSERT_NOT_NULL(mcu1);
    TEST_ASSERT_TRUE(mcu1->enabled);

    /* Phase 6: Cleanup */
    OSAL_printf("[ PHASE 6  ] Cleaning up PCONFIG\n");
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Configuration hot reload test passed\n");
}

/*===========================================================================
 * Test: Configuration validation scenarios
 *===========================================================================*/

/**
 * Test configuration validation edge cases:
 * 1. Valid configuration validation
 * 2. Invalid configuration rejection
 * 3. Boundary conditions
 */
static void test_system_config_validation_scenarios(void) {
    OSAL_printf("[ TEST     ] Configuration validation scenarios\n");
    int32_t ret;

    /* Phase 1: Initialize */
    OSAL_printf("[ PHASE 1  ] Initializing PCONFIG\n");
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 2: Validate valid configuration */
    OSAL_printf("[ PHASE 2  ] Validating valid configuration\n");
    ret = PCONFIG_Validate(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 3: Validate invalid configuration (missing platform name) */
    OSAL_printf("[ PHASE 3  ] Validating invalid configuration\n");
    ret = PCONFIG_Validate(&test_platform_invalid);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 4: Validate NULL configuration */
    OSAL_printf("[ PHASE 4  ] Validating NULL configuration\n");
    ret = PCONFIG_Validate(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 5: Register invalid configuration should fail */
    OSAL_printf("[ PHASE 5  ] Attempting to register invalid configuration\n");
    ret = PCONFIG_Register(&test_platform_invalid);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 6: Cleanup */
    OSAL_printf("[ PHASE 6  ] Cleaning up PCONFIG\n");
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Configuration validation scenarios test passed\n");
}

/*===========================================================================
 * Test: Concurrent configuration queries
 *===========================================================================*/

typedef struct {
    const pconfig_platform_config_t *board;
    uint32_t query_count;
    uint32_t success_count;
    pthread_mutex_t mutex;
} query_thread_ctx_t;

static void* config_query_thread(void *arg) {
    query_thread_ctx_t *ctx = (query_thread_ctx_t*)arg;
    uint32_t i;

    for (i = 0; i < ctx->query_count; i++) {
        /* Query board configuration */
        const pconfig_platform_config_t *board = PCONFIG_GetBoard();
        if (board) {
            /* Query MCU configurations */
            const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(board, 0);
            if (mcu && mcu->enabled) {
                pthread_mutex_lock(&ctx->mutex);
                ctx->success_count++;
                pthread_mutex_unlock(&ctx->mutex);
            }
        }

        /* Small delay to allow interleaving */
        OSAL_usleep(100);
    }

    return NULL;
}

/**
 * Test concurrent configuration queries:
 * 1. Register configuration
 * 2. Spawn multiple query threads
 * 3. Verify thread safety and consistency
 */
static void test_system_concurrent_config_queries(void) {
    OSAL_printf("[ TEST     ] Concurrent configuration queries\n");
    int32_t ret;

    /* Phase 1: Initialize and register */
    OSAL_printf("[ PHASE 1  ] Initializing PCONFIG and registering platform\n");
    ret = PCONFIG_Init();
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = PCONFIG_Register(&test_platform1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* Phase 2: Setup query context */
    OSAL_printf("[ PHASE 2  ] Setting up concurrent query context\n");
    query_thread_ctx_t ctx;
    ctx.board = PCONFIG_GetBoard();
    ctx.query_count = 1000;
    ctx.success_count = 0;
    pthread_mutex_init(&ctx.mutex, NULL);

    /* Phase 3: Spawn query threads */
    OSAL_printf("[ PHASE 3  ] Spawning 4 query threads\n");
    pthread_t threads[4];
    uint32_t i;
    for (i = 0; i < 4; i++) {
        ret = pthread_create(&threads[i], NULL, config_query_thread, &ctx);
        TEST_ASSERT_EQUAL(0, ret);
    }

    /* Phase 4: Wait for threads */
    OSAL_printf("[ PHASE 4  ] Waiting for query threads to complete\n");
    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Phase 5: Verify results */
    OSAL_printf("[ PHASE 5  ] Verifying query results\n");
    OSAL_printf("[ INFO     ] Total queries: %u, Successful: %u\n",
               ctx.query_count * 4, ctx.success_count);
    TEST_ASSERT_EQUAL(ctx.query_count * 4, ctx.success_count);

    /* Phase 6: Cleanup */
    pthread_mutex_destroy(&ctx.mutex);
    PCONFIG_Cleanup();

    OSAL_printf("[ PASS     ] Concurrent configuration queries test passed\n");
}

/*===========================================================================
 * Test: Configuration lifecycle stress test
 *===========================================================================*/

/**
 * Test configuration lifecycle under stress:
 * 1. Repeated init/cleanup cycles
 * 2. Multiple register/query cycles
 * 3. Memory leak detection
 */
static void test_system_config_lifecycle_stress(void) {
    OSAL_printf("[ TEST     ] Configuration lifecycle stress test\n");
    int32_t ret;
    uint32_t cycle;

    for (cycle = 0; cycle < 10; cycle++) {
        OSAL_printf("[ CYCLE %2u ] Starting lifecycle cycle\n", cycle);

        /* Initialize */
        ret = PCONFIG_Init();
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        /* Register multiple platforms */
        ret = PCONFIG_Register(&test_platform1);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        ret = PCONFIG_Register(&test_platform2);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        /* Query configurations */
        const pconfig_platform_config_t *board = PCONFIG_GetBoard();
        TEST_ASSERT_NOT_NULL(board);

        const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(board, 0);
        TEST_ASSERT_NOT_NULL(mcu);

        /* List platforms */
        const pconfig_platform_config_t *configs[10];
        uint32_t count = 10;
        ret = PCONFIG_List(configs, &count);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

        /* Cleanup */
        PCONFIG_Cleanup();

        OSAL_printf("[ CYCLE %2u ] Completed successfully\n", cycle);
    }

    OSAL_printf("[ PASS     ] Configuration lifecycle stress test passed\n");
}

/*===========================================================================
 * Test Suite Registration
 *===========================================================================*/

/* Test cases array */
static const test_case_t test_cases[] = {
    {
        .name = "test_system_config_end_to_end_load",
        .func = test_system_config_end_to_end_load,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_system_multi_platform_switch",
        .func = test_system_multi_platform_switch,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_system_config_hot_reload",
        .func = test_system_config_hot_reload,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_system_config_validation_scenarios",
        .func = test_system_config_validation_scenarios,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_system_concurrent_config_queries",
        .func = test_system_concurrent_config_queries,
        .setup = NULL,
        .teardown = NULL
    },
    {
        .name = "test_system_config_lifecycle_stress",
        .func = test_system_config_lifecycle_stress,
        .setup = NULL,
        .teardown = NULL
    },
};

/* Test suite definition */
static const test_suite_t test_suite = {
    .suite_name = "system_pconfig",
    .module_name = "system_pconfig",
    .layer_name = "PCONFIG",
    .cases = test_cases,
    .case_count = sizeof(test_cases) / sizeof(test_case_t),
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .metadata = {
        .category = TEST_CATEGORY_SYSTEM,
        .tags = TEST_TAG_SLOW | TEST_TAG_HARDWARE,
        .timeout_ms = 15000,
        .description = "PCONFIG system integration tests"
    }
};

/* Test suite registration function */
__attribute__((constructor))
static void register_system_pconfig_tests(void)
{
    libutest_register_suite(&test_suite);
}
