# PMC H200 项目构建配置

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
# 应用程序构建选项
# ============================================================================
option(BUILD_PMC_SUPERVISOR "Build PMC supervisor application" ON)
option(BUILD_PMC_COLLECTOR "Build PMC collector application" ON)
option(BUILD_PMC_COMM "Build PMC communication application" ON)
option(BUILD_PMC_HEALTH "Build PMC health monitor application" ON)
option(BUILD_PMC_LOGGER "Build PMC logger application" ON)

# ============================================================================
# 应用程序
# ============================================================================
if(BUILD_PMC_SUPERVISOR)
    add_subdirectory(apps/pmc_supervisor)
endif()

if(BUILD_PMC_COLLECTOR)
    add_subdirectory(apps/pmc_collector)
endif()

if(BUILD_PMC_COMM)
    add_subdirectory(apps/pmc_comm)
endif()

if(BUILD_PMC_HEALTH)
    add_subdirectory(apps/pmc_health)
endif()

if(BUILD_PMC_LOGGER)
    add_subdirectory(apps/pmc_logger)
endif()

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS pmc_h200_pcl_config pmc_h200_acl_config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
