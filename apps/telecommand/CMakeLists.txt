# Telecommand进程（实时进程）

add_executable(telecommand
    telecommand_main.c
)

target_include_directories(telecommand PRIVATE
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/acl/include
    ${CMAKE_SOURCE_DIR}/pdl/include
)

target_link_libraries(telecommand PRIVATE
    acl
    pdl
    osal
    Threads::Threads
    rt
)

# 安装规则
install(TARGETS telecommand
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
