# ==============================================================================
# PMC Supervisor Application
# ==============================================================================
# This application provides supervision and process management for the PMC system.
# ==============================================================================

# 构建依赖列表（根据 Kconfig 配置）
set(SUPERVISOR_REQUIREMENTS)

# 必须的依赖
if(CONFIG_OSAL)
    list(APPEND SUPERVISOR_REQUIREMENTS osal)
endif()

# libpmc 产品库
list(APPEND SUPERVISOR_REQUIREMENTS libpmc)

# 可选依赖（根据配置启用）
if(CONFIG_HAL)
    list(APPEND SUPERVISOR_REQUIREMENTS hal)
endif()

if(CONFIG_PCONFIG)
    list(APPEND SUPERVISOR_REQUIREMENTS pconfig)
endif()

if(CONFIG_PDL)
    list(APPEND SUPERVISOR_REQUIREMENTS pdl)
endif()

if(CONFIG_ACONFIG)
    list(APPEND SUPERVISOR_REQUIREMENTS aconfig)
endif()

# Register PMC supervisor application using simplified API
register_app_component(
    PRODUCT pmc
    REQUIREMENTS ${SUPERVISOR_REQUIREMENTS}
)
