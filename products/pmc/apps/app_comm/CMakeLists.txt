# ==============================================================================
# PMC Comm Application
# ==============================================================================
# This application handles communication interfaces for the PMC system.
# ==============================================================================

# 构建依赖列表（根据 Kconfig 配置）
set(COMM_REQUIREMENTS)

# 必须的依赖
if(CONFIG_OSAL)
    list(APPEND COMM_REQUIREMENTS osal)
endif()

# libpmc 产品库
list(APPEND COMM_REQUIREMENTS libpmc pmc_runtime)

# 可选依赖（根据配置启用）
if(CONFIG_HAL)
    list(APPEND COMM_REQUIREMENTS hal)
endif()

if(CONFIG_PCONFIG)
    list(APPEND COMM_REQUIREMENTS pconfig)
endif()

if(CONFIG_PDL)
    list(APPEND COMM_REQUIREMENTS pdl)
endif()

if(CONFIG_ACONFIG)
    list(APPEND COMM_REQUIREMENTS aconfig pmc_aconfig)
endif()

# Register PMC comm application using simplified API
register_app_component(
    PRODUCT pmc
    REQUIREMENTS ${COMM_REQUIREMENTS}
)
