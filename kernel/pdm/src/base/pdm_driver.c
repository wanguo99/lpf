// SPDX-License-Identifier: GPL-2.0

#include "pdm_driver.h"

#include "lpf/lpf_errno.h"
#include "lpf/lpf_led_service.h"
#include "lpf/lpf_mcu_service.h"

static bool g_pdm_drivers_ready;

int32_t pdm_register_builtin_drivers(void)
{
	int32_t ret;

#ifdef CONFIG_LPF_MCU_SERVICE
	ret = lpf_mcu_service_register();
	if (ret != OSAL_SUCCESS)
		return ret;
#endif

#ifdef CONFIG_LPF_LED_SERVICE
	ret = lpf_led_service_register();
	if (ret != OSAL_SUCCESS) {
#ifdef CONFIG_LPF_MCU_SERVICE
		lpf_mcu_service_unregister();
#endif
		return ret;
	}
#endif

	return OSAL_SUCCESS;
}

void pdm_unregister_builtin_drivers(void)
{
#ifdef CONFIG_LPF_LED_SERVICE
	lpf_led_service_unregister();
#endif
#ifdef CONFIG_LPF_MCU_SERVICE
	lpf_mcu_service_unregister();
#endif
}

int32_t pdm_driver_registry_init(void)
{
	return lpf_core_init();
}

void pdm_driver_registry_deinit(void)
{
}

int pdm_drivers_init(void)
{
	int32_t ret;

	ret = pdm_register_builtin_drivers();
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	g_pdm_drivers_ready = true;
	return 0;
}

void pdm_drivers_exit(void)
{
	if (!g_pdm_drivers_ready)
		return;

	lpf_device_unregister_all();
	pdm_unregister_builtin_drivers();
	g_pdm_drivers_ready = false;
}
