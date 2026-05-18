/************************************************************************
 * PCL板级配置
 *
 * 功能：
 * - 板级配置类型定义（顶层）
 * - 聚合所有硬件外设配置（MCU、BMC、FPGA、Switch）
 *
 * 设计原则：
 * - 只包含纯硬件外设配置
 * - 不包含业务逻辑（卫星平台、传感器、存储、应用等）
 * - 配置以外设为单位，简洁明了
 ************************************************************************/

#ifndef PCL_BOARD_H
#define PCL_BOARD_H

#include "pcl_mcu.h"
#include "pcl_bmc.h"
#include "pcl_fpga.h"
#include "pcl_switch.h"

/*===========================================================================
 * 板级配置（顶层）
 *===========================================================================*/

/**
 * @brief 板级硬件配置
 *
 * 这是顶层配置结构，以外设为单位描述整个板子的硬件配置
 * 只包含纯硬件外设：MCU、BMC、FPGA、Switch
 */
typedef struct {
    /* 板级信息 */
    const char *platform_name;    /* 平台名称（如"ti"） */
    const char *chip_name;        /* 芯片名称（如"am6254"） */
    const char *project_name;     /* 项目名称（如"H200_100P"） */
    const char *product_name;     /* 产品名称（如"h200_100p_base"） */

    /* 硬件外设配置数组（NULL结尾） */
    pcl_mcu_cfg_t    **mcu_arr;      /* MCU外设数组 */
    pcl_bmc_cfg_t    **bmc_arr;      /* BMC外设数组 */
    pcl_fpga_cfg_t   **fpga_arr;     /* FPGA外设数组 */
    pcl_switch_cfg_t **switch_arr;   /* Switch外设数组 */
} pcl_platform_config_t;

#endif /* PCL_BOARD_H */
