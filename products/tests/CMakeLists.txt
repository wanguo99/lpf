# ============================================================================
# ES-Middleware Test Suite CMakeLists.txt
# ============================================================================
# This builds the test suite as a special "product".
# Tests are conditionally compiled based on Config.in configuration.
#
# Test Discovery System:
# - Test files are automatically discovered and matched to Kconfig options
# - No manual file list maintenance required
# - Convention: test_<module>_<name>.c --> CONFIG_TEST_<MODULE>_<NAME>
# ============================================================================

# Include test discovery functions
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestDiscovery.cmake)

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
    core/test_metadata.c
    core/test_reporter.c
)

# Framework include directories
target_include_directories(libutest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add OSAL headers (required by test framework)
target_include_directories(libutest PRIVATE
    ${SDK_PATH}/core/osal/include/osal
)

# Framework dependencies - only OSAL
if(CONFIG_OSAL)
    target_link_libraries(libutest PUBLIC
        osal
    )
endif()

# ============================================================================
# Test Executable (es-middleware-test)
# ============================================================================
# Main entry point + conditionally compiled test cases

add_executable(es-middleware-test
    core/main.c
)

# Link test framework
target_link_libraries(es-middleware-test PRIVATE
    libutest
)

# Link OSAL (required by test framework)
if(CONFIG_OSAL)
    target_link_libraries(es-middleware-test PRIVATE osal)
endif()

# Add include directories for test cases
target_include_directories(es-middleware-test PRIVATE
    ${SDK_PATH}/core/osal/include/osal
    ${SDK_PATH}/core/hal/include/hal
    ${SDK_PATH}/core/pconfig/include/pconfig
    ${SDK_PATH}/core/pdl/include/pdl
    ${SDK_PATH}/core/prl/include/prl
    ${SDK_PATH}/core/aconfig/include/aconfig
)

# ============================================================================
# Unit Tests - Auto-discovery based on Config.in
# ============================================================================

if(CONFIG_TEST_UNIT)
    message(STATUS "Enabling unit tests")

    # OSAL Unit Tests - Auto-discover all test_osal_*.c files
    if(CONFIG_TEST_OSAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/osal
            MODULE osal
            LIBRARY osal
        )
    endif()

    # HAL Unit Tests - Auto-discover all test_hal_*.c files
    if(CONFIG_TEST_HAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/hal
            MODULE hal
            LIBRARY hal
        )
    endif()

    # PCL Unit Tests - Auto-discover all test_pcl_*.c files
    if(CONFIG_TEST_PCL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pcl
            MODULE pcl
            LIBRARY pconfig
        )
    endif()

    # PDL Unit Tests - Auto-discover all test_pdl_*.c files
    if(CONFIG_TEST_PDL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pdl
            MODULE pdl
            LIBRARY pdl
            INCLUDE_DIRS
                ${SDK_PATH}/core/pdl/src/pdl_bmc
                ${SDK_PATH}/core/pdl/src/pdl_mcu
                ${SDK_PATH}/core/pdl/src/pdl_satellite
        )
    endif()

    # PRL Unit Tests - Auto-discover all test_prl_*.c files
    if(CONFIG_TEST_PRL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/prl
            MODULE prl
            LIBRARY prl
        )
    endif()

    # ACONFIG Unit Tests - Auto-discover all test_aconfig_*.c files
    if(CONFIG_TEST_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/aconfig
            MODULE aconfig
            LIBRARY aconfig
        )
    endif()

    # PCONFIG Unit Tests - Auto-discover all test_pconfig_*.c files
    if(CONFIG_TEST_PCONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pconfig
            MODULE pconfig
            LIBRARY pconfig
        )
    endif()

    # ACL Unit Tests - Auto-discover all test_acl_*.c files
    if(CONFIG_TEST_ACL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/acl
            MODULE acl
            LIBRARY aconfig
        )
    endif()

endif() # CONFIG_TEST_UNIT

# ============================================================================
# Performance Tests - Auto-discovery based on Config.in
# ============================================================================

if(CONFIG_TEST_PERFORMANCE)
    message(STATUS "Enabling performance tests")

    # Core performance test file (always included)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/performance/test_perf_core.c")
        target_sources(es-middleware-test PRIVATE
            performance/test_perf_core.c
        )
    endif()

    # OSAL Performance Tests - Auto-discover all test_perf_osal_*.c files
    if(CONFIG_TEST_PERFORMANCE_OSAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/osal
            MODULE perf_osal
            LIBRARY osal
        )
    endif()

    # HAL Performance Tests - Auto-discover all test_perf_hal_*.c files
    if(CONFIG_TEST_PERFORMANCE_HAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/hal
            MODULE perf_hal
            LIBRARY hal
        )
    endif()

    # PDL Performance Tests - Auto-discover all test_perf_pdl_*.c files
    if(CONFIG_TEST_PERFORMANCE_PDL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/pdl
            MODULE perf_pdl
            LIBRARY pdl
        )
    endif()

    # PCL Performance Tests - Auto-discover all test_perf_pcl_*.c files
    if(CONFIG_TEST_PERFORMANCE_PCL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/pcl
            MODULE perf_pcl
            LIBRARY pconfig
        )
    endif()

    # ACL Performance Tests - Auto-discover all test_perf_acl_*.c files
    if(CONFIG_TEST_PERFORMANCE_ACL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/acl
            MODULE perf_acl
            LIBRARY aconfig
        )
    endif()

endif() # CONFIG_TEST_PERFORMANCE

# ============================================================================
# Stress Tests - Auto-discovery based on Config.in
# ============================================================================

if(CONFIG_TEST_STRESS)
    message(STATUS "Enabling stress tests")

    # Core stress test file (always included)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stress/test_stress_core.c")
        target_sources(es-middleware-test PRIVATE
            stress/test_stress_core.c
        )
    endif()

    # OSAL Stress Tests - Auto-discover all test_stress_osal_*.c files
    if(CONFIG_TEST_STRESS_OSAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/osal
            MODULE stress_osal
            LIBRARY osal
        )
    endif()

    # HAL Stress Tests - Auto-discover all test_stress_hal_*.c files
    if(CONFIG_TEST_STRESS_HAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/hal
            MODULE stress_hal
            LIBRARY hal
        )
    endif()

    # PDL Stress Tests - Auto-discover all test_stress_pdl_*.c files
    if(CONFIG_TEST_STRESS_PDL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/pdl
            MODULE stress_pdl
            LIBRARY pdl
        )
    endif()

    # PCL Stress Tests - Auto-discover all test_stress_pcl_*.c files
    if(CONFIG_TEST_STRESS_PCL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/pcl
            MODULE stress_pcl
            LIBRARY pconfig
        )
    endif()

    # ACL Stress Tests - Auto-discover all test_stress_acl_*.c files
    if(CONFIG_TEST_STRESS_ACL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/acl
            MODULE stress_acl
            LIBRARY aconfig
        )
    endif()

endif() # CONFIG_TEST_STRESS

# ============================================================================
# System Tests - Auto-discovery based on Config.in
# ============================================================================

if(CONFIG_TEST_SYSTEM)
    message(STATUS "Enabling system tests")

    # Core system test file (always included)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/system/test_system_core.c")
        target_sources(es-middleware-test PRIVATE
            system/test_system_core.c
        )
    endif()

    # System tests need core libraries - link them in correct order (dependencies first)
    if(TARGET osal)
        target_link_libraries(es-middleware-test PRIVATE osal)
    endif()
    if(TARGET hal)
        target_link_libraries(es-middleware-test PRIVATE hal)
    endif()
    if(TARGET pdl)
        target_link_libraries(es-middleware-test PRIVATE pdl)
    endif()
    if(TARGET prl)
        target_link_libraries(es-middleware-test PRIVATE prl)
    endif()

    # OSAL System Tests - Auto-discover all test_system_osal_*.c files
    if(CONFIG_TEST_SYSTEM_OSAL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/osal
            MODULE system_osal
            # Library already linked at system test level
        )
    endif()

    # HAL System Tests - Auto-discover all test_system_hal_*.c files
    if(CONFIG_TEST_SYSTEM_HAL)
        # Manually add HAL system test files, excluding test_hal_concurrent_access.c
        # which has its own main() and is built separately
        file(GLOB HAL_SYSTEM_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/system/hal/test_*.c")
        list(REMOVE_ITEM HAL_SYSTEM_TEST_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/system/hal/test_hal_concurrent_access.c")
        if(HAL_SYSTEM_TEST_FILES)
            target_sources(es-middleware-test PRIVATE ${HAL_SYSTEM_TEST_FILES})
        endif()

        # Create separate executable for concurrent access test (has its own main)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/system/hal/test_hal_concurrent_access.c")
            add_executable(test-hal-concurrent
                system/hal/test_hal_concurrent_access.c
            )
            target_link_libraries(test-hal-concurrent PRIVATE pthread rt m)
            if(TARGET osal)
                target_link_libraries(test-hal-concurrent PRIVATE osal)
            endif()
            if(TARGET hal)
                target_link_libraries(test-hal-concurrent PRIVATE hal)
            endif()
            # Link osal again at the end to resolve symbols
            if(TARGET osal)
                target_link_libraries(test-hal-concurrent PRIVATE osal)
            endif()
            set_target_properties(test-hal-concurrent PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            )
        endif()
    endif()

    # PDL System Tests - Auto-discover all test_system_pdl_*.c files
    if(CONFIG_TEST_SYSTEM_PDL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/pdl
            MODULE system_pdl
            # Library already linked at system test level
        )
    endif()

    # PCL System Tests - Auto-discover all test_system_pcl_*.c files
    if(CONFIG_TEST_SYSTEM_PCL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/pcl
            MODULE system_pcl
            LIBRARY pconfig
        )
    endif()

    # ACL System Tests - Auto-discover all test_system_acl_*.c files
    if(CONFIG_TEST_SYSTEM_ACL)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/acl
            MODULE system_acl
            LIBRARY aconfig
        )
    endif()

    # ACONFIG System Tests - Auto-discover all test_system_aconfig_*.c files
    if(CONFIG_TEST_SYSTEM_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/aconfig
            MODULE system_aconfig
            LIBRARY aconfig
        )
    endif()

    # PCONFIG System Tests - Auto-discover all test_system_pconfig_*.c files
    if(CONFIG_TEST_SYSTEM_PCONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/pconfig
            MODULE system_pconfig
            LIBRARY pconfig
        )
    endif()

    # Integration Tests - Auto-discover all test_system_integration_*.c files
    if(CONFIG_TEST_INTEGRATION)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/integration
            MODULE system_integration
            # Libraries already linked at system test level
        )
    endif()

endif() # CONFIG_TEST_SYSTEM

# ============================================================================
# Additional test dependencies
# ============================================================================

# Link pthread and rt for threading tests
target_link_libraries(es-middleware-test PRIVATE
    pthread
    rt
    m
)

# Force re-link osal at the end to resolve symbols
# This ensures OSAL symbols are available for all tests
if(TARGET osal)
    target_link_libraries(es-middleware-test PRIVATE osal)
endif()

# Set output directory
set_target_properties(es-middleware-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS es-middleware-test
    RUNTIME DESTINATION bin
)

message(STATUS "Test suite configuration complete")
