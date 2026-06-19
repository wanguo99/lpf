# ============================================================================
# Core Modules Test Suite CMakeLists.txt
# ============================================================================
# This is a special "product" dedicated to testing ES-Middleware core modules.
# Unlike regular products (e.g., PMC), this product's sole purpose is to
# provide comprehensive testing coverage for:
#   - OSAL, HAL, PDI, PCONFIG, ACONFIG
#
# Test categories:
#   - Unit: Component-level testing
#   - Performance: Benchmarking and profiling
#   - Stress: Stability and resource testing
#   - System: Integration testing
#
# Test Discovery System:
# - Test files are automatically discovered and matched to Kconfig options
# - No manual file list maintenance required
# - Convention: test_<module>_<name>.c --> CONFIG_TEST_<MODULE>_<NAME>
# ============================================================================

# Include test discovery functions
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestDiscovery.cmake)

# ============================================================================
# Product Static Configuration
# ============================================================================
# PCONFIG/ACONFIG are read-only query libraries. The ctest product provides
# concrete configuration tables at build time through the core modules' extra
# source hooks.

set(CTEST_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config)

if(CONFIG_PCONFIG)
    set(PCONFIG_EXTRA_SRCS ${CTEST_CONFIG_DIR}/ctest_pconfig.c)
endif()

if(CONFIG_ACONFIG)
    set(ACONFIG_EXTRA_SRCS ${CTEST_CONFIG_DIR}/ctest_aconfig.c)
endif()

# ============================================================================
# Register SDK Core Components (like other products do)
# ============================================================================
# Build core modules required by tests
# Note: Check if target already exists to avoid duplicate registration
# when tests are built alongside products

if(CONFIG_OSAL AND NOT TARGET osal)
    add_subdirectory(${SDK_PATH}/core/user/osal ${CMAKE_CURRENT_BINARY_DIR}/osal)
endif()

if(CONFIG_HAL AND NOT TARGET hal)
    add_subdirectory(${SDK_PATH}/core/kernel/hal ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

if(CONFIG_PCONFIG AND NOT TARGET pconfig)
    add_subdirectory(${SDK_PATH}/core/kernel/pconfig ${CMAKE_CURRENT_BINARY_DIR}/pconfig)
endif()

if(CONFIG_PDM AND NOT TARGET pdm)
    add_subdirectory(${SDK_PATH}/core/kernel/pdm ${CMAKE_CURRENT_BINARY_DIR}/pdm)
endif()

if(CONFIG_PDI AND NOT TARGET pdi)
    add_subdirectory(${SDK_PATH}/core/user/pdi ${CMAKE_CURRENT_BINARY_DIR}/pdi)
endif()

if(CONFIG_ACONFIG AND NOT TARGET aconfig)
    add_subdirectory(${SDK_PATH}/core/user/aconfig ${CMAKE_CURRENT_BINARY_DIR}/aconfig)
endif()

# ============================================================================
# Test Framework Library (UTEST)
# ============================================================================
# Core test framework - now a shared core module
# Register UTEST module from core (same pattern as other core modules)

if(CONFIG_TEST_FRAMEWORK AND NOT TARGET test_framework)
    add_subdirectory(${SDK_PATH}/core/user/test_framework ${CMAKE_CURRENT_BINARY_DIR}/test_framework)
endif()

# ============================================================================
# Test Executable (es-middleware-test)
# ============================================================================
# Main entry point + conditionally compiled test cases

add_executable(es-middleware-test
    core/main.c
)

# Link test framework
if(CONFIG_TEST_FRAMEWORK)
    target_link_libraries(es-middleware-test PRIVATE test_framework)
endif()

# Link OSAL (required by test framework)
if(CONFIG_OSAL)
    target_link_libraries(es-middleware-test PRIVATE osal)
endif()

# Add include directories for test cases
target_include_directories(es-middleware-test PRIVATE
    ${SDK_PATH}/core/user/osal/include/osal
    ${SDK_PATH}/core/kernel/include
    ${SDK_PATH}/core/kernel/include/hal
    ${SDK_PATH}/core/kernel/include/pconfig
    ${SDK_PATH}/core/user/pdi/include
    ${SDK_PATH}/core/uapi
    ${SDK_PATH}/core/user/aconfig/include/aconfig
)

# Add test framework headers if enabled
if(CONFIG_TEST_FRAMEWORK)
    target_include_directories(es-middleware-test PRIVATE
        ${SDK_PATH}/core/user/test_framework/include/test_framework
    )
endif()

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

    # PCONFIG Unit Tests - Auto-discover all test_pconfig_*.c files
    if(CONFIG_TEST_PCONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pconfig
            MODULE pconfig
            LIBRARY pconfig
        )
    endif()

    # PDI Unit Tests - Auto-discover all test_pdi_*.c files
    if(CONFIG_TEST_PDI)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pdi
            MODULE pdi
            LIBRARY pdi
        )
    endif()

    # PDM Unit Tests - Auto-discover all test_pdm_*.c files
    if(CONFIG_TEST_PDM)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/pdm
            MODULE pdm
            LIBRARY pdm
            INCLUDE_DIRS
                ${SDK_PATH}/core/kernel/pdm/src/protocol
        )
    endif()

    # ACONFIG Unit Tests - 只测试核心 API 功能（不依赖 PMC）
    if(CONFIG_TEST_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/aconfig
            MODULE aconfig
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

    # PCONFIG Performance Tests - Auto-discover all test_perf_pconfig_*.c files
    if(CONFIG_TEST_PERFORMANCE_PCONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/pconfig
            MODULE perf_pconfig
            LIBRARY pconfig
        )
    endif()

    # ACONFIG Performance Tests - Auto-discover all test_perf_aconfig_*.c files
    if(CONFIG_TEST_PERFORMANCE_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/aconfig
            MODULE perf_aconfig
            LIBRARY aconfig
        )
    endif()

endif() # CONFIG_TEST_PERFORMANCE

# ============================================================================
# Stress Tests - Auto-discovery based on Config.in
# ============================================================================

if(CONFIG_TEST_STRESS)
    message(STATUS "Enabling stress tests")

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

    # PCONFIG Stress Tests - Auto-discover all test_stress_pconfig_*.c files
    if(CONFIG_TEST_STRESS_PCONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/pconfig
            MODULE stress_pconfig
            LIBRARY pconfig
        )
    endif()

    # ACONFIG Stress Tests - Auto-discover all test_stress_aconfig_*.c files
    if(CONFIG_TEST_STRESS_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stress/aconfig
            MODULE stress_aconfig
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
    if(TARGET aconfig)
        target_link_libraries(es-middleware-test PRIVATE aconfig)
    endif()
    if(TARGET pconfig)
        target_link_libraries(es-middleware-test PRIVATE pconfig)
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
        if(HAL_SYSTEM_TEST_FILES)
            target_sources(es-middleware-test PRIVATE ${HAL_SYSTEM_TEST_FILES})
            test_collect_registration_functions(${HAL_SYSTEM_TEST_FILES})
        endif()
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

    # ACONFIG System Tests - Auto-discover all test_system_aconfig_*.c files
    if(CONFIG_TEST_SYSTEM_ACONFIG)
        test_discover_and_add(
            TARGET es-middleware-test
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/system/aconfig
            MODULE system_aconfig
            LIBRARY aconfig
        )
    endif()

endif() # CONFIG_TEST_SYSTEM

# Generate explicit test suite registration source after all discovery is done.
test_generate_registry(
    TARGET es-middleware-test
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ctest_autoregistry.c
)

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
