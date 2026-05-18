cmake_minimum_required(VERSION 3.16)

project(pmc_health C)

# 源文件
set(PMC_HEALTH_SOURCES
    src/main.c
    src/pmc_health.c
)

# 头文件目录
set(PMC_HEALTH_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libpmc/include
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/osal/include/core
    ${CMAKE_SOURCE_DIR}/osal/include/ipc
)

# 创建可执行文件
add_executable(pmc_health ${PMC_HEALTH_SOURCES})

# 设置头文件目录
target_include_directories(pmc_health PRIVATE ${PMC_HEALTH_INCLUDE_DIRS})

# 链接库
target_link_libraries(pmc_health PRIVATE
    pmc
    osal
    pthread
    rt
)

# 安装可执行文件
install(TARGETS pmc_health
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
