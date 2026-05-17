# Telemetry进程（后台进程）

add_executable(telemetry
    telemetry_main.c
)

target_include_directories(telemetry PRIVATE
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/acl/include
    ${CMAKE_SOURCE_DIR}/pdl/include
)

target_link_libraries(telemetry PRIVATE
    acl
    pdl
    osal
    Threads::Threads
)

# 安装规则
install(TARGETS telemetry
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
