# Samples 项目构建配置

# ============================================================================
# 产品线和项目宏定义
# ============================================================================
add_compile_definitions(
    PRODUCT=samples
    PRODUCT_PROJECT=project_demo
)

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
# ACL 配置层
# ============================================================================
file(GLOB SAMPLES_ACL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/acl/demo/*.c
)

set(ACL_CONFIG_SOURCES ${SAMPLES_ACL_SOURCES} CACHE INTERNAL "ACL configuration sources from project")

# ============================================================================
# 应用程序构建选项
# ============================================================================
option(BUILD_PRODUCT_DEMO_APP "Build product demo application" ON)

# ============================================================================
# 应用程序
# ============================================================================
if(BUILD_PRODUCT_DEMO_APP)
    add_subdirectory(../apps/product_demo_app ${CMAKE_CURRENT_BINARY_DIR}/apps/product_demo_app)
endif()

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS samples_pcl_config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
