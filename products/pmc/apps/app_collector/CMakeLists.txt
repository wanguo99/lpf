# ==============================================================================
# PMC Collector Application
# ==============================================================================
# This application is responsible for data collection from various sources
# in the PMC (Cargo and Crew Module) system.
# ==============================================================================

# 构建依赖列表（根据 Kconfig 配置）
set(COLLECTOR_REQUIREMENTS)

# 必须的依赖
if(CONFIG_OSAL)
    list(APPEND COLLECTOR_REQUIREMENTS osal)
endif()

# libpmc 产品库
list(APPEND COLLECTOR_REQUIREMENTS libpmc pmc_runtime)

# 可选依赖（根据配置启用）
if(CONFIG_HAL)
    list(APPEND COLLECTOR_REQUIREMENTS hal)
endif()

if(CONFIG_PCONFIG)
    list(APPEND COLLECTOR_REQUIREMENTS pconfig)
endif()

if(CONFIG_PDL)
    list(APPEND COLLECTOR_REQUIREMENTS pdl)
endif()

if(CONFIG_ACONFIG)
    list(APPEND COLLECTOR_REQUIREMENTS aconfig pmc_aconfig)
endif()

# Register PMC collector application using simplified API
register_app_component(
    PRODUCT pmc
    REQUIREMENTS ${COLLECTOR_REQUIREMENTS}
)
