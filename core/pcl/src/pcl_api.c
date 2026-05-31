/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCL_*       - 通用接口
 * - PCL_HW_*    - 硬件配置接口
 ************************************************************************/

#include "api/pcl_api.h"
#include "util/osal_log.h"
#include "osal.h"

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

#define MAX_PLATFORM_CONFIGS 32

typedef struct {
    const pcl_platform_config_t *configs[MAX_PLATFORM_CONFIGS];
    uint32_t count;
    const pcl_platform_config_t *current;
} pcl_registry_t;

static pcl_registry_t g_registry = {0};
static bool g_initialized = false;
static osal_mutex_t g_registry_mutex = NULL;

/*===========================================================================
 * 配置库初始化
 *===========================================================================*/

int32_t PCL_Init(void)
{
    int32_t ret;

    if (g_initialized) {
        return OSAL_SUCCESS;
    }

    /* 创建互斥锁保护全局注册表 */
    ret = OSAL_MutexCreate(&g_registry_mutex);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PCL", "Failed to create registry mutex");
        return ret;
    }

    OSAL_Memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = true;

    LOG_INFO("PCL", "Platform configuration library initialized");
    return OSAL_SUCCESS;
}

void PCL_Cleanup(void)
{
    if (!g_initialized) {
        return;
    }

    OSAL_Memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = false;

    /* 销毁互斥锁 */
    if (NULL != g_registry_mutex) {
        OSAL_MutexDestroy(g_registry_mutex);
        g_registry_mutex = NULL;
    }

    LOG_INFO("PCL", "Platform configuration library cleaned up");
}

/*===========================================================================
 * 平台配置注册和查询
 *===========================================================================*/

int32_t PCL_Register(const pcl_platform_config_t *config)
{
    uint32_t i;
    const pcl_platform_config_t *existing;
    int32_t ret;

    if (!g_initialized) {
        LOG_ERROR("PCL", "Library not initialized");
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config) {
        LOG_ERROR("PCL", "Invalid config pointer");
        return OSAL_ERR_GENERIC;
    }

    /* 验证配置（在加锁前进行，减少临界区时间） */
    if (OSAL_SUCCESS != PCL_Validate(config)) {
        LOG_ERROR("PCL", "Config validation failed: %s/%s",
                  config->platform_name, config->product_name);
        return OSAL_ERR_GENERIC;
    }

    /* 加锁保护全局注册表 */
    ret = OSAL_MutexLock(g_registry_mutex);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PCL", "Failed to lock registry mutex");
        return ret;
    }

    /* 检查注册表是否已满 */
    if (g_registry.count >= MAX_PLATFORM_CONFIGS) {
        OSAL_MutexUnlock(g_registry_mutex);
        LOG_ERROR("PCL", "Registry full");
        return OSAL_ERR_GENERIC;
    }

    /* 检查重复 */
    for (i = 0; i < g_registry.count; i++) {
        existing = g_registry.configs[i];
        if (0 == OSAL_Strcmp(existing->platform_name, config->platform_name) &&
            0 == OSAL_Strcmp(existing->product_name, config->product_name)) {
            OSAL_MutexUnlock(g_registry_mutex);
            LOG_WARN("PCL", "Config already registered: %s/%s",
                     config->platform_name, config->product_name);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 注册配置 */
    g_registry.configs[g_registry.count++] = config;

    /* 解锁 */
    OSAL_MutexUnlock(g_registry_mutex);

    LOG_INFO("PCL", "Registered config: %s/%s",
             config->platform_name, config->product_name);

    return OSAL_SUCCESS;
}

const pcl_platform_config_t* PCL_GetBoard(void)
{
    const pcl_platform_config_t *config;

    if (!g_initialized) {
        return NULL;
    }

    /* 加锁保护全局注册表 */
    if (OSAL_SUCCESS != OSAL_MutexLock(g_registry_mutex)) {
        return NULL;
    }

    config = g_registry.current;

    /* 解锁 */
    OSAL_MutexUnlock(g_registry_mutex);

    return config;
}

const pcl_platform_config_t* PCL_Find(const char *platform,
                                       const char *product,
                                       const char *version __attribute__((unused)))
{
    uint32_t i;
    const pcl_platform_config_t *config;
    const pcl_platform_config_t *found = NULL;

    if (!g_initialized || NULL == platform || NULL == product) {
        return NULL;
    }

    /* 加锁保护全局注册表 */
    if (OSAL_SUCCESS != OSAL_MutexLock(g_registry_mutex)) {
        return NULL;
    }

    for (i = 0; i < g_registry.count; i++) {
        config = g_registry.configs[i];

        if (0 != OSAL_Strcmp(config->platform_name, platform)) {
            continue;
        }

        if (0 != OSAL_Strcmp(config->product_name, product)) {
            continue;
        }

        found = config;
        break;
    }

    /* 解锁 */
    OSAL_MutexUnlock(g_registry_mutex);

    return found;
}

int32_t PCL_List(const pcl_platform_config_t **configs, uint32_t *count)
{
    uint32_t max_count;
    uint32_t actual_count;
    uint32_t i;
    int32_t ret;

    if (!g_initialized || NULL == configs || NULL == count) {
        return OSAL_ERR_GENERIC;
    }

    /* 加锁保护全局注册表 */
    ret = OSAL_MutexLock(g_registry_mutex);
    if (OSAL_SUCCESS != ret) {
        return ret;
    }

    max_count = *count;
    actual_count = (g_registry.count < max_count) ? g_registry.count : max_count;

    for (i = 0; i < actual_count; i++) {
        configs[i] = g_registry.configs[i];
    }

    *count = actual_count;

    /* 解锁 */
    OSAL_MutexUnlock(g_registry_mutex);

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 硬件外设配置查询接口（PCL_HW_*）
 *===========================================================================*/

const pcl_mcu_entry_t* PCL_HW_FindMCU(const pcl_platform_config_t *platform,
                                      const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->mcu_arr) {
        return NULL;
    }

    for (i = 0; platform->mcu_arr[i] != NULL; i++) {
        if (OSAL_Strcmp(platform->mcu_arr[i]->name, name) == 0) {
            return platform->mcu_arr[i];
        }
    }

    return NULL;
}

const pcl_mcu_entry_t* PCL_HW_GetMCU(const pcl_platform_config_t *platform,
                                     uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->mcu_arr) {
        return NULL;
    }

    for (i = 0; platform->mcu_arr[i] != NULL; i++) {
        if (i == id) {
            return platform->mcu_arr[i];
        }
    }

    return NULL;
}

const pcl_bmc_entry_t* PCL_HW_FindBMC(const pcl_platform_config_t *platform,
                                      const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->bmc_arr) {
        return NULL;
    }

    for (i = 0; platform->bmc_arr[i] != NULL; i++) {
        if (OSAL_Strcmp(platform->bmc_arr[i]->name, name) == 0) {
            return platform->bmc_arr[i];
        }
    }

    return NULL;
}

const pcl_bmc_entry_t* PCL_HW_GetBMC(const pcl_platform_config_t *platform,
                                     uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->bmc_arr) {
        return NULL;
    }

    for (i = 0; platform->bmc_arr[i] != NULL; i++) {
        if (i == id) {
            return platform->bmc_arr[i];
        }
    }

    return NULL;
}

const pcl_fpga_cfg_t* PCL_HW_FindFPGA(const pcl_platform_config_t *platform,
                                          const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->fpga_arr) {
        return NULL;
    }

    for (i = 0; platform->fpga_arr[i] != NULL; i++) {
        if (OSAL_Strcmp(platform->fpga_arr[i]->name, name) == 0) {
            return platform->fpga_arr[i];
        }
    }

    return NULL;
}

const pcl_fpga_cfg_t* PCL_HW_GetFPGA(const pcl_platform_config_t *platform,
                                         uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->fpga_arr) {
        return NULL;
    }

    for (i = 0; platform->fpga_arr[i] != NULL; i++) {
        if (i == id) {
            return platform->fpga_arr[i];
        }
    }

    return NULL;
}

const pcl_switch_cfg_t* PCL_HW_FindSwitch(const pcl_platform_config_t *platform,
                                              const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->switch_arr) {
        return NULL;
    }

    for (i = 0; platform->switch_arr[i] != NULL; i++) {
        if (OSAL_Strcmp(platform->switch_arr[i]->name, name) == 0) {
            return platform->switch_arr[i];
        }
    }

    return NULL;
}

const pcl_switch_cfg_t* PCL_HW_GetSwitch(const pcl_platform_config_t *platform,
                                             uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->switch_arr) {
        return NULL;
    }

    for (i = 0; platform->switch_arr[i] != NULL; i++) {
        if (i == id) {
            return platform->switch_arr[i];
        }
    }

    return NULL;
}

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t PCL_Validate(const pcl_platform_config_t *config)
{
    if (NULL == config) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config->platform_name || NULL == config->product_name) {
        LOG_ERROR("PCL", "Missing platform or product name");
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

void PCL_Print(const pcl_platform_config_t *config)
{
    uint32_t i;

    if (NULL == config) {
        return;
    }

    LOG_INFO("PCL", "Platform: %s", config->platform_name);
    LOG_INFO("PCL", "Product: %s", config->product_name);

    /* 打印MCU配置 */
    if (config->mcu_arr) {
        for (i = 0; config->mcu_arr[i] != NULL; i++) {
            LOG_INFO("PCL", "  MCU[%u]: %s", i, config->mcu_arr[i]->name);
        }
    }

    /* 打印BMC配置 */
    if (config->bmc_arr) {
        for (i = 0; config->bmc_arr[i] != NULL; i++) {
            LOG_INFO("PCL", "  BMC[%u]: %s", i, config->bmc_arr[i]->name);
        }
    }
}
