# ============================================================================
# ES-Middleware SDK Root CMakeLists.txt
# ============================================================================
# This CMakeLists.txt handles ONLY compilation and installation.
# All Config.in configuration management is handled by the root Makefile.
#
# The Makefile:
#   - Builds Config.in tools (conf, mconf, nconf)
#   - Handles all config targets (menuconfig, defconfig, etc.)
#   - Generates autoconf.h from .config
#   - Invokes CMake for compilation only
#
# This CMakeLists.txt:
#   - Loads configuration from autoconf.h
#   - Compiles enabled products/modules
#   - Handles installation
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# ============================================================================
# Configuration Loading
# ============================================================================

# Expect CONFIG_FILE and AUTOCONF_H to be passed from Makefile
# If not provided, use defaults (for standalone CMake usage)
if(NOT DEFINED CONFIG_FILE)
    set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.config")
endif()

if(NOT DEFINED AUTOCONF_H)
    set(AUTOCONF_H "${CMAKE_CURRENT_BINARY_DIR}/include/generated/autoconf.h")
endif()

# Validate that configuration exists
if(NOT EXISTS "${CONFIG_FILE}")
    message(FATAL_ERROR
        "Configuration file not found: ${CONFIG_FILE}\n"
        "Please run 'make <board>_defconfig' first.\n"
        "Run 'make list' to see available configurations.")
endif()

message(STATUS "Configuration file: ${CONFIG_FILE}")
message(STATUS "autoconf.h: ${AUTOCONF_H}")

# Set SDK path
set(SDK_PATH ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "SDK_PATH: ${SDK_PATH}")

# ============================================================================
# Load Config.in Configuration into CMake Variables
# ============================================================================

# Parse .config and load CONFIG_* variables into CMake
if(EXISTS "${CONFIG_FILE}")
    file(STRINGS "${CONFIG_FILE}" config_lines)
    set(config_count 0)

    foreach(line ${config_lines})
        # Skip comments and empty lines
        if(line MATCHES "^[ \t]*#" OR line STREQUAL "")
            continue()
        endif()

        # Parse CONFIG_XXX=value
        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=(.*)$")
            set(config_name ${CMAKE_MATCH_1})
            set(config_value ${CMAKE_MATCH_2})

            # Remove quotes from string values
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" config_value "${config_value}")

            # Convert y/n to ON/OFF for CMake
            if(config_value STREQUAL "y")
                set(config_value ON)
            elseif(config_value STREQUAL "n")
                set(config_value OFF)
            endif()

            # Export to CMake cache
            set(CONFIG_${config_name} ${config_value} CACHE INTERNAL "Config.in: ${config_name}")
            math(EXPR config_count "${config_count} + 1")

        # Parse # CONFIG_XXX is not set
        elseif(line MATCHES "^# CONFIG_([A-Za-z0-9_]+) is not set$")
            set(config_name ${CMAKE_MATCH_1})
            set(CONFIG_${config_name} OFF CACHE INTERNAL "Config.in: ${config_name}")
            math(EXPR config_count "${config_count} + 1")
        endif()
    endforeach()

    message(STATUS "Loaded ${config_count} Config.in symbols from .config")
else()
    message(FATAL_ERROR "Configuration file not found: ${CONFIG_FILE}")
endif()

# ============================================================================
# Project Declaration
# ============================================================================

project(es_middleware_sdk C CXX ASM)

# ============================================================================
# Global Build Settings
# ============================================================================

# Set global output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Installation Configuration
# ============================================================================

# Installation prefix (overridable via -DCMAKE_INSTALL_PREFIX=...)
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")
endif()

# Use GNUInstallDirs for standard directory variables
include(GNUInstallDirs)

# Define installation directories
set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/es-middleware)

message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Binaries: ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
message(STATUS "  Libraries: ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
message(STATUS "  Headers: ${CMAKE_INSTALL_PREFIX}/${INSTALL_INCLUDEDIR}")

# ============================================================================
# Include Build System
# ============================================================================

include(${SDK_PATH}/scripts/cmake/compile.cmake)
include(${SDK_PATH}/scripts/cmake/ComponentAPI.cmake)

# ============================================================================
# Auto-include Configuration Header
# ============================================================================

# Check if autoconf.h exists
if(EXISTS ${AUTOCONF_H})
    add_compile_options(-include ${AUTOCONF_H})
    message(STATUS "Auto-including configuration header: ${AUTOCONF_H}")
else()
    message(WARNING "Configuration header not found: ${AUTOCONF_H}")
    message(WARNING "This should have been generated by Makefile")
    message(WARNING "Run 'make syncconfig' or re-run 'make <board>_defconfig'")
endif()

# Add include directory for generated headers
get_filename_component(AUTOCONF_DIR ${AUTOCONF_H} DIRECTORY)
include_directories(${AUTOCONF_DIR}/..)

# ============================================================================
# Generate Build Info Headers
# ============================================================================

set(BUILD_TIME_HEADER "${CMAKE_CURRENT_BINARY_DIR}/config/global_build_info_time.h")
set(BUILD_VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/config/global_build_info_version.h")

# Generate build time header
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S" UTC)
string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)
file(WRITE "${BUILD_TIME_HEADER}"
"/* Auto-generated by CMake at configure time */\n"
"#ifndef GLOBAL_BUILD_INFO_TIME_H\n"
"#define GLOBAL_BUILD_INFO_TIME_H\n"
"\n"
"#define BUILD_TIME \"${BUILD_TIME}\"\n"
"#define BUILD_DATE \"${BUILD_DATE}\"\n"
"\n"
"#endif /* GLOBAL_BUILD_INFO_TIME_H */\n"
)

# Generate build version header
set(SDK_VERSION "1.0.0")
set(SDK_VERSION_MAJOR 1)
set(SDK_VERSION_MINOR 0)
set(SDK_VERSION_PATCH 0)

# Get Git commit hash if available
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

file(WRITE "${BUILD_VERSION_HEADER}"
"/* Auto-generated by CMake at configure time */\n"
"#ifndef GLOBAL_BUILD_INFO_VERSION_H\n"
"#define GLOBAL_BUILD_INFO_VERSION_H\n"
"\n"
"#define SDK_VERSION \"${SDK_VERSION}\"\n"
"#define SDK_VERSION_MAJOR ${SDK_VERSION_MAJOR}\n"
"#define SDK_VERSION_MINOR ${SDK_VERSION_MINOR}\n"
"#define SDK_VERSION_PATCH ${SDK_VERSION_PATCH}\n"
"#define BUILD_GIT_COMMIT_ID \"${GIT_COMMIT_HASH}\"\n"
"\n"
"#endif /* GLOBAL_BUILD_INFO_VERSION_H */\n"
)

message(STATUS "Generated build info headers")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/config)

# ============================================================================
# Build Products Based on Configuration
# ============================================================================

if(CONFIG_PRODUCT_CCM)
    message(STATUS "Building CCM product (CONFIG_PRODUCT_CCM=ON)")
    add_subdirectory(products/ccm)
else()
    message(STATUS "Skipping CCM product (CONFIG_PRODUCT_CCM=OFF)")
endif()

if(CONFIG_PRODUCT_SAMPLE)
    message(STATUS "Building Sample product (CONFIG_PRODUCT_SAMPLE=ON)")
    add_subdirectory(products/sample)
else()
    message(STATUS "Skipping Sample product (CONFIG_PRODUCT_SAMPLE not set)")
endif()

# ============================================================================
# Build Tests
# ============================================================================

if(CONFIG_BUILD_TESTING)
    message(STATUS "Building test suite (CONFIG_BUILD_TESTING=ON)")
    add_subdirectory(products/tests)
else()
    message(STATUS "Skipping tests (CONFIG_BUILD_TESTING=OFF)")
endif()

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "ES-Middleware Configuration Summary")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Products:")
if(CONFIG_PRODUCT_CCM)
    message(STATUS "  CCM: YES")
else()
    message(STATUS "  CCM: NO")
endif()
if(CONFIG_PRODUCT_SAMPLE)
    message(STATUS "  Sample: YES")
else()
    message(STATUS "  Sample: NO")
endif()
if(CONFIG_BUILD_TESTING)
    message(STATUS "  Tests: YES")
else()
    message(STATUS "  Tests: NO")
endif()
message(STATUS "========================================")
message(STATUS "")
