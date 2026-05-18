# Unified Test Framework
# All test code compiled directly into executable

cmake_minimum_required(VERSION 3.10)

if(NOT BUILD_TESTING)
    return()
endif()

message(STATUS "Tests configured (unified test framework)")

# ============================================================================
# Collect Test Sources - Explicitly Listed for Better Control
# ============================================================================

# OSAL Unit Tests
set(OSAL_TEST_SOURCES
    unit/osal/test_osal_version.c
    unit/osal/test_osal_atomic.c
    unit/osal/test_osal_string.c
    unit/osal/test_osal_time.c
    unit/osal/test_osal_heap.c
    unit/osal/test_osal_log.c
    unit/osal/test_osal_errno.c
    unit/osal/test_osal_env.c
    unit/osal/test_osal_file.c
    unit/osal/test_osal_clock.c
    unit/osal/test_osal_mutex.c
    unit/osal/test_osal_semaphore.c
    unit/osal/test_osal_cond.c
    unit/osal/test_osal_signal.c
    unit/osal/test_osal_stress.c
    unit/osal/test_osal_sched.c
    unit/osal/test_osal_shm.c
    unit/osal/test_osal_thread.c
    unit/osal/test_osal_process.c
)

# HAL Unit Tests
set(HAL_TEST_SOURCES
    unit/hal/test_hal_can.c
    unit/hal/test_hal_serial.c
    unit/hal/test_hal_i2c.c
    unit/hal/test_hal_spi.c
    unit/hal/test_hal_watchdog.c
    unit/hal/test_hal_gpio.c
)

# PCL Unit Tests
set(PCL_TEST_SOURCES
    unit/pcl/test_pcl_api.c
)

# PDL Unit Tests
set(PDL_TEST_SOURCES
    unit/pdl/test_pdl_bmc.c
    unit/pdl/test_pdl_bmc_protocol.c
    unit/pdl/test_pdl_mcu.c
    unit/pdl/test_pdl_satellite.c
    unit/pdl/test_pdl_watchdog.c
)

# Performance Tests
set(PERFORMANCE_TEST_SOURCES
    performance/test_perf_core.c
    performance/osal/test_perf_osal.c
    performance/hal/test_perf_hal.c
    performance/pcl/test_perf_pcl.c
    performance/acl/test_perf_acl.c
    performance/pdl/test_perf_pdl.c
)

# Stress Tests
set(STRESS_TEST_SOURCES
    stress/test_stress_core.c
    stress/osal/test_stress_osal.c
    stress/hal/test_stress_hal.c
    stress/pcl/test_stress_pcl.c
    stress/acl/test_stress_acl.c
    stress/pdl/test_stress_pdl.c
)

# System Tests
set(SYSTEM_TEST_SOURCES
    system/test_system_core.c
    system/integration/test_system_integration.c
    system/osal/test_system_osal.c
    system/hal/test_system_hal.c
    system/pcl/test_system_pcl.c
    system/acl/test_system_acl.c
    system/pdl/test_system_pdl.c
)

# Combine all test sources
set(ALL_TEST_SOURCES
    ${OSAL_TEST_SOURCES}
    ${HAL_TEST_SOURCES}
    ${PCL_TEST_SOURCES}
    ${PDL_TEST_SOURCES}
    ${PERFORMANCE_TEST_SOURCES}
    ${STRESS_TEST_SOURCES}
    ${SYSTEM_TEST_SOURCES}
)

# ============================================================================
# Unified Test Runner Executable (renamed to ems-test)
# ============================================================================
add_executable(ems-test
    # Test framework core
    core/main.c
    core/test_runner.c
    core/test_registry.c
    core/test_menu.c

    # All test sources
    ${ALL_TEST_SOURCES}
)

target_include_directories(ems-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/osal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/hal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pcl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pcl/include/api
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pcl/include/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pcl/include/peripheral
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/acl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pdl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/pdl/src/pdl_bmc
)

target_link_libraries(ems-test
    ems::osal_public_api
    ems::osal
    ems::hal
    ems::pcl
    ems::acl
    ems::pdl
    pmc_h200_pcl_config
    pmc_h200_acl_config
    Threads::Threads
    m  # 数学库（性能测试需要sqrt等函数）
)

# macOS 不需要 librt
if(NOT APPLE)
    target_link_libraries(ems-test rt)
endif()

# 输出目录由顶层 CMakeLists.txt 的 CMAKE_RUNTIME_OUTPUT_DIRECTORY 控制

# ============================================================================
# Install and Create Symlinks (Busybox-style)
# ============================================================================
install(TARGETS ems-test
    RUNTIME DESTINATION bin
)

# Create symlinks for module-specific test runners
install(CODE "
    # Handle DESTDIR for cross-compilation (e.g., Buildroot)
    if(DEFINED ENV{DESTDIR})
        set(INSTALL_BIN_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")
    else()
        set(INSTALL_BIN_DIR \"\${CMAKE_INSTALL_PREFIX}/bin\")
    endif()

    # Create symlinks (use relative path for portability)
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink ems-test \${INSTALL_BIN_DIR}/osal-test
    )
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink ems-test \${INSTALL_BIN_DIR}/hal-test
    )
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink ems-test \${INSTALL_BIN_DIR}/pcl-test
    )
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink ems-test \${INSTALL_BIN_DIR}/pdl-test
    )

    message(STATUS \"Created symlinks: osal-test, hal-test, pcl-test, pdl-test -> ems-test\")
")

# ============================================================================
# Custom Test Targets
# ============================================================================
add_custom_target(run_tests
    COMMAND $<TARGET_FILE:ems-test> --all
    DEPENDS ems-test
    COMMENT "Running all unit tests"
)

add_custom_target(run_tests_interactive
    COMMAND $<TARGET_FILE:ems-test> --interactive
    DEPENDS ems-test
    COMMENT "Running tests in interactive mode"
)
