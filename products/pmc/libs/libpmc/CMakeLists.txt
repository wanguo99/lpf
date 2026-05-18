# libpmc - PMC应用公共库

# 源文件
set(LIBPMC_SOURCES
    src/libpmc_ipc.c
    src/libpmc_protocol.c
)

# 创建静态库
add_library(pmc STATIC ${LIBPMC_SOURCES})

# 设置头文件目录
target_include_directories(pmc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core/osal/include
)

# 链接OSAL库
target_link_libraries(pmc
    PUBLIC
        ems::osal
)

# 安装库
install(TARGETS pmc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libpmc
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "libpmc configured")
