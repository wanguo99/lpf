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

if(CONFIG_PRL)
    add_subdirectory(${SDK_PATH}/core/prl ${CMAKE_CURRENT_BINARY_DIR}/prl)
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

# ============================================================================
# Generate Executables from Application Components
# ============================================================================
# Application components are built as static libraries (libapp_*.a)
# Here we create executables that link against these libraries
# ============================================================================

if(CONFIG_APP_COLLECTOR)
    add_executable(collector ${CMAKE_CURRENT_BINARY_DIR}/collector_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/collector_main.c
                       COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/collector_main.c
                       VERBATIM)
    target_link_libraries(collector PRIVATE app_collector)
endif()

if(CONFIG_APP_COMM)
    add_executable(comm ${CMAKE_CURRENT_BINARY_DIR}/comm_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comm_main.c
                       COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/comm_main.c
                       VERBATIM)
    target_link_libraries(comm PRIVATE app_comm)
endif()

if(CONFIG_APP_HEALTH)
    add_executable(health ${CMAKE_CURRENT_BINARY_DIR}/health_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/health_main.c
                       COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/health_main.c
                       VERBATIM)
    target_link_libraries(health PRIVATE app_health)
endif()

if(CONFIG_APP_LOGGER)
    add_executable(logger ${CMAKE_CURRENT_BINARY_DIR}/logger_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/logger_main.c
                       COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/logger_main.c
                       VERBATIM)
    target_link_libraries(logger PRIVATE app_logger)
endif()

if(CONFIG_APP_SUPERVISOR)
    add_executable(supervisor ${CMAKE_CURRENT_BINARY_DIR}/supervisor_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/supervisor_main.c
                       COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/supervisor_main.c
                       VERBATIM)
    target_link_libraries(supervisor PRIVATE app_supervisor)
endif()
