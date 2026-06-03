# ============================================================================
# EMS Test Suite CMakeLists.txt
# ============================================================================
# This builds the test suite as a special "product".
# Tests are conditionally compiled based on Kconfig configuration.
# ============================================================================

# ============================================================================
# Register SDK Core Components (like other products do)
# ============================================================================
# Build core modules required by tests
# Note: Check if target already exists to avoid duplicate registration
# when tests are built alongside products

if(CONFIG_OSAL AND NOT TARGET osal)
    add_subdirectory(${SDK_PATH}/core/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL AND NOT TARGET hal)
    add_subdirectory(${SDK_PATH}/core/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

if(CONFIG_PCL AND NOT TARGET pconfig)
    add_subdirectory(${SDK_PATH}/core/pconfig ${CMAKE_CURRENT_BINARY_DIR}/pconfig)
endif()

if(CONFIG_PDL AND NOT TARGET pdl)
    add_subdirectory(${SDK_PATH}/core/pdl ${CMAKE_CURRENT_BINARY_DIR}/pdl)
endif()

if(CONFIG_PRL AND NOT TARGET prl)
    add_subdirectory(${SDK_PATH}/core/prl ${CMAKE_CURRENT_BINARY_DIR}/prl)
endif()

if(CONFIG_ACL AND NOT TARGET aconfig)
    add_subdirectory(${SDK_PATH}/core/aconfig ${CMAKE_CURRENT_BINARY_DIR}/aconfig)
endif()

# ============================================================================
# Test Framework Library (libutest)
# ============================================================================
# Core test framework - always compiled when BUILD_TESTING is enabled

add_library(libutest STATIC)

# Framework core source files
target_sources(libutest PRIVATE
    core/test_registry.c
    core/test_runner.c
    core/test_menu.c
    core/test_mock.c
)

# Framework include directories
target_include_directories(libutest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add OSAL headers (required by test framework)
target_include_directories(libutest PRIVATE
    ${SDK_PATH}/core/osal/include
    ${SDK_PATH}/core/types/include
)

# Framework dependencies - only OSAL
if(CONFIG_OSAL)
    target_link_libraries(libutest PUBLIC
        osal
    )
endif()

# ============================================================================
# Test Executable (ems-test)
# ============================================================================
# Main entry point + conditionally compiled test cases

add_executable(ems-test
    core/main.c
)

# Link test framework
target_link_libraries(ems-test PRIVATE
    libutest
)

# Link OSAL (required by test framework)
if(CONFIG_OSAL)
    target_link_libraries(ems-test PRIVATE osal)
endif()

# Add include directories for test cases
target_include_directories(ems-test PRIVATE
    ${SDK_PATH}/core/osal/include
    ${SDK_PATH}/core/types/include
    ${SDK_PATH}/core/hal/include
    ${SDK_PATH}/core/pconfig/include
    ${SDK_PATH}/core/pdl/include
    ${SDK_PATH}/core/prl/api
    ${SDK_PATH}/core/prl/include
    ${SDK_PATH}/core/aconfig/include
)

# ============================================================================
# Unit Tests - Conditionally compiled based on CONFIG_TEST_*
# ============================================================================

if(CONFIG_TEST_UNIT)
    message(STATUS "Enabling unit tests")

    # OSAL Unit Tests
    if(CONFIG_TEST_OSAL)
        message(STATUS "  - OSAL unit tests")
        target_sources(ems-test PRIVATE
            unit/osal/test_osal_atomic.c
            unit/osal/test_osal_clock.c
            unit/osal/test_osal_cond.c
            unit/osal/test_osal_env.c
            unit/osal/test_osal_errno.c
            unit/osal/test_osal_file.c
            unit/osal/test_osal_heap.c
            unit/osal/test_osal_log.c
            unit/osal/test_osal_mutex.c
            unit/osal/test_osal_process.c
            unit/osal/test_osal_sched.c
            unit/osal/test_osal_semaphore.c
            unit/osal/test_osal_shm.c
            unit/osal/test_osal_signal.c
            unit/osal/test_osal_stress.c
            unit/osal/test_osal_string.c
            unit/osal/test_osal_thread.c
            unit/osal/test_osal_time.c
            unit/osal/test_osal_version.c
        )
    endif()

    # HAL Unit Tests
    if(CONFIG_TEST_HAL)
        message(STATUS "  - HAL unit tests")
        target_sources(ems-test PRIVATE
            unit/hal/test_hal_can.c
            unit/hal/test_hal_gpio.c
            unit/hal/test_hal_i2c.c
            unit/hal/test_hal_serial.c
            unit/hal/test_hal_spi.c
            unit/hal/test_hal_watchdog.c
        )
        target_link_libraries(ems-test PRIVATE hal)
    endif()

    # PCL Unit Tests
    if(CONFIG_TEST_PCL)
        message(STATUS "  - PCL unit tests")
        target_sources(ems-test PRIVATE
            unit/pcl/test_pcl_api.c
        )
        target_link_libraries(ems-test PRIVATE pcl)
    endif()

    # PDL Unit Tests
    if(CONFIG_TEST_PDL)
        message(STATUS "  - PDL unit tests")
        target_sources(ems-test PRIVATE
            unit/pdl/test_pdl_bmc.c
            # unit/pdl/test_pdl_bmc_protocol.c  # TODO: 需要修复内部 API 引用
            unit/pdl/test_pdl_mcu.c
            unit/pdl/test_pdl_satellite.c
            unit/pdl/test_pdl_watchdog.c
        )
        target_link_libraries(ems-test PRIVATE pdl)
        # Add PDL internal headers for protocol tests
        target_include_directories(ems-test PRIVATE
            ${SDK_PATH}/core/pdl/src/pdl_bmc
            ${SDK_PATH}/core/pdl/src/pdl_mcu
            ${SDK_PATH}/core/pdl/src/pdl_satellite
        )
    endif()

    # PRL Unit Tests (added to Kconfig if not present)
    if(CONFIG_TEST_PRL)
        message(STATUS "  - PRL unit tests")
        target_sources(ems-test PRIVATE
            unit/prl/test_prl_common.c
            unit/prl/test_prl_device.c
        )
        target_link_libraries(ems-test PRIVATE prl)
    endif()

    # ACONFIG Unit Tests
    if(CONFIG_TEST_ACONFIG)
        message(STATUS "  - ACONFIG unit tests")
        target_sources(ems-test PRIVATE
            unit/aconfig/test_aconfig_api.c
        )
        target_link_libraries(ems-test PRIVATE aconfig)
    endif()

    # PCONFIG Unit Tests
    if(CONFIG_TEST_PCONFIG)
        message(STATUS "  - PCONFIG unit tests")
        target_sources(ems-test PRIVATE
            unit/pconfig/test_pconfig_api.c
        )
        target_link_libraries(ems-test PRIVATE pconfig)
    endif()

    # ACL Unit Tests
    if(CONFIG_TEST_ACL)
        message(STATUS "  - ACL unit tests")
        # Note: ACL test files to be added when available
        # target_sources(ems-test PRIVATE
        #     unit/acl/test_acl_api.c
        # )
        # target_link_libraries(ems-test PRIVATE acl)
    endif()

endif() # CONFIG_TEST_UNIT

# ============================================================================
# Performance Tests
# ============================================================================

if(CONFIG_TEST_PERFORMANCE)
    message(STATUS "Enabling performance tests")

    target_sources(ems-test PRIVATE
        performance/test_perf_core.c
    )

    if(CONFIG_TEST_PERFORMANCE_OSAL)
        message(STATUS "  - OSAL performance tests")
        target_sources(ems-test PRIVATE
            performance/osal/test_perf_osal.c
        )
    endif()

    if(CONFIG_TEST_PERFORMANCE_HAL)
        message(STATUS "  - HAL performance tests")
        target_sources(ems-test PRIVATE
            performance/hal/test_perf_hal.c
        )
    endif()

    if(CONFIG_TEST_PERFORMANCE_PDL)
        message(STATUS "  - PDL performance tests")
        target_sources(ems-test PRIVATE
            performance/pdl/test_perf_pdl.c
        )
    endif()

    if(CONFIG_TEST_PERFORMANCE_PCL)
        message(STATUS "  - PCL performance tests")
        # target_sources(ems-test PRIVATE
        #     performance/pcl/test_perf_pcl.c
        # )
    endif()

    if(CONFIG_TEST_PERFORMANCE_ACL)
        message(STATUS "  - ACL performance tests")
        # target_sources(ems-test PRIVATE
        #     performance/acl/test_perf_acl.c
        # )
    endif()

endif() # CONFIG_TEST_PERFORMANCE

# ============================================================================
# Stress Tests
# ============================================================================

if(CONFIG_TEST_STRESS)
    message(STATUS "Enabling stress tests")

    target_sources(ems-test PRIVATE
        stress/test_stress_core.c
    )

    if(CONFIG_TEST_STRESS_OSAL)
        message(STATUS "  - OSAL stress tests")
        target_sources(ems-test PRIVATE
            stress/osal/test_stress_osal.c
        )
    endif()

    if(CONFIG_TEST_STRESS_HAL)
        message(STATUS "  - HAL stress tests")
        target_sources(ems-test PRIVATE
            stress/hal/test_stress_hal.c
        )
    endif()

    if(CONFIG_TEST_STRESS_PDL)
        message(STATUS "  - PDL stress tests")
        target_sources(ems-test PRIVATE
            stress/pdl/test_stress_pdl.c
        )
    endif()

    if(CONFIG_TEST_STRESS_PCL)
        message(STATUS "  - PCL stress tests")
        # target_sources(ems-test PRIVATE
        #     stress/pcl/test_stress_pcl.c
        # )
    endif()

    if(CONFIG_TEST_STRESS_ACL)
        message(STATUS "  - ACL stress tests")
        # target_sources(ems-test PRIVATE
        #     stress/acl/test_stress_acl.c
        # )
    endif()

endif() # CONFIG_TEST_STRESS

# ============================================================================
# System Tests
# ============================================================================

if(CONFIG_TEST_SYSTEM)
    message(STATUS "Enabling system tests")

    target_sources(ems-test PRIVATE
        system/test_system_core.c
    )

    # System tests need core libraries - link them in correct order (dependencies first)
    if(TARGET osal)
        target_link_libraries(ems-test PRIVATE osal)
    endif()
    if(TARGET hal)
        target_link_libraries(ems-test PRIVATE hal)
    endif()
    if(TARGET pdl)
        target_link_libraries(ems-test PRIVATE pdl)
    endif()
    if(TARGET prl)
        target_link_libraries(ems-test PRIVATE prl)
    endif()

    if(CONFIG_TEST_SYSTEM_OSAL)
        message(STATUS "  - OSAL system tests")
        target_sources(ems-test PRIVATE
            system/osal/test_system_osal.c
        )
    endif()

    if(CONFIG_TEST_SYSTEM_HAL)
        message(STATUS "  - HAL system tests")
        target_sources(ems-test PRIVATE
            system/hal/test_system_hal.c
        )
        # Create separate executable for concurrent access test (has its own main)
        add_executable(test-hal-concurrent
            system/hal/test_hal_concurrent_access.c
        )
        target_link_libraries(test-hal-concurrent PRIVATE osal pthread rt m)
        set_target_properties(test-hal-concurrent PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()

    if(CONFIG_TEST_SYSTEM_PDL)
        message(STATUS "  - PDL system tests")
        target_sources(ems-test PRIVATE
            system/pdl/test_system_pdl.c
        )
    endif()

    if(CONFIG_TEST_SYSTEM_PCL)
        message(STATUS "  - PCL system tests")
        # target_sources(ems-test PRIVATE
        #     system/pcl/test_system_pcl.c
        # )
    endif()

    if(CONFIG_TEST_SYSTEM_ACL)
        message(STATUS "  - ACL system tests")
        # target_sources(ems-test PRIVATE
        #     system/acl/test_system_acl.c
        # )
    endif()

    if(CONFIG_TEST_SYSTEM_ACONFIG)
        message(STATUS "  - ACONFIG system tests")
        target_sources(ems-test PRIVATE
            system/aconfig/test_system_aconfig.c
        )
    endif()

    if(CONFIG_TEST_SYSTEM_PCONFIG)
        message(STATUS "  - PCONFIG system tests")
        target_sources(ems-test PRIVATE
            system/pconfig/test_system_pconfig.c
        )
    endif()

    if(CONFIG_TEST_INTEGRATION)
        message(STATUS "  - Integration tests")
        target_sources(ems-test PRIVATE
            system/integration/test_system_integration.c
        )
        # Libraries already linked at system test level
    endif()

endif() # CONFIG_TEST_SYSTEM

# ============================================================================
# Additional test dependencies
# ============================================================================

# Link pthread and rt for threading tests
target_link_libraries(ems-test PRIVATE
    pthread
    rt
    m
)

# Force re-link osal at the end to resolve symbols
# This ensures OSAL symbols are available for all tests
if(TARGET osal)
    target_link_libraries(ems-test PRIVATE osal)
endif()

# Set output directory
set_target_properties(ems-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS ems-test
    RUNTIME DESTINATION bin
)

message(STATUS "Test suite configuration complete")
