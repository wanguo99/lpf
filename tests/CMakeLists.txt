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
)

# HAL Unit Tests
set(HAL_TEST_SOURCES
    unit/hal/test_hal_can.c
    unit/hal/test_hal_serial.c
    unit/hal/test_hal_i2c.c
    unit/hal/test_hal_spi.c
    unit/hal/test_hal_watchdog.c
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
)

# System Tests (currently empty)
set(SYSTEM_TEST_SOURCES
    # Add system test files here when available
)

# Stress Tests (currently empty)
set(STRESS_TEST_SOURCES
    # Add stress test files here when available
)

# Combine all test sources
set(ALL_TEST_SOURCES
    ${OSAL_TEST_SOURCES}
    ${HAL_TEST_SOURCES}
    ${PCL_TEST_SOURCES}
    ${PDL_TEST_SOURCES}
    ${SYSTEM_TEST_SOURCES}
    ${STRESS_TEST_SOURCES}
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
    osal/test_osal_heap.c
    osal/test_osal_log.c

    # HAL tests
    hal/test_hal_can.c
    hal/test_hal_serial.c
    hal/test_hal_i2c.c
    hal/test_hal_spi.c
    hal/test_hal_watchdog.c

    # PCL tests
    pcl/test_pcl_api.c

    # PDL tests
    pdl/test_pdl_satellite.c
    pdl/test_pdl_mcu.c
    pdl/test_pdl_bmc.c
    pdl/test_pdl_bmc_protocol.c
>>>>>>> 0937567 (新增：实现 HAL 层 Watchdog 驱动和喂狗应用)
)

target_include_directories(ems-test PRIVATE
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

target_link_libraries(ems-test
    ems::osal_public_api
    ems::osal
    ems::hal
    ems::pcl
    ems::pdl
    Threads::Threads
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
    set(INSTALL_BIN_DIR \"\${CMAKE_INSTALL_PREFIX}/bin\")
    set(EMS_TEST_PATH \"\${INSTALL_BIN_DIR}/ems-test\")

    # Create symlinks
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
