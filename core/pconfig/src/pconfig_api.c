/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCONFIG_*       - 通用接口
 * - PCONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "pconfig/pconfig.h"

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

#define MAX_PLATFORM_CONFIGS 32

typedef struct {
    const pconfig_platform_config_t *configs[MAX_PLATFORM_CONFIGS];
    uint32_t count;
    const pconfig_platform_config_t *current;
} pconfig_registry_t;

static pconfig_registry_t g_registry = {0};
static bool g_initialized = false;
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
 * 配置库初始化
 *===========================================================================*/

int32_t PCONFIG_init(void)
{
    if (g_initialized) {
        return OSAL_SUCCESS;
    }

    OSAL_memset(&g_registry, 0, OSAL_sizeof(g_registry));
    g_initialized = true;

    LOG_INFO("PCONFIG", "Platform configuration library initialized");
    return OSAL_SUCCESS;
}

void PCONFIG_cleanup(void)
{
    if (!g_initialized) {
        return;
    }

    OSAL_memset(&g_registry, 0, OSAL_sizeof(g_registry));
    g_initialized = false;

    /* 销毁互斥锁 */
    OSAL_pthread_mutex_destroy(&g_registry_mutex);

    LOG_INFO("PCONFIG", "Platform configuration library cleaned up");
}

/*===========================================================================
 * 平台配置注册和查询
 *===========================================================================*/

int32_t PCONFIG_register(const pconfig_platform_config_t *config)
{
    uint32_t i;
    const pconfig_platform_config_t *existing;
    int32_t ret;

    if (!g_initialized) {
        LOG_ERROR("PCONFIG", "Library not initialized");
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config) {
        LOG_ERROR("PCONFIG", "Invalid config pointer");
        return OSAL_ERR_GENERIC;
    }

    /* 验证配置（在加锁前进行，减少临界区时间） */
    if (OSAL_SUCCESS != PCONFIG_validate(config)) {
        LOG_ERROR("PCONFIG", "Config validation failed: %s/%s",
                  config->platform_name, config->product_name);
        return OSAL_ERR_GENERIC;
    }

    /* 加锁保护全局注册表 */
    ret = OSAL_pthread_mutex_lock(&g_registry_mutex);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PCONFIG", "Failed to lock registry mutex");
        return ret;
    }

    /* 检查注册表是否已满 */
    if (g_registry.count >= MAX_PLATFORM_CONFIGS) {
        OSAL_pthread_mutex_unlock(&g_registry_mutex);
        LOG_ERROR("PCONFIG", "Registry full");
        return OSAL_ERR_GENERIC;
    }

    /* 检查重复 */
    for (i = 0; i < g_registry.count; i++) {
        existing = g_registry.configs[i];
        if (0 == OSAL_strcmp(existing->platform_name, config->platform_name) &&
            0 == OSAL_strcmp(existing->product_name, config->product_name)) {
            OSAL_pthread_mutex_unlock(&g_registry_mutex);
            LOG_WARN("PCONFIG", "Config already registered: %s/%s",
                     config->platform_name, config->product_name);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 注册配置 */
    g_registry.configs[g_registry.count++] = config;

    /* 解锁 */
    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO("PCONFIG", "Registered config: %s/%s",
             config->platform_name, config->product_name);

    return OSAL_SUCCESS;
}

const pconfig_platform_config_t* PCONFIG_GetBoard(void)
{
    const pconfig_platform_config_t *config;

    if (!g_initialized) {
        return NULL;
    }

    /* 加锁保护全局注册表 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_lock(&g_registry_mutex)) {
        return NULL;
    }

    config = g_registry.current;

    /* 解锁 */
    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    return config;
}

const pconfig_platform_config_t* PCONFIG_Find(const char *platform,
                                       const char *product,
                                       const char *version __attribute__((unused)))
{
    uint32_t i;
    const pconfig_platform_config_t *config;
    const pconfig_platform_config_t *found = NULL;

    if (!g_initialized || NULL == platform || NULL == product) {
        return NULL;
    }

    /* 加锁保护全局注册表 */
    if (OSAL_SUCCESS != OSAL_pthread_mutex_lock(&g_registry_mutex)) {
        return NULL;
    }

    for (i = 0; i < g_registry.count; i++) {
        config = g_registry.configs[i];

        if (0 != OSAL_strcmp(config->platform_name, platform)) {
            continue;
        }

        if (0 != OSAL_strcmp(config->product_name, product)) {
            continue;
        }

        found = config;
        break;
    }

    /* 解锁 */
    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    return found;
}

int32_t PCONFIG_list(const pconfig_platform_config_t **configs, uint32_t *count)
{
    uint32_t max_count;
    uint32_t actual_count;
    uint32_t i;
    int32_t ret;

    if (!g_initialized || NULL == configs || NULL == count) {
        return OSAL_ERR_GENERIC;
    }

    /* 加锁保护全局注册表 */
    ret = OSAL_pthread_mutex_lock(&g_registry_mutex);
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
    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 硬件外设配置查询接口（PCONFIG_HW_*）
 *
 * 说明：所有 Get 函数已改为 inline 函数，在头文件中实现
 *===========================================================================*/

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t PCONFIG_validate(const pconfig_platform_config_t *config)
{
    if (NULL == config) {
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config->platform_name || NULL == config->product_name) {
        LOG_ERROR("PCONFIG", "Missing platform or product name");
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

void PCONFIG_print(const pconfig_platform_config_t *config)
{
    uint32_t i;

    if (NULL == config) {
        return;
    }

    LOG_INFO("PCONFIG", "Platform: %s", config->platform_name);
    LOG_INFO("PCONFIG", "Product: %s", config->product_name);

    /* 打印MCU配置 */
    if (config->mcu_array) {
        for (i = 0; i < config->mcu_count; i++) {
            LOG_INFO("PCONFIG", "  MCU[%u]: %s", i,
                     config->mcu_array[i].description ? config->mcu_array[i].description : "N/A");
        }
    }

    /* 打印FPGA配置 */
    if (config->fpga_array) {
        for (i = 0; i < config->fpga_count; i++) {
            LOG_INFO("PCONFIG", "  FPGA[%u]: %s", i,
                     config->fpga_array[i].description ? config->fpga_array[i].description : "N/A");
        }
    }

    /* 打印Switch配置 */
    if (config->switch_array) {
        for (i = 0; i < config->switch_count; i++) {
            LOG_INFO("PCONFIG", "  Switch[%u]: %s", i,
                     config->switch_array[i].description ? config->switch_array[i].description : "N/A");
        }
    }

    /* 打印PMC配置 */
    if (config->pmc_array) {
        for (i = 0; i < config->pmc_count; i++) {
            LOG_INFO("PCONFIG", "  PMC[%u]: %s", i,
                     config->pmc_array[i].description ? config->pmc_array[i].description : "N/A");
        }
    }
}
