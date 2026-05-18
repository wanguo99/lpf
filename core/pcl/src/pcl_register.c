/************************************************************************
 * 硬件配置自动注册
 *
 * 功能：
 * - 自动注册所有平台的硬件配置
 * - 在系统启动时调用
 ************************************************************************/

#include "pcl_api.h"
#include <osal.h>

/*===========================================================================
 * 外部配置声明
 *===========================================================================*/

/* TI AM625平台 - H200载荷板 */
/* 注意：产品特定配置已移至 products/ 目录，由产品自己管理 */
/* extern const pcl_platform_config_t pcl_h200_100p_base; */
/* extern const pcl_platform_config_t pcl_h200_100p_v1; */
/* extern const pcl_platform_config_t pcl_h200_100p_v2; */

/* 其他平台配置可以在这里添加 */
/* extern const pcl_platform_config_t pcl_xxx; */

/*===========================================================================
 * 配置注册表
 *===========================================================================*/

static const pcl_platform_config_t* g_all_configs[] = {
    /* 产品特定配置已移至 products/ 目录 */
    /* &pcl_h200_100p_base, */
    /* &pcl_h200_100p_v1, */
    /* &pcl_h200_100p_v2, */
    /* 在这里添加新的配置 */
};

#define CONFIG_COUNT (sizeof(g_all_configs) / sizeof(g_all_configs[0]))

/*===========================================================================
 * 注册函数
 *===========================================================================*/

/**
 * @brief 注册所有硬件配置
 *
 * 注意：产品特定配置已移至 products/ 目录，由产品自己管理和注册
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PCL_RegisterAll(void)
{
    LOG_INFO("XCONFIG", "No built-in configurations to register (product configs managed separately)");
    return OSAL_SUCCESS;
}

/**
 * @brief 根据环境变量或编译选项选择默认配置
 *
 * 优先级：
 * 1. 环境变量 PCL_PLATFORM, PCL_PRODUCT, PCL_VERSION
 * 2. 编译时定义 DEFAULT_PLATFORM, DEFAULT_PRODUCT, DEFAULT_VERSION
 * 3. 默认使用第一个配置
 *
 * @return 配置指针，失败返回NULL
 */
const pcl_platform_config_t* PCL_SelectDefault(void)
{
    const char *platform = NULL;
    const char *product = NULL;
    const char *version = NULL;
    const pcl_platform_config_t *config = NULL;

    /* 1. 尝试从环境变量读取 */
    platform = OSAL_getenv("PCL_PLATFORM");
    product = OSAL_getenv("PCL_PRODUCT");
    version = OSAL_getenv("PCL_VERSION");

    if (NULL != platform && NULL != product) {
        config = PCL_Find(platform, product, version);
        if (NULL != config) {
            LOG_INFO("XCONFIG", "Selected config from environment: %s/%s/%s",
                     platform, product, version ? version : "any");
            return config;
        }
    }

    /* 2. 尝试使用编译时定义 */
#if defined(DEFAULT_PLATFORM) && defined(DEFAULT_PRODUCT)
    platform = DEFAULT_PLATFORM;
    product = DEFAULT_PRODUCT;
#ifdef DEFAULT_VERSION
    version = DEFAULT_VERSION;
#else
    version = NULL;
#endif

    config = PCL_Find(platform, product, version);
    if (NULL != config) {
        LOG_INFO("XCONFIG", "Selected config from compile-time defaults: %s/%s/%s",
                 platform, product, version ? version : "any");
        return config;
    }
#endif

    /* 3. 使用第一个配置作为默认 */
    if (CONFIG_COUNT > 0) {
        config = g_all_configs[0];
        LOG_INFO("PCL", "Using first config as default: %s/%s",
                 config->platform_name, config->product_name);
        return config;
    }

    LOG_ERROR("XCONFIG", "No configuration available");
    return NULL;
}
