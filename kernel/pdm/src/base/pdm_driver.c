// SPDX-License-Identifier: GPL-2.0

#include "pdm_driver.h"
#include "pdm_status.h"

static bool g_pdm_drivers_ready;

#ifdef CONFIG_PDM_MCU_SUPPORT
int32_t pdm_mcu_driver_register(void);
void pdm_mcu_driver_unregister(void);
#endif

#ifdef CONFIG_PDM_LED_SUPPORT
int32_t pdm_led_driver_register(void);
void pdm_led_driver_unregister(void);
#endif

int32_t pdm_register_builtin_drivers(void)
{
	int32_t ret;

#ifdef CONFIG_PDM_MCU_SUPPORT
	ret = pdm_mcu_driver_register();
	if (ret != OSAL_SUCCESS)
		return ret;
#endif

#ifdef CONFIG_PDM_LED_SUPPORT
	ret = pdm_led_driver_register();
	if (ret != OSAL_SUCCESS) {
#ifdef CONFIG_PDM_MCU_SUPPORT
		pdm_mcu_driver_unregister();
#endif
		return ret;
	}
#endif

	return OSAL_SUCCESS;
}

void pdm_unregister_builtin_drivers(void)
{
#ifdef CONFIG_PDM_LED_SUPPORT
	pdm_led_driver_unregister();
#endif
#ifdef CONFIG_PDM_MCU_SUPPORT
	pdm_mcu_driver_unregister();
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
		return pdm_status_to_errno(ret);

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
