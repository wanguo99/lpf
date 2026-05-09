# Watchdog应用
# 看门狗喂狗应用

# 目录变量
set(WATCHDOG_APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 源文件
set(WATCHDOG_APP_SOURCES
    ${WATCHDOG_APP_SRC_DIR}/main.c
)

# 创建可执行文件
add_executable(watchdog_app ${WATCHDOG_APP_SOURCES})

# 链接依赖
target_link_libraries(watchdog_app
    ems::osal
    ems::pdl
    Threads::Threads
)

# 安装规则（使用 GNUInstallDirs 变量）
install(TARGETS watchdog_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Watchdog app configured")
