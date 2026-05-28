/************************************************************************
 * TI AM625平台 - H200-100P载荷板V1配置
 ************************************************************************/

#include "pcl/pcl_config.h"

static pcl_mcu_cfg_t *pcl_mcu_arr[] = { NULL };
static pcl_bmc_cfg_t *pcl_bmc_arr[] = { NULL };
static pcl_fpga_cfg_t *pcl_fpga_arr[] = { NULL };
static pcl_switch_cfg_t *pcl_switch_arr[] = { NULL };

const pcl_platform_config_t pcl_h200_100p_v1 = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P",
    .product_name = "h200_100p_v1",
    
    .mcu_arr = pcl_mcu_arr,
    .bmc_arr = pcl_bmc_arr,
    .fpga_arr = pcl_fpga_arr,
    .switch_arr = pcl_switch_arr
};
