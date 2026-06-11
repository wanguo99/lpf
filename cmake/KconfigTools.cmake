# KconfigTools.cmake
# Builds native Kconfig tools (conf, mconf, nconf) as part of the CMake project
#
# This module:
# 1. Builds kconfig binaries from scripts/kconfig/ at configure time
# 2. Exports paths to the built tools for use by Kconfig.cmake
# 3. Handles dependencies (ncurses, flex, bison)
# 4. Uses pre-generated parser files (no flex/bison required at runtime)
#
# Output variables (exported to cache):
#   KCONFIG_CONF_EXECUTABLE    - Path to conf binary (required)
#   KCONFIG_MCONF_EXECUTABLE   - Path to mconf binary (if ncurses available)
#   KCONFIG_NCONF_EXECUTABLE   - Path to nconf binary (if ncurses available)
#   KCONFIG_TOOLS_BUILT        - TRUE if tools were successfully built
#
# Requirements:
#   - C compiler
#   - ncurses development libraries (for mconf/nconf)
#   - Optional: flex/bison (for regenerating parsers, not required by default)
#
# Usage:
#   include(cmake/KconfigTools.cmake)
#   # Tools are now available at ${KCONFIG_CONF_EXECUTABLE}, etc.

cmake_minimum_required(VERSION 3.16)

# Prevent multiple inclusion
if(KCONFIG_TOOLS_BUILT)
    return()
endif()

set(KCONFIG_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../scripts/kconfig")
set(KCONFIG_BUILD_DIR "${CMAKE_BINARY_DIR}/kconfig-tools-build")

if(NOT EXISTS "${KCONFIG_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Kconfig source directory not found: ${KCONFIG_SOURCE_DIR}")
endif()

# Check if tools already exist and are up-to-date (cache optimization)
set(CONF_TOOL "${KCONFIG_BUILD_DIR}/conf")
set(TOOLS_STAMP "${KCONFIG_BUILD_DIR}/.tools_built_stamp")

if(EXISTS "${CONF_TOOL}" AND EXISTS "${TOOLS_STAMP}")
    # Check if source files are newer than stamp
    file(GLOB_RECURSE KCONFIG_SOURCES "${KCONFIG_SOURCE_DIR}/*.c" "${KCONFIG_SOURCE_DIR}/*.h")
    set(REBUILD_NEEDED FALSE)

    file(TIMESTAMP "${TOOLS_STAMP}" STAMP_TIME)
    foreach(src ${KCONFIG_SOURCES})
        file(TIMESTAMP "${src}" SRC_TIME)
        if(SRC_TIME IS_NEWER_THAN STAMP_TIME)
            set(REBUILD_NEEDED TRUE)
            break()
        endif()
    endforeach()

    if(NOT REBUILD_NEEDED)
        message(STATUS "Kconfig tools already built (using cache)")
        set(KCONFIG_CONF_EXECUTABLE "${CONF_TOOL}" CACHE FILEPATH "Path to conf binary")
        set(KCONFIG_MCONF_EXECUTABLE "${KCONFIG_BUILD_DIR}/mconf" CACHE FILEPATH "Path to mconf binary")
        set(KCONFIG_NCONF_EXECUTABLE "${KCONFIG_BUILD_DIR}/nconf" CACHE FILEPATH "Path to nconf binary")
        set(KCONFIG_TOOLS_BUILT TRUE CACHE INTERNAL "Kconfig tools build status")
        return()
    endif()
endif()

message(STATUS "Building Kconfig tools...")

# Check for ncurses (required for mconf/nconf)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(NCURSES QUIET ncurses)
    if(NCURSES_FOUND)
        message(STATUS "ncurses found - mconf/nconf will be available")
    else()
        message(STATUS "ncurses not found - only conf will be built")
    endif()
else()
    message(STATUS "pkg-config not found - only conf will be built")
endif()

# Configure kconfig tools at configure time
message(STATUS "Configuring kconfig tools in ${KCONFIG_BUILD_DIR}")

execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S "${KCONFIG_SOURCE_DIR}"
        -B "${KCONFIG_BUILD_DIR}"
        -DCMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE KCONFIG_CONFIG_RESULT
    OUTPUT_VARIABLE KCONFIG_CONFIG_OUTPUT
    ERROR_VARIABLE KCONFIG_CONFIG_ERROR
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT KCONFIG_CONFIG_RESULT EQUAL 0)
    message(WARNING "Failed to configure kconfig tools")
    message(STATUS "CMake output: ${KCONFIG_CONFIG_OUTPUT}")
    message(STATUS "CMake error: ${KCONFIG_CONFIG_ERROR}")
    set(KCONFIG_TOOLS_BUILT FALSE CACHE INTERNAL "Kconfig tools build status")
    return()
endif()

# Build conf (required tool)
message(STATUS "Building conf tool...")
execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${KCONFIG_BUILD_DIR}" --target conf
    RESULT_VARIABLE CONF_BUILD_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT CONF_BUILD_RESULT EQUAL 0)
    message(WARNING "Failed to build conf tool")
    set(KCONFIG_TOOLS_BUILT FALSE CACHE INTERNAL "Kconfig tools build status")
    return()
endif()

set(KCONFIG_CONF_EXECUTABLE "${KCONFIG_BUILD_DIR}/conf" CACHE FILEPATH "Path to conf binary")
message(STATUS "Built conf: ${KCONFIG_CONF_EXECUTABLE}")

# Try to build mconf if ncurses is available
if(NCURSES_FOUND)
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${KCONFIG_BUILD_DIR}" --target mconf
        RESULT_VARIABLE MCONF_BUILD_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(MCONF_BUILD_RESULT EQUAL 0)
        set(KCONFIG_MCONF_EXECUTABLE "${KCONFIG_BUILD_DIR}/mconf" CACHE FILEPATH "Path to mconf binary")
        message(STATUS "Built mconf: ${KCONFIG_MCONF_EXECUTABLE}")
    else()
        message(STATUS "mconf build failed (this is OK - menuconfig will not be available)")
    endif()

    # Try to build nconf
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${KCONFIG_BUILD_DIR}" --target nconf
        RESULT_VARIABLE NCONF_BUILD_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NCONF_BUILD_RESULT EQUAL 0)
        set(KCONFIG_NCONF_EXECUTABLE "${KCONFIG_BUILD_DIR}/nconf" CACHE FILEPATH "Path to nconf binary")
        message(STATUS "Built nconf: ${KCONFIG_NCONF_EXECUTABLE}")
    else()
        message(STATUS "nconf build failed (this is OK - nconfig will not be available)")
    endif()
else()
    message(STATUS "ncurses not found - skipping mconf/nconf build")
endif()

# Mark tools as successfully built
set(KCONFIG_TOOLS_BUILT TRUE CACHE INTERNAL "Kconfig tools build status")

# Create timestamp file for cache optimization
file(WRITE "${TOOLS_STAMP}" "Kconfig tools built at: ${CMAKE_CURRENT_LIST_FILE}\n")

# Summary
message(STATUS "Kconfig tools built successfully:")
message(STATUS "  conf:  ${KCONFIG_CONF_EXECUTABLE}")
if(KCONFIG_MCONF_EXECUTABLE AND EXISTS "${KCONFIG_MCONF_EXECUTABLE}")
    message(STATUS "  mconf: ${KCONFIG_MCONF_EXECUTABLE}")
endif()
if(KCONFIG_NCONF_EXECUTABLE AND EXISTS "${KCONFIG_NCONF_EXECUTABLE}")
    message(STATUS "  nconf: ${KCONFIG_NCONF_EXECUTABLE}")
endif()
