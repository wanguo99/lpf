# ACL层（Application Configuration Layer）
# 业务配置层：定义业务功能到硬件设备的映射

# ACL配置选择（默认H200_100P）
set(ACL_CONFIG "H200_100P" CACHE STRING "ACL configuration: H200_100P, pmc_v2, demo")

# ACL核心源文件
set(ACL_SOURCES
    src/acl_api.c
    src/acl_telemetry_cache.c
)

# 根据配置选择对应的配置文件
if(ACL_CONFIG STREQUAL "H200_100P")
    list(APPEND ACL_SOURCES
        configs/H200_100P/acl_h200_100p.c
        configs/H200_100P/acl_h200_100p_invalidation.c
    )
    message(STATUS "ACL配置: H200_100P")
elseif(ACL_CONFIG STREQUAL "pmc_v2")
    list(APPEND ACL_SOURCES
        configs/pmc_v2/acl_pmc_v2.c
        configs/pmc_v2/acl_pmc_v2_invalidation.c
    )
    message(STATUS "ACL配置: PMC v2.0")
elseif(ACL_CONFIG STREQUAL "demo")
    list(APPEND ACL_SOURCES
        configs/demo/acl_demo_v1.c
        configs/demo/acl_demo_v1_invalidation.c
    )
    message(STATUS "ACL配置: Demo")
else()
    message(FATAL_ERROR "未知的ACL配置: ${ACL_CONFIG}")
endif()

# 创建ACL静态库
add_library(acl STATIC ${ACL_SOURCES})

# 包含目录
target_include_directories(acl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/osal/include
        ${CMAKE_SOURCE_DIR}/pdl/include
)

# 链接OSAL库
target_link_libraries(acl
    PRIVATE
        osal
)

# 编译选项
target_compile_options(acl PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
)

# 安装规则
install(TARGETS acl
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/acl
    FILES_MATCHING PATTERN "*.h"
)
