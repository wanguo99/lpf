# ============================================================================
# CCM Product CMakeLists.txt
# ============================================================================
# CCM (Central Control Module) product build configuration.
# This file is included by the root CMakeLists.txt when CONFIG_PRODUCT_CCM=y
# ============================================================================

# Register SDK core components
if(CONFIG_OSAL)
    add_subdirectory(${SDK_PATH}/core/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(${SDK_PATH}/core/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# Automatically load platform config based on CONFIG_PROJECT_NAME
load_platform_config(CONFIG_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR}/configs/platforms)

# Register PCL with platform config
if(CONFIG_PCL)
    # Add platform PCL config sources to PCL component
    if(PLATFORM_PCL_SRCS)
        set(PCL_EXTRA_SRCS ${PLATFORM_PCL_SRCS})
        set(PCL_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    add_subdirectory(${SDK_PATH}/core/pcl ${CMAKE_CURRENT_BINARY_DIR}/pcl)
endif()

if(CONFIG_PDL)
    add_subdirectory(${SDK_PATH}/core/pdl ${CMAKE_CURRENT_BINARY_DIR}/pdl)
endif()

# Register ACL with platform config
if(CONFIG_ACL)
    # Add platform ACL config sources to ACL component
    if(PLATFORM_ACL_SRCS)
        set(ACL_EXTRA_SRCS ${PLATFORM_ACL_SRCS})
        set(ACL_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    add_subdirectory(${SDK_PATH}/core/acl ${CMAKE_CURRENT_BINARY_DIR}/acl)
endif()

# Register product-specific libraries
if(CONFIG_LIBCCM)
    add_subdirectory(libs/libccm)
endif()

# Register applications
if(CONFIG_APP_COLLECTOR)
    add_subdirectory(apps/app_collector)
endif()

if(CONFIG_APP_COMM)
    add_subdirectory(apps/app_comm)
endif()

if(CONFIG_APP_HEALTH)
    add_subdirectory(apps/app_health)
endif()

if(CONFIG_APP_LOGGER)
    add_subdirectory(apps/app_logger)
endif()

if(CONFIG_APP_SUPERVISOR)
    add_subdirectory(apps/app_supervisor)
endif()
