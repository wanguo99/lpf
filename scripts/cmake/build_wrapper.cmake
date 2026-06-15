# ==============================================================================
# Build System Wrapper for ES-Middleware
# ==============================================================================
# This module provides a clean interface layer between Makefile and CMake,
# inspired by ESP-IDF and Zephyr RTOS architecture.
#
# Design principles:
#   - Clear separation of concerns
#   - Environment validation before build
#   - Consistent behavior across platforms
#   - Integration-ready (Buildroot, Yocto, etc.)
#
# Usage:
#   include(scripts/cmake/build_wrapper.cmake)
#   build_wrapper_init()
#
# Public functions:
#   build_wrapper_init()       - Initialize build environment
#   build_wrapper_validate()   - Validate build environment
#   build_wrapper_print_info() - Print build information
# ==============================================================================

# Guard against multiple inclusion
if(DEFINED __BUILD_WRAPPER_INCLUDED)
    return()
endif()
set(__BUILD_WRAPPER_INCLUDED TRUE)

# ==============================================================================
# Build environment detection
# ==============================================================================

function(_detect_build_environment)
    # Detect if building inside Buildroot
    if(DEFINED ENV{BR2_EXTERNAL} OR DEFINED ENV{BR_BUILDING})
        set(BUILD_IN_BUILDROOT TRUE PARENT_SCOPE)
        set(BUILD_QUIET_MODE TRUE PARENT_SCOPE)
    else()
        set(BUILD_IN_BUILDROOT FALSE PARENT_SCOPE)
        set(BUILD_QUIET_MODE FALSE PARENT_SCOPE)
    endif()

    # Detect if building inside Yocto/OpenEmbedded
    if(DEFINED ENV{OECORE_NATIVE_SYSROOT})
        set(BUILD_IN_YOCTO TRUE PARENT_SCOPE)
        set(BUILD_QUIET_MODE TRUE PARENT_SCOPE)
    else()
        set(BUILD_IN_YOCTO FALSE PARENT_SCOPE)
    endif()

    # Detect build type from environment or config
    if(DEFINED ENV{CMAKE_BUILD_TYPE})
        set(BUILD_TYPE_FROM_ENV $ENV{CMAKE_BUILD_TYPE} PARENT_SCOPE)
    else()
        set(BUILD_TYPE_FROM_ENV "" PARENT_SCOPE)
    endif()
endfunction()

# ==============================================================================
# Version information management
# ==============================================================================

function(_generate_version_info)
    # SDK version (should match config)
    set(SDK_VERSION "1.0.0")
    set(SDK_VERSION_MAJOR 1)
    set(SDK_VERSION_MINOR 0)
    set(SDK_VERSION_PATCH 0)

    # Get Git information - handle Buildroot override case
    set(GIT_WORK_DIR "${CMAKE_SOURCE_DIR}")

    # Check if source is overridden (local development)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        set(GIT_WORK_DIR "${CMAKE_SOURCE_DIR}")
    elseif(DEFINED ENV{ES_MIDDLEWARE_OVERRIDE_SRCDIR} AND EXISTS "$ENV{ES_MIDDLEWARE_OVERRIDE_SRCDIR}/.git")
        set(GIT_WORK_DIR "$ENV{ES_MIDDLEWARE_OVERRIDE_SRCDIR}")
    endif()

    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${GIT_WORK_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${GIT_WORK_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE GIT_RESULT
        )
        if(NOT GIT_RESULT EQUAL 0)
            set(GIT_COMMIT_HASH "unknown")
        endif()

        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
            WORKING_DIRECTORY ${GIT_WORK_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${GIT_WORK_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    else()
        set(GIT_COMMIT_HASH "unknown")
        set(GIT_DESCRIBE "unknown")
        set(GIT_BRANCH "unknown")
    endif()

    if(NOT GIT_COMMIT_HASH)
        set(GIT_COMMIT_HASH "unknown")
    endif()
    if(NOT GIT_DESCRIBE)
        set(GIT_DESCRIBE "unknown")
    endif()
    if(NOT GIT_BRANCH)
        set(GIT_BRANCH "unknown")
    endif()

    # Build timestamp
    string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S UTC" UTC)
    string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)
    string(TIMESTAMP BUILD_TIMESTAMP "%s" UTC)

    # Export to parent scope
    set(SDK_VERSION ${SDK_VERSION} PARENT_SCOPE)
    set(SDK_VERSION_MAJOR ${SDK_VERSION_MAJOR} PARENT_SCOPE)
    set(SDK_VERSION_MINOR ${SDK_VERSION_MINOR} PARENT_SCOPE)
    set(SDK_VERSION_PATCH ${SDK_VERSION_PATCH} PARENT_SCOPE)
    set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} PARENT_SCOPE)
    set(GIT_DESCRIBE ${GIT_DESCRIBE} PARENT_SCOPE)
    set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
    set(BUILD_TIME ${BUILD_TIME} PARENT_SCOPE)
    set(BUILD_DATE ${BUILD_DATE} PARENT_SCOPE)
    set(BUILD_TIMESTAMP ${BUILD_TIMESTAMP} PARENT_SCOPE)
endfunction()

function(_write_version_headers)
    # Generate build time header
    set(BUILD_TIME_HEADER "${CMAKE_BINARY_DIR}/config/global_build_info_time.h")
    file(WRITE "${BUILD_TIME_HEADER}"
"/* Auto-generated by build_wrapper.cmake */
#ifndef GLOBAL_BUILD_INFO_TIME_H
#define GLOBAL_BUILD_INFO_TIME_H

#define BUILD_TIME \"${BUILD_TIME}\"
#define BUILD_DATE \"${BUILD_DATE}\"
#define BUILD_TIMESTAMP ${BUILD_TIMESTAMP}

#endif /* GLOBAL_BUILD_INFO_TIME_H */
")

    # Generate build version header
    set(BUILD_VERSION_HEADER "${CMAKE_BINARY_DIR}/config/global_build_info_version.h")
    file(WRITE "${BUILD_VERSION_HEADER}"
"/* Auto-generated by build_wrapper.cmake */
#ifndef GLOBAL_BUILD_INFO_VERSION_H
#define GLOBAL_BUILD_INFO_VERSION_H

#define SDK_VERSION \"${SDK_VERSION}\"
#define SDK_VERSION_MAJOR ${SDK_VERSION_MAJOR}
#define SDK_VERSION_MINOR ${SDK_VERSION_MINOR}
#define SDK_VERSION_PATCH ${SDK_VERSION_PATCH}
#define BUILD_GIT_COMMIT_ID \"${GIT_COMMIT_HASH}\"
#define BUILD_GIT_DESCRIBE \"${GIT_DESCRIBE}\"
#define BUILD_GIT_BRANCH \"${GIT_BRANCH}\"

#endif /* GLOBAL_BUILD_INFO_VERSION_H */
")

    # Add include directory
    include_directories(${CMAKE_BINARY_DIR}/config)

    # Export paths to parent scope
    set(BUILD_TIME_HEADER ${BUILD_TIME_HEADER} PARENT_SCOPE)
    set(BUILD_VERSION_HEADER ${BUILD_VERSION_HEADER} PARENT_SCOPE)
endfunction()

# ==============================================================================
# Installation path configuration
# ==============================================================================

function(_configure_install_paths)
    # Use GNUInstallDirs for standard directory variables
    include(GNUInstallDirs)

    # Detect installation prefix based on environment
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if(BUILD_IN_BUILDROOT OR BUILD_IN_YOCTO)
            # Buildroot/Yocto typically install to /usr
            set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix" FORCE)
        else()
            # Standalone builds default to /usr/local
            set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix" FORCE)
        endif()
    endif()

    # Define installation directories
    set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR} PARENT_SCOPE)
    set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR} PARENT_SCOPE)
    set(INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/es-middleware PARENT_SCOPE)

    # Only print in verbose mode
    if(NOT BUILD_QUIET_MODE)
        message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
    endif()
endfunction()

# ==============================================================================
# Build type configuration
# ==============================================================================

function(_configure_build_type)
    # Priority: CONFIG_BUILD_TYPE > CMAKE_BUILD_TYPE > Default (Release)
    if(CONFIG_BUILD_TYPE_DEBUG)
        set(computed_build_type "Debug")
    elseif(CONFIG_BUILD_TYPE_RELEASE)
        set(computed_build_type "Release")
    elseif(CONFIG_BUILD_TYPE_RELWITHDEBINFO)
        set(computed_build_type "RelWithDebInfo")
    elseif(CONFIG_BUILD_TYPE_MINSIZEREL)
        set(computed_build_type "MinSizeRel")
    elseif(CMAKE_BUILD_TYPE)
        set(computed_build_type ${CMAKE_BUILD_TYPE})
    else()
        set(computed_build_type "Release")
    endif()

    set(CMAKE_BUILD_TYPE ${computed_build_type} CACHE STRING "Build type" FORCE)
    set(CMAKE_BUILD_TYPE ${computed_build_type} PARENT_SCOPE)

    # Only print in verbose mode
    if(NOT BUILD_QUIET_MODE)
        message(STATUS "Build type: ${computed_build_type}")
    endif()
endfunction()

# ==============================================================================
# Output directory configuration
# ==============================================================================

function(_configure_output_directories)
    # Set global output directories
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)

    # Only print in verbose mode
    if(NOT BUILD_QUIET_MODE)
        message(STATUS "Output: ${CMAKE_BINARY_DIR}")
    endif()
endfunction()

# ==============================================================================
# Compiler and toolchain validation
# ==============================================================================

function(_validate_toolchain)
    # Check compiler versions
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_C_COMPILER_VERSION VERSION_LESS "7.0")
            message(WARNING "GCC version ${CMAKE_C_COMPILER_VERSION} is old - recommend 7.0+")
        endif()
    endif()

    # Validate cross-compilation setup if applicable
    if(CMAKE_CROSSCOMPILING)
        # Only print in verbose mode
        if(NOT BUILD_QUIET_MODE)
            message(STATUS "Cross-compiling for ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        if(NOT CMAKE_FIND_ROOT_PATH)
            message(WARNING "CMAKE_FIND_ROOT_PATH not set - cross-compilation may fail")
        endif()
    endif()

    # Check for required tools
    find_program(MAKE_EXECUTABLE NAMES make gmake)
    if(NOT MAKE_EXECUTABLE)
        message(WARNING "Make not found in PATH")
    endif()
endfunction()

# ==============================================================================
# Public API
# ==============================================================================

# Initialize build wrapper
function(build_wrapper_init)
    if(BUILD_WRAPPER_INITIALIZED)
        return()
    endif()

    # Step 1: Detect build environment (must be first to set BUILD_QUIET_MODE)
    _detect_build_environment()

    # Only show header in verbose mode
    if(NOT BUILD_QUIET_MODE)
        message(STATUS "")
        message(STATUS "========================================================================")
        message(STATUS "ES-Middleware Build Wrapper")
        message(STATUS "========================================================================")
    endif()

    # Step 2: Generate version information
    _generate_version_info()
    _write_version_headers()

    # Step 3: Configure build type
    _configure_build_type()

    # Step 4: Configure installation paths
    _configure_install_paths()

    # Step 5: Configure output directories
    _configure_output_directories()

    # Step 6: Validate toolchain
    _validate_toolchain()

    # Export initialization state
    set(BUILD_WRAPPER_INITIALIZED TRUE PARENT_SCOPE)
    set(BUILD_IN_BUILDROOT ${BUILD_IN_BUILDROOT} PARENT_SCOPE)
    set(BUILD_IN_YOCTO ${BUILD_IN_YOCTO} PARENT_SCOPE)
    set(BUILD_QUIET_MODE ${BUILD_QUIET_MODE} PARENT_SCOPE)

    # Export version info to parent
    set(SDK_VERSION ${SDK_VERSION} PARENT_SCOPE)
    set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} PARENT_SCOPE)
    set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
    set(BUILD_TIME ${BUILD_TIME} PARENT_SCOPE)

    # Export installation dirs to parent
    set(INSTALL_BINDIR ${INSTALL_BINDIR} PARENT_SCOPE)
    set(INSTALL_LIBDIR ${INSTALL_LIBDIR} PARENT_SCOPE)
    set(INSTALL_INCLUDEDIR ${INSTALL_INCLUDEDIR} PARENT_SCOPE)

    # Export output dirs to parent
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)

    # Export build type to parent
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} PARENT_SCOPE)

    # Only show completion message in verbose mode
    if(NOT BUILD_QUIET_MODE)
        message(STATUS "Build Wrapper: Initialization complete")
        message(STATUS "========================================================================")
        message(STATUS "")
    endif()
endfunction()

# Validate build environment (called before actual build)
function(build_wrapper_validate)
    if(NOT BUILD_WRAPPER_INITIALIZED)
        message(FATAL_ERROR "Build wrapper not initialized - call build_wrapper_init() first")
    endif()

    # Check disk space (warning only)
    if(UNIX)
        execute_process(
            COMMAND df -k ${CMAKE_BINARY_DIR}
            OUTPUT_VARIABLE df_output
            ERROR_QUIET
        )
        if(df_output MATCHES "([0-9]+)%")
            set(disk_usage ${CMAKE_MATCH_1})
            if(disk_usage GREATER 90)
                message(WARNING "Disk usage is ${disk_usage}% - build may fail")
            endif()
        endif()
    endif()
endfunction()

# Print build information summary
function(build_wrapper_print_info)
    message(STATUS "")
    message(STATUS "========================================================================")
    message(STATUS "Build Information")
    message(STATUS "========================================================================")
    message(STATUS "SDK Version:        ${SDK_VERSION}")
    message(STATUS "Git Commit:         ${GIT_COMMIT_HASH}")
    message(STATUS "Git Branch:         ${GIT_BRANCH}")
    message(STATUS "Build Time:         ${BUILD_TIME}")
    message(STATUS "Build Type:         ${CMAKE_BUILD_TYPE}")
    message(STATUS "Install Prefix:     ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "")
    message(STATUS "Compiler:")
    message(STATUS "  C Compiler:       ${CMAKE_C_COMPILER}")
    message(STATUS "  C++ Compiler:     ${CMAKE_CXX_COMPILER}")
    if(CMAKE_CROSSCOMPILING)
        message(STATUS "  Target System:    ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    message(STATUS "")
    message(STATUS "Environment:")
    if(BUILD_IN_BUILDROOT)
        message(STATUS "  Buildroot:        YES")
    elseif(BUILD_IN_YOCTO)
        message(STATUS "  Yocto:            YES")
    else()
        message(STATUS "  Standalone:       YES")
    endif()
    message(STATUS "========================================================================")
    message(STATUS "")
endfunction()
