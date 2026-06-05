/************************************************************************
 * TI AM625平台 - H200-100P载荷板V2配置
 ************************************************************************/

#include "pconfig.h"

static pconfig_mcu_entry_t *pconfig_mcu_arr[] = { NULL };
static pconfig_bmc_entry_t *pconfig_bmc_arr[] = { NULL };
static pconfig_fpga_cfg_t *pconfig_fpga_arr[] = { NULL };
static pconfig_switch_cfg_t *pconfig_switch_arr[] = { NULL };

const pconfig_platform_config_t pconfig_h200_100p_v2 = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P",
    .product_name = "h200_100p_v2",

    .mcu_arr = pconfig_mcu_arr,
    .bmc_arr = pconfig_bmc_arr,
    .fpga_arr = pconfig_fpga_arr,
    .switch_arr = pconfig_switch_arr
};
