# ============================================================================
# Sample Product CMakeLists.txt
# ============================================================================
# Sample product build configuration.
# This file is included by the root CMakeLists.txt when CONFIG_PRODUCT_SAMPLE=y
# ============================================================================

# Register components for this product
if(CONFIG_OSAL)
    add_subdirectory(${SDK_PATH}/core/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(${SDK_PATH}/core/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

if(CONFIG_TM_CACHE)
    add_subdirectory(${SDK_PATH}/core/tm_cache ${CMAKE_CURRENT_BINARY_DIR}/tm_cache)
endif()

# Register application
if(CONFIG_APP_DEMO)
    add_subdirectory(apps/app_demo)
endif()
