# PMC H200_AM625 项目构建配置

# ============================================================================
# 产品线和项目宏定义
# ============================================================================
add_compile_definitions(
    PRODUCT_PMC
    PROJECT_H200_AM625
)

# ============================================================================
# 产品线级别的库
# ============================================================================
add_subdirectory(../libs ${CMAKE_CURRENT_BINARY_DIR}/libs)

# ============================================================================
# PCL 配置层
# ============================================================================
set(PMC_H200_PCL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_h200_100p_base.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_h200_100p_v1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_h200_100p_v2.c
)

add_library(pmc_h200_pcl_config STATIC ${PMC_H200_PCL_SOURCES})
target_link_libraries(pmc_h200_pcl_config
    PUBLIC ems::pcl_public_api
    PUBLIC ems::hal_public_api
    PRIVATE ems::osal
)

# ============================================================================
# ACL 配置层
# ============================================================================
set(PMC_H200_ACL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/acl/pmc_acl_config.c
)

add_library(pmc_h200_acl_config STATIC ${PMC_H200_ACL_SOURCES})
target_link_libraries(pmc_h200_acl_config
    PUBLIC ems::acl
    PRIVATE ems::osal
)

# ============================================================================
# 应用程序构建选项（从 Kconfig 读取）
# ============================================================================
set(KCONFIG_HEADER "${CMAKE_SOURCE_DIR}/include/generated/autoconf.h")

if(EXISTS ${KCONFIG_HEADER})
    # 从 Kconfig 读取应用构建选项
    kconfig_is_set(CONFIG_BUILD_PMC_SUPERVISOR BUILD_PMC_SUPERVISOR)
    kconfig_is_set(CONFIG_BUILD_PMC_COLLECTOR BUILD_PMC_COLLECTOR)
    kconfig_is_set(CONFIG_BUILD_PMC_COMM BUILD_PMC_COMM)
    kconfig_is_set(CONFIG_BUILD_PMC_HEALTH BUILD_PMC_HEALTH)
    kconfig_is_set(CONFIG_BUILD_PMC_LOGGER BUILD_PMC_LOGGER)
else()
    # 回退：使用 CMake option
    option(BUILD_PMC_SUPERVISOR "Build PMC supervisor application" ON)
    option(BUILD_PMC_COLLECTOR "Build PMC collector application" ON)
    option(BUILD_PMC_COMM "Build PMC communication application" ON)
    option(BUILD_PMC_HEALTH "Build PMC health monitor application" ON)
    option(BUILD_PMC_LOGGER "Build PMC logger application" ON)
endif()

# ============================================================================
# 应用程序（位于产品线级别的 apps/ 目录）
# ============================================================================
if(BUILD_PMC_SUPERVISOR)
    add_subdirectory(../apps/pmc_supervisor ${CMAKE_CURRENT_BINARY_DIR}/apps/pmc_supervisor)
    message(STATUS "PMC: Building supervisor application")
endif()

if(BUILD_PMC_COLLECTOR)
    add_subdirectory(../apps/pmc_collector ${CMAKE_CURRENT_BINARY_DIR}/apps/pmc_collector)
    message(STATUS "PMC: Building collector application")
endif()

if(BUILD_PMC_COMM)
    add_subdirectory(../apps/pmc_comm ${CMAKE_CURRENT_BINARY_DIR}/apps/pmc_comm)
    message(STATUS "PMC: Building comm application")
endif()

if(BUILD_PMC_HEALTH)
    add_subdirectory(../apps/pmc_health ${CMAKE_CURRENT_BINARY_DIR}/apps/pmc_health)
    message(STATUS "PMC: Building health application")
endif()

if(BUILD_PMC_LOGGER)
    add_subdirectory(../apps/pmc_logger ${CMAKE_CURRENT_BINARY_DIR}/apps/pmc_logger)
    message(STATUS "PMC: Building logger application")
endif()

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS pmc_h200_pcl_config pmc_h200_acl_config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
