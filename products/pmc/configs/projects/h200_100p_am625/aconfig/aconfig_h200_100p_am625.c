/**
 * @file aconfig_h200_100p_am625.c
 * @brief PMC H200-100P-AM625 product ACONFIG table.
 */

#include <stddef.h>

#include "aconfig.h"
#include "pmc_aconfig_types.h"
#include "pmc_tc_functions.h"
#include "pmc_tm_functions.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static const uint32_t tc_power_on_affects[] = {
	PMC_TM_POWER_STATUS,
};

static const uint32_t tc_power_off_affects[] = {
	PMC_TM_POWER_STATUS,
	PMC_TM_CPU_TEMP,
};

static const uint32_t tc_mcu_reset_affects[] = {
	PMC_TM_MCU_STATUS,
	PMC_TM_MCU_UPTIME,
};

static const pmc_tc_entry_t g_h200_tc_entries[] = {
	{
		.function_id = PMC_TC_POWER_ON,
		.config = {
			.function_id = PMC_TC_POWER_ON,
			.device = { .type = PMC_DEVICE_MCU, .index = 0 },
			.invalidated_tm_ids = tc_power_on_affects,
			.invalidated_tm_count = ARRAY_SIZE(tc_power_on_affects),
			.enabled = true,
			.user_context = NULL,
		},
	},
	{
		.function_id = PMC_TC_POWER_OFF,
		.config = {
			.function_id = PMC_TC_POWER_OFF,
			.device = { .type = PMC_DEVICE_MCU, .index = 1 },
			.invalidated_tm_ids = tc_power_off_affects,
			.invalidated_tm_count = ARRAY_SIZE(tc_power_off_affects),
			.enabled = true,
			.user_context = NULL,
		},
	},
	{
		.function_id = PMC_TC_MCU_RESET,
		.config = {
			.function_id = PMC_TC_MCU_RESET,
			.device = { .type = PMC_DEVICE_MCU, .index = 0 },
			.invalidated_tm_ids = tc_mcu_reset_affects,
			.invalidated_tm_count = ARRAY_SIZE(tc_mcu_reset_affects),
			.enabled = true,
			.user_context = NULL,
		},
	},
};

static const pmc_tm_entry_t g_h200_tm_entries[] = {
	{
		.function_id = PMC_TM_CPU_TEMP,
		.config = {
			.function_id = PMC_TM_CPU_TEMP,
			.device = { .type = PMC_DEVICE_MCU, .index = 0 },
			.poll_period_ms = 1000,
			.validity_period_ms = 2000,
			.enabled = true,
			.user_context = NULL,
		},
	},
	{
		.function_id = PMC_TM_POWER_STATUS,
		.config = {
			.function_id = PMC_TM_POWER_STATUS,
			.device = { .type = PMC_DEVICE_MCU, .index = 1 },
			.poll_period_ms = 500,
			.validity_period_ms = 1000,
			.enabled = true,
			.user_context = NULL,
		},
	},
	{
		.function_id = PMC_TM_MCU_STATUS,
		.config = {
			.function_id = PMC_TM_MCU_STATUS,
			.device = { .type = PMC_DEVICE_MCU, .index = 0 },
			.poll_period_ms = 500,
			.validity_period_ms = 1000,
			.enabled = true,
			.user_context = NULL,
		},
	},
	{
		.function_id = PMC_TM_MCU_UPTIME,
		.config = {
			.function_id = PMC_TM_MCU_UPTIME,
			.device = { .type = PMC_DEVICE_MCU, .index = 0 },
			.poll_period_ms = 10000,
			.validity_period_ms = 20000,
			.enabled = true,
			.user_context = NULL,
		},
	},
};

static const pmc_aconfig_function_map_t g_h200_function_map = {
	.tc_entries = g_h200_tc_entries,
	.tc_count = ARRAY_SIZE(g_h200_tc_entries),
	.tm_entries = g_h200_tm_entries,
	.tm_count = ARRAY_SIZE(g_h200_tm_entries),
};

const aconfig_config_table_t aconfig_h200_100p_am625 = {
	.name = "h200_100p_am625",
	.function_map = (aconfig_function_map_t *)&g_h200_function_map,
	.user_data = NULL,
};
