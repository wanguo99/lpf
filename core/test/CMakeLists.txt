# ============================================================================
# Test Framework CMakeLists.txt
# ============================================================================
# Comprehensive test framework for ES-Middleware testing
# Supports: unit, performance, stress, system, and integration tests

if(NOT CONFIG_TEST_FRAMEWORK)
    return()
endif()

# ============================================================================
# Library Definition
# ============================================================================

add_library(test STATIC)

# Source files
target_sources(test PRIVATE
    src/test_registry.c
    src/test_runner.c
    src/test_menu.c
    src/test_mock.c
    src/test_metadata.c
    src/test_reporter.c
    src/test_stress.c
)

# ============================================================================
# Include Directories
# ============================================================================

# Public headers (accessible to test users)
target_include_directories(test PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Private headers (for building utest itself)
target_include_directories(test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/test
)

# ============================================================================
# Dependencies
# ============================================================================

# TEST requires OSAL for basic OS abstractions
if(CONFIG_OSAL)
    target_link_libraries(test PUBLIC osal)
endif()

# Link standard libraries
target_link_libraries(test PUBLIC
    pthread
    rt
    m
)

# ============================================================================
# Compiler Options
# ============================================================================

# Enable warnings for test framework code
target_compile_options(test PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# ============================================================================
# Installation (Optional - for standalone installation)
# ============================================================================

# Install library and headers (optional, mainly for development)
if(FALSE)  # Disabled by default - utest is primarily for internal testing
install(TARGETS test
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "TEST: Test framework enabled")
if(CONFIG_TEST_FRAMEWORK_VERBOSE)
    message(STATUS "  Verbose output: enabled")
endif()
if(CONFIG_TEST_FRAMEWORK_COLOR)
    message(STATUS "  Colorized output: enabled")
endif()
message(STATUS "  Max tests: ${CONFIG_TEST_FRAMEWORK_MAX_TESTS}")
message(STATUS "  Max name length: ${CONFIG_TEST_FRAMEWORK_MAX_NAME_LEN}")
