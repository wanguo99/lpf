# ============================================================================
# PMC Product CMakeLists.txt
# ============================================================================
# PMC (Central Control Module) product build configuration.
# This file is included by the root CMakeLists.txt when CONFIG_PRODUCT_PMC=y
# ============================================================================

# Register SDK core components
if(CONFIG_OSAL)
    add_subdirectory(${SDK_PATH}/core/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(${SDK_PATH}/core/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# Automatically load platform config based on CONFIG_PROJECT_NAME
load_platform_config(CONFIG_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR}/configs/projects)

# Create platform config object library to avoid path duplication
if(PLATFORM_PCONFIG_SRCS OR PLATFORM_ACONFIG_SRCS)
    add_library(platform_config_obj OBJECT)

    # Add include directories from core modules
    target_include_directories(platform_config_obj PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include         # PMC 产品层头文件（新增）
        ${CMAKE_CURRENT_SOURCE_DIR}/include/pmc     # PMC 产品层头文件子目录
        ${SDK_PATH}/core/pconfig/include
        ${SDK_PATH}/core/pconfig/include/pconfig
        ${SDK_PATH}/core/aconfig/include
        ${SDK_PATH}/core/aconfig/include/aconfig
        ${SDK_PATH}/core/pdl/include
        ${SDK_PATH}/core/pdl/include/pdl
        ${SDK_PATH}/core/hal/include
        ${SDK_PATH}/core/hal/include/hal
        ${SDK_PATH}/core/osal/include
        ${SDK_PATH}/core/osal/include/osal
    )

    # 注意：不需要通过 -D 添加 CONFIG_* 宏，因为 autoconf.h 已经通过 -include 自动包含
    # 重复定义会导致编译错误（-Werror）

    if(PLATFORM_PCONFIG_SRCS)
        target_sources(platform_config_obj PRIVATE ${PLATFORM_PCONFIG_SRCS})
    endif()

    if(PLATFORM_ACONFIG_SRCS)
        target_sources(platform_config_obj PRIVATE ${PLATFORM_ACONFIG_SRCS})
        if(PLATFORM_CONFIG_INCLUDE)
            target_include_directories(platform_config_obj PRIVATE ${PLATFORM_CONFIG_INCLUDE})
        endif()
    endif()
endif()

# Register PCONFIG with platform config
if(CONFIG_PCONFIG)
    # Clear platform sources - they will be linked via object library
    if(PLATFORM_PCONFIG_SRCS)
        set(PCONFIG_PLATFORM_OBJS $<TARGET_OBJECTS:platform_config_obj>)
        set(PCONFIG_EXTRA_SRCS "")  # Don't add sources directly
        set(PCONFIG_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    add_subdirectory(${SDK_PATH}/core/pconfig ${CMAKE_CURRENT_BINARY_DIR}/pconfig)
    # Link platform config objects after pconfig is created
    if(TARGET platform_config_obj AND TARGET pconfig)
        target_sources(pconfig PRIVATE $<TARGET_OBJECTS:platform_config_obj>)

        set(PCONFIG_AUTO_REGISTER_C "${CMAKE_CURRENT_BINARY_DIR}/pconfig_auto_register.c")
        set(PCONFIG_AUTO_REGISTER_SRC [=[
#include "osal.h"
#include "pconfig/pconfig.h"
extern const pconfig_platform_config_t pconfig_@CONFIG_PROJECT_NAME@;
__attribute__((constructor)) static void pconfig_auto_register(void)
{
    if (PCONFIG_init() == OSAL_SUCCESS) {
        (void)PCONFIG_register(&pconfig_@CONFIG_PROJECT_NAME@);
    }
}
]=])
        string(REPLACE "@CONFIG_PROJECT_NAME@" "${CONFIG_PROJECT_NAME}"
               PCONFIG_AUTO_REGISTER_SRC "${PCONFIG_AUTO_REGISTER_SRC}")
        file(WRITE ${PCONFIG_AUTO_REGISTER_C} "${PCONFIG_AUTO_REGISTER_SRC}")
        target_sources(pconfig PRIVATE ${PCONFIG_AUTO_REGISTER_C})
    endif()
endif()

if(CONFIG_PDL)
    add_subdirectory(${SDK_PATH}/core/pdl ${CMAKE_CURRENT_BINARY_DIR}/pdl)
endif()

if(CONFIG_PRL)
    add_subdirectory(${SDK_PATH}/core/prl ${CMAKE_CURRENT_BINARY_DIR}/prl)
endif()

# Register ACONFIG with platform config
if(CONFIG_ACONFIG)
    # Clear platform sources - they will be linked via object library
    if(PLATFORM_ACONFIG_SRCS)
        set(ACONFIG_EXTRA_SRCS "")  # Don't add sources directly
        set(ACONFIG_EXTRA_INCLUDE ${PLATFORM_CONFIG_INCLUDE})
    endif()
    # Core ACONFIG (generic framework)
    add_subdirectory(${SDK_PATH}/core/aconfig ${CMAKE_CURRENT_BINARY_DIR}/aconfig)
    # PMC product-specific ACONFIG implementation
    add_subdirectory(aconfig ${CMAKE_CURRENT_BINARY_DIR}/pmc_aconfig)
    # Link platform config objects after aconfig is created
    if(TARGET platform_config_obj AND TARGET aconfig)
        target_sources(aconfig PRIVATE $<TARGET_OBJECTS:platform_config_obj>)
        if(PLATFORM_CONFIG_INCLUDE)
            target_include_directories(aconfig PUBLIC ${PLATFORM_CONFIG_INCLUDE})
        endif()
    endif()
endif()

# Product-owned runtime entry for configuration initialization.
if(CONFIG_PCONFIG OR CONFIG_ACONFIG)
    add_subdirectory(pmc_runtime ${CMAKE_CURRENT_BINARY_DIR}/pmc_runtime)
endif()

# Register product-specific libraries
if(CONFIG_LIBPMC)
    add_subdirectory(libs/libpmc)
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

# Test application (debug/verification)
if(CONFIG_TEST_CALL_CHAIN)
    add_subdirectory(apps/test_call_chain)
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

# Test application executable
if(CONFIG_TEST_CALL_CHAIN)
    add_executable(test_call_chain_app ${CMAKE_CURRENT_BINARY_DIR}/test_main.c)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_main.c
                       COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_SOURCE_DIR}/apps/test_call_chain/src/main.c
                       ${CMAKE_CURRENT_BINARY_DIR}/test_main.c
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/apps/test_call_chain/src/main.c
                       VERBATIM)
    target_include_directories(test_call_chain_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/test_call_chain/src
    )
    target_link_libraries(test_call_chain_app PRIVATE test_call_chain)
    set_target_properties(test_call_chain_app PROPERTIES OUTPUT_NAME test_call_chain)
endif()

# ============================================================================
# Installation Rules
# ============================================================================
# Install PMC product executables to ${CMAKE_INSTALL_PREFIX}/bin
# Supports DESTDIR for staged installation (Buildroot compatibility)
# Usage: make install DESTDIR=/path/to/staging

if(CONFIG_APP_COLLECTOR)
    install(TARGETS collector
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()

if(CONFIG_APP_COMM)
    install(TARGETS comm
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()

if(CONFIG_APP_HEALTH)
    install(TARGETS health
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()

if(CONFIG_APP_LOGGER)
    install(TARGETS logger
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()

if(CONFIG_APP_SUPERVISOR)
    install(TARGETS supervisor
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()

# Test application installation (optional)
if(CONFIG_TEST_CALL_CHAIN)
    install(TARGETS test_call_chain_app
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        COMPONENT applications
    )
endif()
