/**
 * @file pdl_mcu_helper.c
 * @brief PDL MCU 便捷接口实现
 */

#include "pdl_mcu.h"
#include "api/pcl_api.h"
#include "osal.h"

/**
 * @brief 通过设备名称初始化MCU驱动（便捷接口）
 */
int32_t PDL_MCU_InitByName(const char *device_name, pdl_mcu_handle_t *handle)
{
    const pcl_platform_config_t *platform;
    const pcl_mcu_entry_t *entry;

    if (!device_name || !handle) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取当前平台配置 */
    platform = PCL_GetBoard();
    if (!platform) {
        LOG_ERROR("PDL_MCU", "No platform config registered");
        return OSAL_ERR_NOT_FOUND;
    }

    /* 查找MCU配置 */
    entry = PCL_HW_FindMCU(platform, device_name);
    if (!entry) {
        LOG_ERROR("PDL_MCU", "MCU device not found: %s", device_name);
        return OSAL_ERR_NOT_FOUND;
    }

    /* 检查设备是否启用 */
    if (!entry->enabled) {
        LOG_WARN("PDL_MCU", "MCU device disabled: %s", device_name);
        return OSAL_ERR_GENERIC;
    }

    /* 调用标准初始化接口 */
    return PDL_MCU_Init(&entry->config, handle);
}
