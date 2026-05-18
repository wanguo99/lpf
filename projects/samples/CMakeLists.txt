# Samples 项目构建配置

# ============================================================================
# PCL 配置层
# ============================================================================
set(SAMPLES_PCL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_product_demo_base.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_product_demo_v1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pcl/pcl_product_demo_v2.c
)

add_library(samples_pcl_config STATIC ${SAMPLES_PCL_SOURCES})
target_link_libraries(samples_pcl_config
    PUBLIC ems::pcl_public_api
    PRIVATE ems::osal
)

# ============================================================================
# 应用程序构建选项
# ============================================================================
option(BUILD_SAMPLE_APP "Build sample application" ON)

# ============================================================================
# 应用程序
# ============================================================================
if(BUILD_SAMPLE_APP)
    add_subdirectory(apps/sample_app)
endif()

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS samples_pcl_config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
