cmake_minimum_required(VERSION 3.16)

# ============================================================================
# ES-Middleware SDK Root CMakeLists.txt
# ============================================================================
# This is the unified build entry point for the entire ES-Middleware SDK.
# It builds all enabled products based on Kconfig configuration.
# ============================================================================

# Set SDK path (this is the root directory)
set(SDK_PATH ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "SDK_PATH: ${SDK_PATH}")

# Load Kconfig integration (builds kconfig tools and processes configuration)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/KconfigTools.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Kconfig.cmake)

# Configure Kconfig
kconfig_config(
    KCONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/Kconfig"
    CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/.config"
    AUTOCONF_H "${CMAKE_CURRENT_BINARY_DIR}/config/autoconf.h"
    CMAKE_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/config/kconfig.cmake"
    USE_NATIVE_TOOLS TRUE
)

# Standard CMake project declaration
project(es_middleware_sdk C CXX ASM)

# Set global output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include build system
include(${SDK_PATH}/tools/cmake/compile.cmake)

# Generate build info headers (version and time) at configure time
get_python(python python_version python_info_str)
if(NOT python)
    message(FATAL_ERROR "python not found, please install python firstly(python3 recommend)!")
endif()

# Auto-include autoconf.h for all compilation units
set(KCONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/config/autoconf.h")
if(EXISTS ${KCONFIG_HEADER})
    add_compile_options(-include ${KCONFIG_HEADER})
    message(STATUS "Auto-including kconfig header: ${KCONFIG_HEADER}")
else()
    message(WARNING "Configuration header not found: ${KCONFIG_HEADER}")
    message(STATUS "Configuration will be generated during build")
endif()

# Execute at configure time to generate build info headers
execute_process(
    COMMAND ${python} ${SDK_PATH}/tools/kconfig/update_build_info.py
            --configfile header
            ${CMAKE_CURRENT_BINARY_DIR}/config/global_build_info_time.h
            ${CMAKE_CURRENT_BINARY_DIR}/config/global_build_info_version.h
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate build info headers")
endif()

# Add build info directory to include path
include_directories(${CMAKE_CURRENT_BINARY_DIR}/config)

# ============================================================================
# Build Products Based on Configuration
# ============================================================================
# Each product is built only if CONFIG_PRODUCT_XXX is enabled in .config
# ============================================================================

if(CONFIG_PRODUCT_CCM)
    message("-- Building CCM product")
    add_subdirectory(products/ccm)
endif()

if(CONFIG_PRODUCT_SAMPLE)
    message("-- Building Sample product")
    add_subdirectory(products/sample)
endif()

# ============================================================================
# Build Tests as a Special Product
# ============================================================================
if(CONFIG_BUILD_TESTING)
    message("-- Building test suite (as test product)")
    add_subdirectory(products/tests)
endif()
