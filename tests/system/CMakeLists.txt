# System tests CMakeLists.txt

# 系统测试核心库
add_library(system_tests STATIC
    test_system_core.c
)

target_include_directories(system_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
)

target_link_libraries(system_tests
    osal
)

# 系统集成测试
add_library(system_integration_tests STATIC
    integration/test_system_integration.c
)

target_include_directories(system_integration_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
    ${CMAKE_SOURCE_DIR}/hal/include
    ${CMAKE_SOURCE_DIR}/pdl/include
)

target_link_libraries(system_integration_tests
    system_tests
    osal
    hal
    pdl
    tests_core
)
