/************************************************************************
 * PDL层 - 杂项服务实现（打桩版本）
 *
 * 说明：
 * - 当前为打桩实现，返回模拟数据
 * - 实际实现需要根据具体平台进行适配
 ************************************************************************/

#include "osal.h"
#include "pdl.h"

/*===========================================================================
 * 硬件ID (HWID) 实现
 *===========================================================================*/

/**
 * @brief 读取硬件ID（打桩实现）
 *
 * 实际实现建议：
 * - Linux: 从设备树 /proc/device-tree/hardware-id 读取
 * - RTOS: 从 EEPROM、OTP 或固定地址读取
 * - 测试: 从环境变量 EMS_HWID 读取
 */
int32_t PDL_MISC_GetHWID(pdl_hwid_t *hwid)
{
    if (hwid == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* TODO: 实际实现时需要根据平台读取真实HWID */

#ifdef EMS_PLATFORM_LINUX
    /* Linux平台：尝试从环境变量读取，用于测试 */
    const char *hwid_env = OSAL_GetEnv("EMS_HWID");
    if (hwid_env != NULL) {
        char *endptr;
        unsigned long value = OSAL_StrToUL(hwid_env, &endptr, 0);
        if (*endptr == '\0') {
            *hwid = (pdl_hwid_t)value;
            return OSAL_SUCCESS;
        }
    }

    /* 默认返回一个测试用的HWID */
    *hwid = 0x00010001;  /* 示例：版本1.1 */
    return OSAL_SUCCESS;

#else
    /* 其他平台：返回未实现 */
    *hwid = PDL_HWID_UNKNOWN;
    return OSAL_ERR_NOT_IMPLEMENTED;
#endif
}

/*===========================================================================
 * 分区表查询实现
 *===========================================================================*/

int32_t PDL_MISC_GetPartitionInfo(const char *name, pdl_partition_info_t *info)
{
    if (name == NULL || info == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* TODO: 实际实现时需要从内核或MTD设备读取分区表 */

    /* 打桩：返回未实现 */
    return OSAL_ERR_NOT_IMPLEMENTED;
}

/*===========================================================================
 * 设备启动信息实现
 *===========================================================================*/

int32_t PDL_MISC_GetBootInfo(pdl_boot_info_t *info)
{
    if (info == NULL) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* TODO: 实际实现时需要读取启动原因寄存器或日志 */

    /* 打桩：返回模拟数据 */
    info->reason = PDL_BOOT_REASON_POWER_ON;
    info->boot_count = 1;
    info->last_boot_time = 0;

    return OSAL_SUCCESS;
}
