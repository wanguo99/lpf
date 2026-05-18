# Product Demo应用
#

# 目录变量
set(PRODUCT_DEMO_APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 源文件
set(PRODUCT_DEMO_APP_SOURCES
    ${PRODUCT_DEMO_APP_SRC_DIR}/main.c
)

# 创建可执行文件
add_executable(product_demo_app ${PRODUCT_DEMO_APP_SOURCES})

# 链接依赖（使用命名空间别名）
target_link_libraries(product_demo_app
    ems::osal
    Threads::Threads
)

# 输出目录由顶层 CMakeLists.txt 的 CMAKE_RUNTIME_OUTPUT_DIRECTORY 控制

# 安装规则（使用 GNUInstallDirs 变量）
install(TARGETS product_demo_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Product demo app configured")
