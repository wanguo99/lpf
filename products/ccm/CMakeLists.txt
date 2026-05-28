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

if(CONFIG_PCL)
    add_subdirectory(${SDK_PATH}/components/pcl ${CMAKE_CURRENT_BINARY_DIR}/pcl)
endif()

if(CONFIG_PDL)
    add_subdirectory(${SDK_PATH}/components/pdl ${CMAKE_CURRENT_BINARY_DIR}/pdl)
endif()

if(CONFIG_ACL)
    add_subdirectory(${SDK_PATH}/components/acl ${CMAKE_CURRENT_BINARY_DIR}/acl)
endif()

# Set platform config directory based on Kconfig
if(CONFIG_H200_AM625)
    set(PLATFORM_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/configs/platforms/h200_am625)

    # Collect platform config source files
    file(GLOB PLATFORM_ACL_SRCS "${PLATFORM_CONFIG_DIR}/acl/*.c" "${PLATFORM_CONFIG_DIR}/acl/*/*.c")
    file(GLOB PLATFORM_PCL_SRCS "${PLATFORM_CONFIG_DIR}/pcl/*.c")
    set(PLATFORM_CONFIG_SRCS ${PLATFORM_ACL_SRCS} ${PLATFORM_PCL_SRCS})

    # Add platform config include directories (both root and acl subdirectory)
    set(PLATFORM_CONFIG_INCLUDE ${PLATFORM_CONFIG_DIR} ${PLATFORM_CONFIG_DIR}/acl)
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
