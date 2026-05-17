# Performance tests CMakeLists.txt

# 性能测试核心库
add_library(perf_tests STATIC
    test_perf_core.c
)

target_include_directories(perf_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
)

target_link_libraries(perf_tests
    osal
    m  # 数学库（sqrt等）
)

# OSAL性能测试
add_library(perf_osal_tests STATIC
    osal/test_perf_osal.c
)

target_include_directories(perf_osal_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
    ${CMAKE_SOURCE_DIR}/osal/include
)

target_link_libraries(perf_osal_tests
    perf_tests
    osal
    tests_core
)
