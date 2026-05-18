cmake_minimum_required(VERSION 3.16)

project(pmc_logger C)

# 源文件
set(PMC_LOGGER_SOURCES
    src/main.c
    src/pmc_logger.c
)

# 头文件目录
set(PMC_LOGGER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libpmc/include
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/osal/include/core
    ${CMAKE_SOURCE_DIR}/osal/include/ipc
)

# 创建可执行文件
add_executable(pmc_logger ${PMC_LOGGER_SOURCES})

# 设置头文件目录
target_include_directories(pmc_logger PRIVATE ${PMC_LOGGER_INCLUDE_DIRS})

# 链接库
target_link_libraries(pmc_logger PRIVATE
    pmc
    osal
    pthread
    rt
)

# 安装可执行文件
install(TARGETS pmc_logger
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
