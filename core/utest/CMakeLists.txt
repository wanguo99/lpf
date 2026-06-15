# ============================================================================
# UTEST (Unit Test Framework) CMakeLists.txt
# ============================================================================
# Lightweight test framework for ES-Middleware unit/integration testing

if(NOT CONFIG_UTEST)
    return()
endif()

# ============================================================================
# Library Definition
# ============================================================================

add_library(utest STATIC)

# Source files
target_sources(utest PRIVATE
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
target_include_directories(utest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Private headers (for building utest itself)
target_include_directories(utest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utest
)

# ============================================================================
# Dependencies
# ============================================================================

# UTEST requires OSAL for basic OS abstractions
if(CONFIG_OSAL)
    target_link_libraries(utest PUBLIC osal)
endif()

# Link standard libraries
target_link_libraries(utest PUBLIC
    pthread
    rt
    m
)

# ============================================================================
# Compiler Options
# ============================================================================

# Enable warnings for test framework code
target_compile_options(utest PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# ============================================================================
# Installation (Optional - for standalone installation)
# ============================================================================

# Install library and headers (optional, mainly for development)
if(FALSE)  # Disabled by default - utest is primarily for internal testing
install(TARGETS utest
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

message(STATUS "UTEST: Test framework enabled")
if(CONFIG_UTEST_VERBOSE)
    message(STATUS "  Verbose output: enabled")
endif()
if(CONFIG_UTEST_COLOR)
    message(STATUS "  Colorized output: enabled")
endif()
message(STATUS "  Max tests: ${CONFIG_UTEST_MAX_TESTS}")
message(STATUS "  Max name length: ${CONFIG_UTEST_MAX_NAME_LEN}")
