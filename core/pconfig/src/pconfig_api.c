/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCONFIG_*       - 通用接口
 * - PCONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "pdl.h"
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

int32_t PCONFIG_Init(void)
{
    if (g_initialized) {
        return OSAL_SUCCESS;
    }

    OSAL_memset(&g_registry, 0, OSAL_sizeof(g_registry));
    g_initialized = true;

    LOG_INFO("PCL", "Platform configuration library initialized");
    return OSAL_SUCCESS;
}

void PCONFIG_Cleanup(void)
{
    if (!g_initialized) {
        return;
    }

    OSAL_memset(&g_registry, 0, OSAL_sizeof(g_registry));
    g_initialized = false;

    /* 销毁互斥锁 */
    OSAL_pthread_mutex_destroy(&g_registry_mutex);

    LOG_INFO("PCL", "Platform configuration library cleaned up");
}

/*===========================================================================
 * 平台配置注册和查询
 *===========================================================================*/

int32_t PCONFIG_Register(const pconfig_platform_config_t *config)
{
    uint32_t i;
    const pconfig_platform_config_t *existing;
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
    if (OSAL_SUCCESS != PCONFIG_Validate(config)) {
        LOG_ERROR("PCL", "Config validation failed: %s/%s",
                  config->platform_name, config->product_name);
        return OSAL_ERR_GENERIC;
    }

    /* 加锁保护全局注册表 */
    ret = OSAL_pthread_mutex_lock(&g_registry_mutex);
    if (OSAL_SUCCESS != ret) {
        LOG_ERROR("PCL", "Failed to lock registry mutex");
        return ret;
    }

    /* 检查注册表是否已满 */
    if (g_registry.count >= MAX_PLATFORM_CONFIGS) {
        OSAL_pthread_mutex_unlock(&g_registry_mutex);
        LOG_ERROR("PCL", "Registry full");
        return OSAL_ERR_GENERIC;
    }

    /* 检查重复 */
    for (i = 0; i < g_registry.count; i++) {
        existing = g_registry.configs[i];
        if (0 == OSAL_strcmp(existing->platform_name, config->platform_name) &&
            0 == OSAL_strcmp(existing->product_name, config->product_name)) {
            OSAL_pthread_mutex_unlock(&g_registry_mutex);
            LOG_WARN("PCL", "Config already registered: %s/%s",
                     config->platform_name, config->product_name);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 注册配置 */
    g_registry.configs[g_registry.count++] = config;

    /* 解锁 */
    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO("PCL", "Registered config: %s/%s",
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

int32_t PCONFIG_List(const pconfig_platform_config_t **configs, uint32_t *count)
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
 *===========================================================================*/

const pconfig_mcu_entry_t* PCONFIG_HW_FindMCU(const pconfig_platform_config_t *platform,
                                      const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->mcu_arr) {
        return NULL;
    }

    for (i = 0; i < platform->mcu_count; i++) {
        if (OSAL_strcmp(platform->mcu_arr[i]->name, name) == 0) {
            return platform->mcu_arr[i];
        }
    }

    return NULL;
}

const pconfig_mcu_entry_t* PCONFIG_HW_GetMCU(const pconfig_platform_config_t *platform,
                                     uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->mcu_arr) {
        return NULL;
    }

    for (i = 0; i < platform->mcu_count; i++) {
        if (i == id) {
            return platform->mcu_arr[i];
        }
    }

    return NULL;
}

const pconfig_bmc_entry_t* PCONFIG_HW_FindBMC(const pconfig_platform_config_t *platform,
                                      const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->bmc_arr) {
        return NULL;
    }

    for (i = 0; i < platform->bmc_count; i++) {
        if (OSAL_strcmp(platform->bmc_arr[i]->name, name) == 0) {
            return platform->bmc_arr[i];
        }
    }

    return NULL;
}

const pconfig_bmc_entry_t* PCONFIG_HW_GetBMC(const pconfig_platform_config_t *platform,
                                     uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->bmc_arr) {
        return NULL;
    }

    for (i = 0; i < platform->bmc_count; i++) {
        if (i == id) {
            return platform->bmc_arr[i];
        }
    }

    return NULL;
}

const pconfig_fpga_cfg_t* PCONFIG_HW_FindFPGA(const pconfig_platform_config_t *platform,
                                          const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->fpga_arr) {
        return NULL;
    }

    for (i = 0; i < platform->fpga_count; i++) {
        if (OSAL_strcmp(platform->fpga_arr[i]->name, name) == 0) {
            return platform->fpga_arr[i];
        }
    }

    return NULL;
}

const pconfig_fpga_cfg_t* PCONFIG_HW_GetFPGA(const pconfig_platform_config_t *platform,
                                         uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->fpga_arr) {
        return NULL;
    }

    for (i = 0; i < platform->fpga_count; i++) {
        if (i == id) {
            return platform->fpga_arr[i];
        }
    }

    return NULL;
}

const pconfig_switch_cfg_t* PCONFIG_HW_FindSwitch(const pconfig_platform_config_t *platform,
                                              const char *name)
{
    uint32_t i;

    if (NULL == platform || NULL == name || NULL == platform->switch_arr) {
        return NULL;
    }

    for (i = 0; i < platform->switch_count; i++) {
        if (OSAL_strcmp(platform->switch_arr[i]->name, name) == 0) {
            return platform->switch_arr[i];
        }
    }

    return NULL;
}

const pconfig_switch_cfg_t* PCONFIG_HW_GetSwitch(const pconfig_platform_config_t *platform,
                                             uint32_t id)
{
    uint32_t i;

    if (NULL == platform || NULL == platform->switch_arr) {
        return NULL;
    }

    for (i = 0; i < platform->switch_count; i++) {
        if (i == id) {
            return platform->switch_arr[i];
        }
    }

    return NULL;
}

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t PCONFIG_Validate(const pconfig_platform_config_t *config)
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

void PCONFIG_Print(const pconfig_platform_config_t *config)
{
    uint32_t i;

    if (NULL == config) {
        return;
    }

    LOG_INFO("PCL", "Platform: %s", config->platform_name);
    LOG_INFO("PCL", "Product: %s", config->product_name);

    /* 打印MCU配置 */
    if (config->mcu_arr) {
        for (i = 0; i < config->mcu_count; i++) {
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

/*===========================================================================
 * HWID 相关接口
 *===========================================================================*/

/**
 * @brief 检查配置是否支持指定的HWID
 *
 * @param[in] config 配置指针
 * @param[in] hwid   硬件ID结构体指针
 *
 * @return true=支持，false=不支持
 */
static bool pconfig_is_hwid_supported(const pconfig_platform_config_t *config, const pdl_hwid_t *hwid)
{
    uint32_t i;

    if (config == NULL || hwid == NULL) {
        return false;
    }

    /* hwid_count == 0 或 hwid_list == NULL 表示支持所有HWID */
    if (config->hwid_count == 0 || config->hwid_list == NULL) {
        return true;
    }

    /* 在HWID列表中查找匹配项 */
    for (i = 0; i < config->hwid_count; i++) {
        const pdl_hwid_t *pattern = &config->hwid_list[i];

        /* 匹配规则：比较关键字段，忽略序列号和生产日期 */
        if (pattern->magic == hwid->magic &&
            pattern->product_id == hwid->product_id &&
            pattern->project_id == hwid->project_id &&
            pattern->board_type == hwid->board_type &&
            pattern->hw_revision == hwid->hw_revision) {
            return true;
        }
    }

    return false;
}

const pconfig_platform_config_t* PCONFIG_FindByHWID(const pdl_hwid_t *hwid)
{
    uint32_t i;
    const pconfig_platform_config_t *config = NULL;

    if (!g_initialized) {
        LOG_ERROR("PCL", "Library not initialized");
        return NULL;
    }

    if (hwid == NULL) {
        LOG_ERROR("PCL", "Invalid HWID pointer");
        return NULL;
    }

    /* 加锁保护 */
    OSAL_pthread_mutex_lock(&g_registry_mutex);

    /* 遍历所有已注册的配置 */
    for (i = 0; i < g_registry.count; i++) {
        if (pconfig_is_hwid_supported(g_registry.configs[i], hwid)) {
            config = g_registry.configs[i];
            LOG_INFO("PCL", "Found config for HWID: product=0x%04X, project=0x%04X, board=0x%02X, hw_rev=0x%02X -> %s/%s",
                     hwid->product_id, hwid->project_id, hwid->board_type, hwid->hw_revision,
                     config->platform_name, config->product_name);
            break;
        }
    }

    OSAL_pthread_mutex_unlock(&g_registry_mutex);

    if (config == NULL) {
        LOG_WARN("PCL", "No config found for HWID: product=0x%04X, project=0x%04X, board=0x%02X, hw_rev=0x%02X",
                 hwid->product_id, hwid->project_id, hwid->board_type, hwid->hw_revision);
    }

    return config;
}

int32_t PCONFIG_LoadByHWID(void)
{
    pdl_hwid_t hwid;
    const pconfig_platform_config_t *config;
    int32_t ret;

    if (!g_initialized) {
        LOG_ERROR("PCL", "Library not initialized");
        return OSAL_ERR_INVALID_STATE;
    }

    /* 从 PDL_MISC 读取 HWID */
    ret = PDL_MISC_GetHWID(&hwid);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("PCL", "Failed to read HWID: %d", ret);
        return ret;
    }

    LOG_INFO("PCL", "Read HWID: magic=0x%08X, product=0x%04X, project=0x%04X, board=0x%02X, hw_rev=0x%02X, SN=%u",
             hwid.magic, hwid.product_id, hwid.project_id, hwid.board_type, hwid.hw_revision, hwid.serial_number);

    /* 根据 HWID 查找配置 */
    config = PCONFIG_FindByHWID(&hwid);
    if (config == NULL) {
        LOG_ERROR("PCL", "No matching config for HWID");
        return OSAL_ERR_NAME_NOT_FOUND;
    }

    /* 注册配置（Register 会自动设置为 current） */
    ret = PCONFIG_Register(config);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("PCL", "Failed to register config: %d", ret);
        return ret;
    }

    LOG_INFO("PCL", "Successfully loaded config for HWID");
    return OSAL_SUCCESS;
}
