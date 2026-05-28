# ============================================================================
# CCM Product CMakeLists.txt
# ============================================================================
# CCM (Central Control Module) product build configuration.
# This file is included by the root CMakeLists.txt when CONFIG_PRODUCT_CCM=y
# ============================================================================

# Register SDK core components
if(CONFIG_OSAL)
    add_subdirectory(${SDK_PATH}/components/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(${SDK_PATH}/components/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# Set platform config directory and collect config files
if(CONFIG_H200_AM625)
    set(PLATFORM_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/configs/platforms/h200_am625)

    # Collect platform config source files
    file(GLOB PLATFORM_ACL_SRCS "${PLATFORM_CONFIG_DIR}/acl/*.c" "${PLATFORM_CONFIG_DIR}/acl/*/*.c")
    file(GLOB PLATFORM_PCL_SRCS "${PLATFORM_CONFIG_DIR}/pcl/*.c")

    # Add platform config include directories
    set(PLATFORM_CONFIG_INCLUDE ${PLATFORM_CONFIG_DIR}/acl)
endif()

# Register PCL with platform config
if(CONFIG_PCL)
    # Add platform PCL config sources to PCL component
    if(PLATFORM_PCL_SRCS)
        set(PCL_EXTRA_SRCS ${PLATFORM_PCL_SRCS})
        set(PCL_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    add_subdirectory(${SDK_PATH}/components/pcl ${CMAKE_CURRENT_BINARY_DIR}/pcl)
endif()

if(CONFIG_PDL)
    add_subdirectory(${SDK_PATH}/components/pdl ${CMAKE_CURRENT_BINARY_DIR}/pdl)
endif()

# Register ACL with platform config
if(CONFIG_ACL)
    # Add platform ACL config sources to ACL component
    if(PLATFORM_ACL_SRCS)
        set(ACL_EXTRA_SRCS ${PLATFORM_ACL_SRCS})
        set(ACL_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    add_subdirectory(${SDK_PATH}/components/acl ${CMAKE_CURRENT_BINARY_DIR}/acl)
endif()

# Register product-specific components
if(CONFIG_LIBCCM)
    add_subdirectory(components/libccm)
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
