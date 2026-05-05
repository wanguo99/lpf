# Unified Test Framework
# All test code compiled directly into executable

cmake_minimum_required(VERSION 3.10)

if(NOT BUILD_TESTING)
    return()
endif()

message(STATUS "Tests configured (unified test framework)")

# ============================================================================
# Collect Test Sources
# ============================================================================
file(GLOB_RECURSE UNIT_TEST_SOURCES
    "unit/osal/*.c"
    "unit/hal/*.c"
    "unit/pcl/*.c"
    "unit/pdl/*.c"
)

file(GLOB_RECURSE SYSTEM_TEST_SOURCES "system/**/*.c")
file(GLOB_RECURSE STRESS_TEST_SOURCES "stress/**/*.c")

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
    ${UNIT_TEST_SOURCES}
    ${SYSTEM_TEST_SOURCES}
    ${STRESS_TEST_SOURCES}
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
