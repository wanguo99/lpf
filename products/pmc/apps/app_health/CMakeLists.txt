# ==============================================================================
# PMC Health Application
# ==============================================================================
# This application monitors system health and diagnostics for the PMC system.
# ==============================================================================

# 构建依赖列表（根据 Kconfig 配置）
set(HEALTH_REQUIREMENTS)

# 必须的依赖
if(CONFIG_OSAL)
    list(APPEND HEALTH_REQUIREMENTS osal)
endif()

# libpmc 产品库
list(APPEND HEALTH_REQUIREMENTS libpmc pmc_runtime)

# 可选依赖（根据配置启用）
if(CONFIG_HAL)
    list(APPEND HEALTH_REQUIREMENTS hal)
endif()

if(CONFIG_PCONFIG)
    list(APPEND HEALTH_REQUIREMENTS pconfig)
endif()

if(CONFIG_PDL)
    list(APPEND HEALTH_REQUIREMENTS pdl)
endif()

if(CONFIG_ACONFIG)
    list(APPEND HEALTH_REQUIREMENTS aconfig pmc_aconfig)
endif()

# Register PMC health application using simplified API
register_app_component(
    PRODUCT pmc
    REQUIREMENTS ${HEALTH_REQUIREMENTS}
)
