# ==============================================================================
# PMC Test Application
# ==============================================================================
# Complete call chain test application for verification
# ==============================================================================

# Build dependencies
set(TEST_REQUIREMENTS)

# Required dependencies
if(CONFIG_OSAL)
    list(APPEND TEST_REQUIREMENTS osal)
endif()

if(CONFIG_HAL)
    list(APPEND TEST_REQUIREMENTS hal)
endif()

if(CONFIG_PDL)
    list(APPEND TEST_REQUIREMENTS pdl)
endif()

if(CONFIG_PCONFIG)
    list(APPEND TEST_REQUIREMENTS pconfig)
endif()

if(CONFIG_ACONFIG)
    list(APPEND TEST_REQUIREMENTS aconfig pmc_aconfig)
endif()

if(TARGET pmc_runtime)
    list(APPEND TEST_REQUIREMENTS pmc_runtime)
endif()

# Platform config object
if(TARGET platform_config_obj)
    list(APPEND TEST_REQUIREMENTS platform_config_obj)
endif()

# Register test application using simplified API
register_app_component(
    PRODUCT pmc
    REQUIREMENTS ${TEST_REQUIREMENTS}
)

