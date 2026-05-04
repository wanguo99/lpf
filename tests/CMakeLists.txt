# Unified Test Framework
# All test code compiled directly into executable

cmake_minimum_required(VERSION 3.10)

if(NOT BUILD_TESTING)
    return()
endif()

message(STATUS "Tests configured (unified test framework)")

# ============================================================================
# Unified Test Runner Executable
# ============================================================================
add_executable(unit-test
    # Test framework core
    core/main.c
    core/test_runner.c
    core/test_registry.c
    core/test_menu.c

    # OSAL tests
    osal/test_osal_task.c
    osal/test_osal_queue.c
    osal/test_osal_mutex.c
    osal/test_osal_signal.c
    osal/test_osal_atomic.c
    osal/test_osal_string.c
    osal/test_osal_file.c
    osal/test_osal_time.c
    osal/test_osal_clock.c
    osal/test_osal_env.c
    osal/test_osal_errno.c
    osal/test_osal_version.c
    osal/test_osal_heap.c
    osal/test_osal_log.c
    osal/test_osal_resource_tracker.c

    # HAL tests
    hal/test_hal_can.c
    hal/test_hal_serial.c
    hal/test_hal_i2c.c
    hal/test_hal_spi.c

    # PCL tests
    pcl/test_pcl_api.c

    # PDL tests
    pdl/test_pdl_satellite.c
    pdl/test_pdl_mcu.c
    pdl/test_pdl_bmc.c
    pdl/test_pdl_bmc_protocol.c
)

target_include_directories(unit-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../osal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../pcl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../pcl/include/api
    ${CMAKE_CURRENT_SOURCE_DIR}/../pcl/include/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/../pcl/include/peripheral
    ${CMAKE_CURRENT_SOURCE_DIR}/../pdl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../pdl/src/pdl_bmc
)

target_compile_definitions(unit-test PRIVATE
    OSAL_ENABLE_RESOURCE_TRACKING
)

target_link_libraries(unit-test
    ems::osal_public_api
    ems::osal
    ems::hal
    ems::pcl
    ems::pdl
    Threads::Threads
)

# macOS 不需要 librt
if(NOT APPLE)
    target_link_libraries(unit-test rt)
endif()

# 输出目录由顶层 CMakeLists.txt 的 CMAKE_RUNTIME_OUTPUT_DIRECTORY 控制

# ============================================================================
# Custom Test Targets
# ============================================================================
add_custom_target(run_tests
    COMMAND $<TARGET_FILE:unit-test> --all
    DEPENDS unit-test
    COMMENT "Running all unit tests"
)

add_custom_target(run_tests_interactive
    COMMAND $<TARGET_FILE:unit-test> --interactive
    DEPENDS unit-test
    COMMENT "Running tests in interactive mode"
)
