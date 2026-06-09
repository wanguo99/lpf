/************************************************************************
 * PDL层 - 杂项服务接口
 *
 * 功能：
 * - 硬件ID（HWID）读取
 * - 设备分区表查询
 * - 设备启动信息
 * - 其他非通用外设功能
 ************************************************************************/

#ifndef PDL_MISC_H
#define PDL_MISC_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 硬件ID (HWID)
 *===========================================================================*/

/**
 * @brief 硬件ID类型
 *
 * HWID 用于标识不同的硬件版本，以便加载对应的配置
 */
typedef uint32_t pdl_hwid_t;

/**
 * @brief 特殊HWID值
 */
#define PDL_HWID_INVALID    0x00000000  /* 无效HWID */
#define PDL_HWID_UNKNOWN    0xFFFFFFFF  /* 未知HWID */

/**
 * @brief 读取硬件ID
 *
 * @param[out] hwid    返回读取到的硬件ID
 *
 * @return 错误码
 * @retval OSAL_OK               成功
 * @retval OSAL_ERR_INVALID_PARAM 参数无效
 * @retval OSAL_ERR_NOT_IMPLEMENTED 功能未实现
 * @retval OSAL_ERR_IO           读取失败
 *
 * @note 实现方式：
 *       - Linux: 从设备树、EEPROM 或文件读取
 *       - RTOS: 从 EEPROM、OTP 或配置区读取
 *       - 测试环境: 从环境变量或配置文件读取
 */
int32_t PDL_MISC_GetHWID(pdl_hwid_t *hwid);

/*===========================================================================
 * 分区表查询
 *===========================================================================*/

/**
 * @brief 分区信息
 */
typedef struct {
    char name[32];          /* 分区名称 */
    uint64_t offset;        /* 起始偏移 */
    uint64_t size;          /* 分区大小 */
    uint32_t flags;         /* 分区标志 */
} pdl_partition_info_t;

/**
 * @brief 获取分区信息
 *
 * @param[in] name          分区名称
 * @param[out] info         返回分区信息
 *
 * @return 错误码
 * @retval OSAL_OK               成功
 * @retval OSAL_ERR_INVALID_PARAM 参数无效
 * @retval OSAL_ERR_NOT_FOUND    分区不存在
 * @retval OSAL_ERR_NOT_IMPLEMENTED 功能未实现
 */
int32_t PDL_MISC_GetPartitionInfo(const char *name, pdl_partition_info_t *info);

/*===========================================================================
 * 设备启动信息
 *===========================================================================*/

/**
 * @brief 启动原因
 */
typedef enum {
    PDL_BOOT_REASON_UNKNOWN = 0,    /* 未知 */
    PDL_BOOT_REASON_POWER_ON,       /* 上电启动 */
    PDL_BOOT_REASON_RESET,          /* 复位启动 */
    PDL_BOOT_REASON_WATCHDOG,       /* 看门狗复位 */
    PDL_BOOT_REASON_SOFTWARE,       /* 软件复位 */
    PDL_BOOT_REASON_UPGRADE,        /* 升级后启动 */
} pdl_boot_reason_t;

/**
 * @brief 启动信息
 */
typedef struct {
    pdl_boot_reason_t reason;       /* 启动原因 */
    uint32_t boot_count;            /* 启动次数 */
    uint64_t last_boot_time;        /* 上次启动时间（秒） */
} pdl_boot_info_t;

/**
 * @brief 获取设备启动信息
 *
 * @param[out] info         返回启动信息
 *
 * @return 错误码
 * @retval OSAL_OK               成功
 * @retval OSAL_ERR_INVALID_PARAM 参数无效
 * @retval OSAL_ERR_NOT_IMPLEMENTED 功能未实现
 */
int32_t PDL_MISC_GetBootInfo(pdl_boot_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* PDL_MISC_H */
