# Stress tests CMakeLists.txt

# 压力测试核心库
add_library(stress_tests STATIC
    test_stress_core.c
)

target_include_directories(stress_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
)

target_link_libraries(stress_tests
    osal
)

# OSAL压力测试
add_library(stress_osal_tests STATIC
    osal/test_stress_osal.c
)

target_include_directories(stress_osal_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
)

target_link_libraries(stress_osal_tests
    stress_tests
    osal
    tests_core
)
