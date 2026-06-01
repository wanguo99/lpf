/**
 * @file pdl_bmc_helper.c
 * @brief PDL BMC 便捷接口实现
 */

#include "pdl_bmc.h"
#include "api/pcl_api.h"
#include "osal.h"

/**
 * @brief 通过设备名称初始化BMC服务（便捷接口）
 */
int32_t PDL_BMC_InitByName(const char *device_name, pdl_bmc_handle_t *handle)
{
    const pcl_platform_config_t *platform;
    const pcl_bmc_entry_t *entry;

    if (!device_name || !handle) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取当前平台配置 */
    platform = PCL_GetBoard();
    if (!platform) {
        LOG_ERROR("PDL_BMC", "No platform config registered");
        return OSAL_ERR_NOT_FOUND;
    }

    /* 查找BMC配置 */
    entry = PCL_HW_FindBMC(platform, device_name);
    if (!entry) {
        LOG_ERROR("PDL_BMC", "BMC device not found: %s", device_name);
        return OSAL_ERR_NOT_FOUND;
    }

    /* 检查设备是否启用 */
    if (!entry->enabled) {
        LOG_WARN("PDL_BMC", "BMC device disabled: %s", device_name);
        return OSAL_ERR_GENERIC;
    }

    /* 调用标准初始化接口 */
    return PDL_BMC_Init(&entry->config, handle);
}
