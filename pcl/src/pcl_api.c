/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCL_*       - 通用接口
 * - PCL_HW_*    - 硬件配置接口
 * - PCL_APP_*   - APP配置接口
 ************************************************************************/

#include "pcl_api.h"
#include "util/osal_log.h"
#include "osal.h"

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

#define MAX_BOARD_CONFIGS 32

typedef struct {
    const pcl_board_config_t *configs[MAX_BOARD_CONFIGS];
    uint32_t count;
    const pcl_board_config_t *current;
} pcl_registry_t;

static pcl_registry_t g_registry = {0};
static bool g_initialized = false;

/*===========================================================================
 * 配置库初始化
 *===========================================================================*/

int32_t PCL_Init(void)
{
    if (g_initialized) {
        return OSAL_SUCCESS;
    }

    OSAL_Memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = true;

    LOG_INFO("XCONFIG", "Hardware configuration library initialized");
    return OSAL_SUCCESS;
}

void PCL_Cleanup(void)
{
    if (!g_initialized) {
        return;
    }

    OSAL_Memset(&g_registry, 0, sizeof(g_registry));
    g_initialized = false;

    LOG_INFO("XCONFIG", "Hardware configuration library cleaned up");
}

/*===========================================================================
 * 板级配置注册和查询
 *===========================================================================*/

int32_t PCL_Register(const pcl_board_config_t *config)
{
    if (!g_initialized) {
        LOG_ERROR("XCONFIG", "Library not initialized");
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config) {
        LOG_ERROR("XCONFIG", "Invalid config pointer");
        return OSAL_ERR_GENERIC;
    }

    if (g_registry.count >= MAX_BOARD_CONFIGS) {
        LOG_ERROR("XCONFIG", "Registry full (max %d configs)", MAX_BOARD_CONFIGS);
        return OSAL_ERR_GENERIC;
    }

    /* 验证配置 */
    if (OSAL_SUCCESS != PCL_Validate(config)) {
        LOG_ERROR("XCONFIG", "Config validation failed: %s/%s/%s",
                  config->platform, config->product, config->version);
        return OSAL_ERR_GENERIC;
    }

    /* 检查重复 */
    for (uint32_t i = 0; i < g_registry.count; i++) {
        const pcl_board_config_t *existing = g_registry.configs[i];
        if (0 == OSAL_Strcmp(existing->platform, config->platform) &&
            0 == OSAL_Strcmp(existing->product, config->product) &&
            0 == OSAL_Strcmp(existing->version, config->version)) {
            LOG_WARN("XCONFIG", "Config already registered: %s/%s/%s",
                     config->platform, config->product, config->version);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 注册配置 */
    g_registry.configs[g_registry.count++] = config;

    LOG_INFO("XCONFIG", "Registered config: %s/%s/%s - %s",
             config->platform, config->product, config->version,
             config->description);

    return OSAL_SUCCESS;
}

const pcl_board_config_t* PCL_GetBoard(void)
{
    if (!g_initialized) {
        return NULL;
    }
    return g_registry.current;
}

const pcl_board_config_t* PCL_Find(const char *platform,
                                       const char *product,
                                       const char *version)
{
    if (!g_initialized || NULL == platform || NULL == product) {
        return NULL;
    }

    for (uint32_t i = 0; i < g_registry.count; i++) {
        const pcl_board_config_t *config = g_registry.configs[i];

        if (0 != OSAL_Strcmp(config->platform, platform)) {
            continue;
        }

        if (0 != OSAL_Strcmp(config->product, product)) {
            continue;
        }

        /* 如果指定了版本，则精确匹配；否则返回第一个匹配的 */
        if (NULL == version || 0 == OSAL_Strcmp(config->version, version)) {
            return config;
        }
    }

    return NULL;
}

int32_t PCL_List(const pcl_board_config_t **configs, uint32_t *count)
{
    if (!g_initialized || NULL == configs || NULL == count) {
        return OSAL_ERR_GENERIC;
    }

    uint32_t max_count = *count;
    uint32_t actual_count = (g_registry.count < max_count) ? g_registry.count : max_count;

    for (uint32_t i = 0; i < actual_count; i++) {
        configs[i] = g_registry.configs[i];
    }

    *count = actual_count;
    return OSAL_SUCCESS;
}

/*===========================================================================
 * 硬件外设配置查询接口（PCL_HW_*）
 *===========================================================================*/

const pcl_mcu_cfg_t* PCL_HW_FindMCU(const pcl_board_config_t *board,
                                        const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->mcu_count; i++) {
        if (OSAL_Strcmp(board->mcus[i]->name, name) == 0) {
            return board->mcus[i];
        }
    }

    return NULL;
}

const pcl_mcu_cfg_t* PCL_HW_GetMCU(const pcl_board_config_t *board,
                                       uint32_t id)
{
    if (NULL == board || id >= board->mcu_count) {
        return NULL;
    }

    return board->mcus[id];
}

const pcl_bmc_cfg_t* PCL_HW_FindBMC(const pcl_board_config_t *board,
                                        const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->bmc_count; i++) {
        if (OSAL_Strcmp(board->bmcs[i]->name, name) == 0) {
            return board->bmcs[i];
        }
    }

    return NULL;
}

const pcl_bmc_cfg_t* PCL_HW_GetBMC(const pcl_board_config_t *board,
                                       uint32_t id)
{
    if (NULL == board || id >= board->bmc_count) {
        return NULL;
    }

    return board->bmcs[id];
}

const pcl_satellite_cfg_t* PCL_HW_FindSatellite(const pcl_board_config_t *board,
                                                     const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->satellite_count; i++) {
        if (OSAL_Strcmp(board->satellites[i]->name, name) == 0) {
            return board->satellites[i];
        }
    }

    return NULL;
}

const pcl_satellite_cfg_t* PCL_HW_GetSatellite(const pcl_board_config_t *board,
                                                    uint32_t id)
{
    if (NULL == board || id >= board->satellite_count) {
        return NULL;
    }

    return board->satellites[id];
}

const pcl_sensor_cfg_t* PCL_HW_FindSensor(const pcl_board_config_t *board,
                                              const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->sensor_count; i++) {
        if (OSAL_Strcmp(board->sensors[i]->name, name) == 0) {
            return board->sensors[i];
        }
    }

    return NULL;
}

const pcl_sensor_cfg_t* PCL_HW_GetSensor(const pcl_board_config_t *board,
                                             uint32_t id)
{
    if (NULL == board || id >= board->sensor_count) {
        return NULL;
    }

    return board->sensors[id];
}

const pcl_storage_cfg_t* PCL_HW_FindStorage(const pcl_board_config_t *board,
                                                 const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->storage_count; i++) {
        if (OSAL_Strcmp(board->storages[i]->name, name) == 0) {
            return board->storages[i];
        }
    }

    return NULL;
}

const pcl_storage_cfg_t* PCL_HW_GetStorage(const pcl_board_config_t *board,
                                                uint32_t id)
{
    if (NULL == board || id >= board->storage_count) {
        return NULL;
    }

    return board->storages[id];
}

const pcl_power_domain_t* PCL_HW_FindPowerDomain(const pcl_board_config_t *board,
                                                      const char *name)
{
    if (NULL == board || NULL == name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->power_domain_count; i++) {
        if (OSAL_Strcmp(board->power_domains[i]->name, name) == 0) {
            return board->power_domains[i];
        }
    }

    return NULL;
}

/*===========================================================================
 * APP配置查询接口（PCL_APP_*）
 *===========================================================================*/

const pcl_app_config_t* PCL_APP_Find(const pcl_board_config_t *board,
                                         const char *app_name)
{
    if (NULL == board || NULL == app_name) {
        return NULL;
    }

    for (uint32_t i = 0; i < board->app_count; i++) {
        if (OSAL_Strcmp(board->apps[i]->app_name, app_name) == 0) {
            return board->apps[i];
        }
    }

    return NULL;
}

const pcl_app_device_mapping_t* PCL_APP_FindDevice(const pcl_app_config_t *app,
                                                        const char *function)
{
    if (NULL == app || NULL == function) {
        return NULL;
    }

    for (uint32_t i = 0; i < app->mapping_count; i++) {
        if (OSAL_Strcmp(app->device_mappings[i].function, function) == 0) {
            return &app->device_mappings[i];
        }
    }

    return NULL;
}

const void* PCL_APP_GetDeviceByMapping(const pcl_board_config_t *board,
                                             const pcl_app_device_mapping_t *mapping)
{
    if (NULL == board || NULL == mapping) {
        return NULL;
    }

    switch (mapping->device_type) {
        case PCL_DEV_MCU:
            return PCL_HW_GetMCU(board, mapping->device_id);

        case PCL_DEV_BMC:
            return PCL_HW_GetBMC(board, mapping->device_id);

        case PCL_DEV_SATELLITE:
            return PCL_HW_GetSatellite(board, mapping->device_id);

        case PCL_DEV_SENSOR:
            return PCL_HW_GetSensor(board, mapping->device_id);

        case PCL_DEV_STORAGE:
            return PCL_HW_GetStorage(board, mapping->device_id);

        default:
            return NULL;
    }
}

/*===========================================================================
 * 配置验证辅助函数
 *===========================================================================*/

/**
 * @brief 验证MCU配置（匹配新的配置结构）
 */
static int32_t validate_mcu_interface(const pcl_mcu_cfg_t *mcu)
{
    /* 验证接口类型 */
    if (mcu->interface >= PCL_MCU_INTERFACE_SPI + 1) {
        LOG_ERROR("XCONFIG", "MCU '%s': Invalid interface type %d",
                  mcu->name, mcu->interface);
        return OSAL_ERR_GENERIC;
    }

    /* 根据接口类型验证配置 */
    switch (mcu->interface) {
        case PCL_MCU_INTERFACE_CAN:
            if (NULL == mcu->can.device || OSAL_Strlen(mcu->can.device) == 0) {
                LOG_ERROR("XCONFIG", "MCU '%s': CAN device name is empty", mcu->name);
                return OSAL_ERR_GENERIC;
            }
            if (mcu->can.bitrate == 0) {
                LOG_ERROR("XCONFIG", "MCU '%s': CAN bitrate is zero", mcu->name);
                return OSAL_ERR_GENERIC;
            }
            break;

        case PCL_MCU_INTERFACE_SERIAL:
            if (NULL == mcu->serial.device || OSAL_Strlen(mcu->serial.device) == 0) {
                LOG_ERROR("XCONFIG", "MCU '%s': Serial device name is empty", mcu->name);
                return OSAL_ERR_GENERIC;
            }
            if (mcu->serial.baudrate == 0) {
                LOG_ERROR("XCONFIG", "MCU '%s': Serial baudrate is zero", mcu->name);
                return OSAL_ERR_GENERIC;
            }
            break;

        case PCL_MCU_INTERFACE_I2C:
        case PCL_MCU_INTERFACE_SPI:
            /* I2C和SPI暂时不验证，预留 */
            break;

        default:
            LOG_ERROR("XCONFIG", "MCU '%s': Unsupported interface type %d",
                      mcu->name, mcu->interface);
            return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 验证卫星平台配置（匹配新的配置结构）
 */
static int32_t validate_satellite_interface(const pcl_satellite_cfg_t *sat)
{
    /* 验证CAN设备名 */
    if (NULL == sat->can_device || OSAL_Strlen(sat->can_device) == 0) {
        LOG_ERROR("XCONFIG", "Satellite '%s': CAN device name is empty", sat->name);
        return OSAL_ERR_GENERIC;
    }

    /* 验证CAN波特率 */
    if (sat->can_bitrate == 0) {
        LOG_ERROR("XCONFIG", "Satellite '%s': CAN bitrate is zero", sat->name);
        return OSAL_ERR_GENERIC;
    }

    /* 验证心跳间隔 */
    if (sat->heartbeat_interval_ms == 0) {
        LOG_WARN("XCONFIG", "Satellite '%s': Heartbeat interval is zero", sat->name);
    }

    /* 验证命令超时 */
    if (sat->cmd_timeout_ms == 0) {
        LOG_WARN("XCONFIG", "Satellite '%s': Command timeout is zero", sat->name);
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 验证BMC配置（匹配新的配置结构）
 */
static int32_t validate_bmc_channel(const pcl_bmc_cfg_t *bmc)
{
    /* 验证网络通道配置 */
    if (bmc->network.enabled) {
        if (NULL == bmc->network.ip_addr || OSAL_Strlen(bmc->network.ip_addr) == 0) {
            LOG_ERROR("XCONFIG", "BMC '%s': Network IP address is empty", bmc->name);
            return OSAL_ERR_GENERIC;
        }
        if (bmc->network.port == 0) {
            LOG_WARN("XCONFIG", "BMC '%s': Network port is zero, using default 623", bmc->name);
        }
        if (bmc->network.timeout_ms == 0) {
            LOG_WARN("XCONFIG", "BMC '%s': Network timeout is zero", bmc->name);
        }
    }

    /* 验证串口通道配置 */
    if (bmc->serial.enabled) {
        if (NULL == bmc->serial.device || OSAL_Strlen(bmc->serial.device) == 0) {
            LOG_ERROR("XCONFIG", "BMC '%s': Serial device name is empty", bmc->name);
            return OSAL_ERR_GENERIC;
        }
        if (bmc->serial.baudrate == 0) {
            LOG_ERROR("XCONFIG", "BMC '%s': Serial baudrate is zero", bmc->name);
            return OSAL_ERR_GENERIC;
        }
        if (bmc->serial.timeout_ms == 0) {
            LOG_WARN("XCONFIG", "BMC '%s': Serial timeout is zero", bmc->name);
        }
    }

    /* 至少要启用一个通道 */
    if (!bmc->network.enabled && !bmc->serial.enabled) {
        LOG_ERROR("XCONFIG", "BMC '%s': Both network and serial channels are disabled", bmc->name);
        return OSAL_ERR_GENERIC;
    }

    /* 验证主通道设置 */
    if (bmc->primary_channel == PCL_BMC_CHANNEL_NETWORK && !bmc->network.enabled) {
        LOG_ERROR("XCONFIG", "BMC '%s': Primary channel is network but network is disabled", bmc->name);
        return OSAL_ERR_GENERIC;
    }
    if (bmc->primary_channel == PCL_BMC_CHANNEL_SERIAL && !bmc->serial.enabled) {
        LOG_ERROR("XCONFIG", "BMC '%s': Primary channel is serial but serial is disabled", bmc->name);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t PCL_Validate(const pcl_board_config_t *config)
{
    if (NULL == config) {
        LOG_ERROR("XCONFIG", "Config is NULL");
        return OSAL_ERR_GENERIC;
    }

    /* 验证基本字段 */
    if (NULL == config->platform || 0 == OSAL_Strlen(config->platform)) {
        LOG_ERROR("XCONFIG", "Invalid platform name");
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config->product || 0 == OSAL_Strlen(config->product)) {
        LOG_ERROR("XCONFIG", "Invalid product name");
        return OSAL_ERR_GENERIC;
    }

    if (NULL == config->version || 0 == OSAL_Strlen(config->version)) {
        LOG_ERROR("XCONFIG", "Invalid version");
        return OSAL_ERR_GENERIC;
    }

    /* 验证MCU配置 */
    if (config->mcu_count > 0 && NULL == config->mcus) {
        LOG_ERROR("XCONFIG", "MCU count > 0 but mcus is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->mcu_count; i++) {
        const pcl_mcu_cfg_t *mcu = config->mcus[i];
        if (NULL == mcu) {
            LOG_ERROR("XCONFIG", "MCU[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (OSAL_Strlen(mcu->name) == 0) {
            LOG_ERROR("XCONFIG", "MCU[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
        /* 验证MCU硬件接口配置 */
        if (mcu->enabled && validate_mcu_interface(mcu) != OSAL_SUCCESS) {
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证BMC配置 */
    if (config->bmc_count > 0 && NULL == config->bmcs) {
        LOG_ERROR("XCONFIG", "BMC count > 0 but bmcs is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->bmc_count; i++) {
        const pcl_bmc_cfg_t *bmc = config->bmcs[i];
        if (NULL == bmc) {
            LOG_ERROR("XCONFIG", "BMC[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == bmc->name || OSAL_Strlen(bmc->name) == 0) {
            LOG_ERROR("XCONFIG", "BMC[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
        /* 验证BMC通道配置 */
        if (bmc->enabled && validate_bmc_channel(bmc) != OSAL_SUCCESS) {
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证卫星平台配置 */
    if (config->satellite_count > 0 && NULL == config->satellites) {
        LOG_ERROR("XCONFIG", "Satellite count > 0 but satellites is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->satellite_count; i++) {
        const pcl_satellite_cfg_t *sat = config->satellites[i];
        if (NULL == sat) {
            LOG_ERROR("XCONFIG", "Satellite[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == sat->name || OSAL_Strlen(sat->name) == 0) {
            LOG_ERROR("XCONFIG", "Satellite[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
        /* 验证卫星平台硬件接口配置 */
        if (sat->enabled && validate_satellite_interface(sat) != OSAL_SUCCESS) {
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证传感器配置 */
    if (config->sensor_count > 0 && NULL == config->sensors) {
        LOG_ERROR("XCONFIG", "Sensor count > 0 but sensors is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->sensor_count; i++) {
        if (NULL == config->sensors[i]) {
            LOG_ERROR("XCONFIG", "Sensor[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == config->sensors[i]->name ||
            OSAL_Strlen(config->sensors[i]->name) == 0) {
            LOG_ERROR("XCONFIG", "Sensor[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证存储设备配置 */
    if (config->storage_count > 0 && NULL == config->storages) {
        LOG_ERROR("XCONFIG", "Storage count > 0 but storages is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->storage_count; i++) {
        if (NULL == config->storages[i]) {
            LOG_ERROR("XCONFIG", "Storage[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == config->storages[i]->name ||
            OSAL_Strlen(config->storages[i]->name) == 0) {
            LOG_ERROR("XCONFIG", "Storage[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证电源域配置 */
    if (config->power_domain_count > 0 && NULL == config->power_domains) {
        LOG_ERROR("XCONFIG", "Power domain count > 0 but power_domains is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->power_domain_count; i++) {
        if (NULL == config->power_domains[i]) {
            LOG_ERROR("XCONFIG", "Power domain[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == config->power_domains[i]->name ||
            OSAL_Strlen(config->power_domains[i]->name) == 0) {
            LOG_ERROR("XCONFIG", "Power domain[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
    }

    /* 验证APP配置 */
    if (config->app_count > 0 && NULL == config->apps) {
        LOG_ERROR("XCONFIG", "APP count > 0 but apps is NULL");
        return OSAL_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < config->app_count; i++) {
        const pcl_app_config_t *app = config->apps[i];
        if (NULL == app) {
            LOG_ERROR("XCONFIG", "APP[%d] is NULL", i);
            return OSAL_ERR_GENERIC;
        }
        if (NULL == app->app_name || OSAL_Strlen(app->app_name) == 0) {
            LOG_ERROR("XCONFIG", "APP[%d] name is empty", i);
            return OSAL_ERR_GENERIC;
        }
        /* 验证设备映射 */
        if (app->mapping_count > 0 && NULL == app->device_mappings) {
            LOG_ERROR("XCONFIG", "APP '%s': mapping_count > 0 but device_mappings is NULL",
                      app->app_name);
            return OSAL_ERR_GENERIC;
        }
    }

    return OSAL_SUCCESS;
}

void PCL_Print(const pcl_board_config_t *config)
{
    if (NULL == config) {
        OSAL_Printf("Config is NULL\n");
        return;
    }

    OSAL_Printf("\n========================================\n");
    OSAL_Printf("Board Configuration\n");
    OSAL_Printf("========================================\n");
    OSAL_Printf("Platform:    %s\n", config->platform);
    OSAL_Printf("Product:     %s\n", config->product);
    OSAL_Printf("Version:     %s\n", config->version);
    OSAL_Printf("Description: %s\n", config->description);
    OSAL_Printf("\n");

    /* 打印MCU外设配置 */
    OSAL_Printf("MCU Peripherals: %d\n", config->mcu_count);
    for (uint32_t i = 0; i < config->mcu_count; i++) {
        const pcl_mcu_cfg_t *mcu = config->mcus[i];
        OSAL_Printf("  [%d] %s - %s\n", i, mcu->name,
                  mcu->enabled ? "Enabled" : "Disabled");
        OSAL_Printf("      Interface: %s\n",
                  mcu->interface == PCL_MCU_INTERFACE_CAN ? "CAN" :
                  mcu->interface == PCL_MCU_INTERFACE_SERIAL ? "UART" :
                  mcu->interface == PCL_MCU_INTERFACE_I2C ? "I2C" :
                  mcu->interface == PCL_MCU_INTERFACE_SPI ? "SPI" : "Unknown");
    }
    OSAL_Printf("\n");

    /* 打印BMC外设配置 */
    OSAL_Printf("BMC Peripherals: %d\n", config->bmc_count);
    for (uint32_t i = 0; i < config->bmc_count; i++) {
        const pcl_bmc_cfg_t *bmc = config->bmcs[i];
        OSAL_Printf("  [%d] %s - %s\n", i, bmc->name,
                  bmc->enabled ? "Enabled" : "Disabled");
    }
    OSAL_Printf("\n");

    /* 打印卫星平台接口配置 */
    OSAL_Printf("Satellite Interfaces: %d\n", config->satellite_count);
    for (uint32_t i = 0; i < config->satellite_count; i++) {
        const pcl_satellite_cfg_t *sat = config->satellites[i];
        OSAL_Printf("  [%d] %s - %s\n", i, sat->name,
                  sat->enabled ? "Enabled" : "Disabled");
    }
    OSAL_Printf("\n");

    /* 打印传感器外设配置 */
    OSAL_Printf("Sensor Peripherals: %d\n", config->sensor_count);
    for (uint32_t i = 0; i < config->sensor_count; i++) {
        const pcl_sensor_cfg_t *sensor = config->sensors[i];
        OSAL_Printf("  [%d] %s - %s\n", i, sensor->name,
                  sensor->enabled ? "Enabled" : "Disabled");
    }
    OSAL_Printf("\n");

    /* 打印存储设备配置 */
    OSAL_Printf("Storage Devices: %d\n", config->storage_count);
    for (uint32_t i = 0; i < config->storage_count; i++) {
        const pcl_storage_cfg_t *storage = config->storages[i];
        OSAL_Printf("  [%d] %s - %s\n", i, storage->name,
                  storage->enabled ? "Enabled" : "Disabled");
    }
    OSAL_Printf("\n");

    /* 打印APP配置 */
    OSAL_Printf("APP Configurations: %d\n", config->app_count);
    for (uint32_t i = 0; i < config->app_count; i++) {
        const pcl_app_config_t *app = config->apps[i];
        OSAL_Printf("  [%d] %s - %s\n", i, app->app_name, app->description);
        OSAL_Printf("      Device Mappings: %d\n", app->mapping_count);
        for (uint32_t j = 0; j < app->mapping_count; j++) {
            const pcl_app_device_mapping_t *mapping = &app->device_mappings[j];
            OSAL_Printf("        - %s: %s[%d] %s\n",
                      mapping->function,
                      mapping->device_type == PCL_DEV_MCU ? "MCU" :
                      mapping->device_type == PCL_DEV_BMC ? "BMC" :
                      mapping->device_type == PCL_DEV_SATELLITE ? "SATELLITE" :
                      mapping->device_type == PCL_DEV_SENSOR ? "SENSOR" :
                      mapping->device_type == PCL_DEV_STORAGE ? "STORAGE" : "Unknown",
                      mapping->device_id,
                      mapping->required ? "(Required)" : "(Optional)");
        }
    }

    OSAL_Printf("========================================\n\n");
}
