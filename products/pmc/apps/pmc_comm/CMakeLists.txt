cmake_minimum_required(VERSION 3.16)

project(pmc_comm C)

# 源文件
set(PMC_COMM_SOURCES
    src/main.c
    src/pmc_comm.c
)

# 头文件目录
set(PMC_COMM_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libpmc/include
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/osal/include/core
    ${CMAKE_SOURCE_DIR}/osal/include/ipc
)

# 创建可执行文件
add_executable(pmc_comm ${PMC_COMM_SOURCES})

# 设置头文件目录
target_include_directories(pmc_comm PRIVATE ${PMC_COMM_INCLUDE_DIRS})

# 链接库
target_link_libraries(pmc_comm PRIVATE
    pmc
    osal
    pthread
    rt
)

# 安装可执行文件
install(TARGETS pmc_comm
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
