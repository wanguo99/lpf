/************************************************************************
 * PCL - FPGA外设配置
 *
 * 功能：
 * - FPGA外设配置类型定义
 * - 支持多种通信接口（SPI、I2C、PCIe等）
 ************************************************************************/

#ifndef PCL_FPGA_H
#define PCL_FPGA_H

#include "pcl_hardware_interface.h"

/**
 * @brief FPGA外设配置
 */
typedef struct {
    /* 外设基本信息 */
    const char *name;                    /* 外设名称 */
    const char *description;             /* 描述信息 */
    bool enabled;                        /* 是否启用 */

    /* 通信接口 */
    pcl_hw_interface_type_t interface_type;
    pcl_hw_interface_cfg_t interface_cfg;

    /* FPGA特定配置 */
    uint32_t cmd_timeout_ms;             /* 命令超时时间 */
    uint32_t retry_count;                /* 重试次数 */
} pcl_fpga_cfg_t;

#endif /* PCL_FPGA_H */
