/************************************************************************
 * TI AM625平台 - H200-AM625载荷板V1配置
 ************************************************************************/

#include "pconfig.h"

static pconfig_mcu_entry_t *pconfig_mcu_arr[] = { NULL };
static pconfig_bmc_entry_t *pconfig_bmc_arr[] = { NULL };
static pconfig_fpga_cfg_t *pconfig_fpga_arr[] = { NULL };
static pconfig_switch_cfg_t *pconfig_switch_arr[] = { NULL };

const pconfig_platform_config_t pconfig_h200_am625_v1 = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_AM625",
    .product_name = "h200_am625_v1",

    .mcu_count = 0,
    .mcu_arr = pconfig_mcu_arr,
    .bmc_count = 0,
    .bmc_arr = pconfig_bmc_arr,
    .fpga_count = 0,
    .fpga_arr = pconfig_fpga_arr,
    .switch_count = 0,
    .switch_arr = pconfig_switch_arr
};
