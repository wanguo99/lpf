# ACL层（Application Configuration Layer）
# 业务配置层：定义业务功能到硬件设备的映射

# ACL核心源文件（仅包含框架代码）
set(ACL_SOURCES
    src/acl_api.c
    src/acl_telemetry_cache.c
)

# ACL配置源文件由项目层提供（通过ACL_CONFIG_SOURCES变量）
# 项目层应该在包含此文件前设置：
#   set(ACL_CONFIG_SOURCES
#       ${PROJECT_SOURCE_DIR}/acl/acl_xxx.c
#       ${PROJECT_SOURCE_DIR}/acl/acl_xxx_invalidation.c
#   )
if(DEFINED ACL_CONFIG_SOURCES)
    list(APPEND ACL_SOURCES ${ACL_CONFIG_SOURCES})
    message(STATUS "ACL configured with project-specific sources")
else()
    message(WARNING "No ACL_CONFIG_SOURCES defined - building ACL framework only")
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
