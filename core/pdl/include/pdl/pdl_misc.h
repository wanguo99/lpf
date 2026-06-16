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
 * @brief 硬件ID结构体（20字节紧凑型）
 *
 * 设计说明：
 * - magic: 'HWID' ASCII 编码，用于验证数据有效性
 * - format_version: 支持未来格式演进
 * - 包含产品、项目、板卡类型、硬件版本
 * - 每块板子有唯一的序列号
 * - 包含生产日期和 CRC 校验
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 魔数：0x48574944 ('HWID') */
    uint8_t  format_version;    /* HWID 格式版本：0x01 */
    uint8_t  vendor_id;         /* 厂商ID */

    uint16_t product_id;        /* 产品ID */
    uint16_t project_id;        /* 项目ID */

    uint8_t  board_type;        /* 板卡类型 */
    uint8_t  hw_revision;       /* 硬件版本 */

    uint32_t serial_number;     /* 序列号（唯一标识） */

    uint16_t manufacture_date;  /* 生产日期（压缩格式） */
    uint16_t crc16;             /* CRC16 校验 */
} pdl_hwid_t;

/**
 * @brief 魔数定义
 */
#define PDL_HWID_MAGIC          0x48574944  /* 'HWID' ASCII 编码 */

/**
 * @brief HWID 格式版本
 */
#define PDL_HWID_FORMAT_V1      0x01        /* 当前版本 */

/**
 * @brief 特殊值定义（用于兼容性）
 */
#define PDL_HWID_INVALID        0x00000000  /* 无效的 magic，表示未初始化 */

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
int32_t PDL_MISC_get_hwid(pdl_hwid_t *hwid);

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
int32_t PDL_MISC_get_partition_info(const char *name, pdl_partition_info_t *info);

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
int32_t PDL_MISC_get_boot_info(pdl_boot_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* PDL_MISC_H */
