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
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 计算 HWID 的 CRC16
 *
 * @param hwid HWID 指针
 * @return CRC16 值
 *
 * @note 校验前 18 字节（不包括 crc16 字段本身）
 */
static uint16_t pdl_misc_calculate_hwid_crc(const pdl_hwid_t *hwid)
{
    /* TODO: 实现真正的 CRC16 算法 */
    /* 当前返回简单校验和作为占位 */
    const uint8_t *data = (const uint8_t *)hwid;
    uint16_t sum = 0;
    size_t i;

    for (i = 0; i < sizeof(pdl_hwid_t) - sizeof(uint16_t); i++) {
        sum += data[i];
    }

    return sum;
}

/**
 * @brief 验证 HWID 有效性
 *
 * @param hwid HWID 指针
 * @return true=有效，false=无效
 */
static bool pdl_misc_validate_hwid(const pdl_hwid_t *hwid)
{
    uint16_t calculated_crc;

    if (hwid == NULL) {
        return false;
    }

    /* 检查魔数 */
    if (hwid->magic != PDL_HWID_MAGIC) {
        return false;
    }

    /* 检查格式版本 */
    if (hwid->format_version != PDL_HWID_FORMAT_V1) {
        return false;
    }

    /* 验证 CRC */
    calculated_crc = pdl_misc_calculate_hwid_crc(hwid);
    if (calculated_crc != hwid->crc16) {
        return false;
    }

    return true;
}

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
        /* TODO: 从环境变量解析 HWID 结构体 */
        /* 当前仅作为示例 */
    }

    /* 默认返回一个测试用的HWID */
    OSAL_memset(hwid, 0, sizeof(pdl_hwid_t));

    hwid->magic = PDL_HWID_MAGIC;
    hwid->format_version = PDL_HWID_FORMAT_V1;
    hwid->vendor_id = 0x01;
    hwid->product_id = 0x0001;      /* H200 */
    hwid->project_id = 0x0001;      /* 100P */
    hwid->board_type = 0x01;        /* 主板 */
    hwid->hw_revision = 0x10;       /* V1.0 */
    hwid->serial_number = 0x00000001;
    hwid->manufacture_date = 0x0821; /* 2024-01-01 */

    /* 计算 CRC */
    hwid->crc16 = pdl_misc_calculate_hwid_crc(hwid);

    return OSAL_SUCCESS;

#else
    /* 其他平台：返回未实现 */
    OSAL_memset(hwid, 0, sizeof(pdl_hwid_t));
    hwid->magic = PDL_HWID_INVALID;
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
