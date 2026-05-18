cmake_minimum_required(VERSION 3.16)

project(libpmc C)

# 源文件
set(LIBPMC_SOURCES
    src/libpmc_ipc.c
    src/libpmc_protocol.c
)

# 头文件目录
set(LIBPMC_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/osal/include/core
    ${CMAKE_SOURCE_DIR}/osal/include/ipc
)

# 创建静态库
add_library(pmc STATIC ${LIBPMC_SOURCES})

# 设置头文件目录
target_include_directories(pmc PUBLIC ${LIBPMC_INCLUDE_DIRS})

# 链接OSAL库
target_link_libraries(pmc PUBLIC osal)

# 安装库
install(TARGETS pmc
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION include/libpmc
    FILES_MATCHING PATTERN "*.h"
)
