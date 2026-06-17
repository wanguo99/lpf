/**
 * @file pmc_runtime.c
 * @brief PMC product runtime initialization.
 */

#include "osal.h"
#include "pmc_runtime.h"

#ifdef CONFIG_PCONFIG
#include "pconfig.h"
#endif

#ifdef CONFIG_ACONFIG
#include "aconfig.h"
#endif

#ifdef CONFIG_PCONFIG
int32_t PMC_Runtime_RegisterPConfig(void);
#endif

#ifdef CONFIG_ACONFIG
int32_t PMC_Runtime_RegisterAConfig(void);
#endif

static bool g_runtime_initialized = false;

int32_t PMC_Runtime_Init(void)
{
	int32_t ret;

	if (g_runtime_initialized) {
		return OSAL_SUCCESS;
	}

#ifdef CONFIG_PCONFIG
	ret = PCONFIG_init();
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("PMC_RUNTIME", "PCONFIG_init failed: %d", ret);
		return ret;
	}

	ret = PMC_Runtime_RegisterPConfig();
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("PMC_RUNTIME", "PCONFIG registration failed: %d", ret);
		return ret;
	}
#endif

#ifdef CONFIG_ACONFIG
	ret = ACONFIG_init();
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("PMC_RUNTIME", "ACONFIG_init failed: %d", ret);
		return ret;
	}

	ret = PMC_Runtime_RegisterAConfig();
	if (OSAL_SUCCESS != ret) {
		LOG_ERROR("PMC_RUNTIME", "ACONFIG registration failed: %d", ret);
		return ret;
	}
#endif

	g_runtime_initialized = true;
	LOG_INFO("PMC_RUNTIME", "PMC runtime initialized");
	return OSAL_SUCCESS;
}

void PMC_Runtime_Cleanup(void)
{
	if (!g_runtime_initialized) {
		return;
	}

#ifdef CONFIG_ACONFIG
	ACONFIG_cleanup();
#endif

#ifdef CONFIG_PCONFIG
	PCONFIG_cleanup();
#endif

	g_runtime_initialized = false;
	LOG_INFO("PMC_RUNTIME", "PMC runtime cleaned up");
}
