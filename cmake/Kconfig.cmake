# Kconfig.cmake
# CMake integration for Kconfig configuration system using native Buildroot tools
#
# This module provides:
# 1. kconfig_config() - Load and process Kconfig configuration
# 2. Custom targets: menuconfig, nconfig, defconfig, savedefconfig
# 3. Automatic generation of autoconf.h from .config
# 4. Export CONFIG_* symbols to CMake cache
# 5. Dependency tracking for configuration changes
#
# Usage in root CMakeLists.txt:
#   include(cmake/KconfigTools.cmake)  # Build native tools first
#   include(cmake/Kconfig.cmake)
#   kconfig_config(
#       KCONFIG_FILE "Kconfig"
#       CONFIG_FILE "${CMAKE_SOURCE_DIR}/.config"
#       AUTOCONF_H "${CMAKE_BINARY_DIR}/config/autoconf.h"
#       USE_NATIVE_TOOLS TRUE
#   )

cmake_minimum_required(VERSION 3.16)

# Check if we should use native tools (requires KconfigTools.cmake)
if(NOT DEFINED KCONFIG_TOOLS_BUILT)
    set(USE_NATIVE_KCONFIG_TOOLS FALSE)
else()
    set(USE_NATIVE_KCONFIG_TOOLS ${KCONFIG_TOOLS_BUILT})
endif()

# Fallback to Python genconfig.py if native tools not available
if(NOT USE_NATIVE_KCONFIG_TOOLS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(KCONFIG_GENCONFIG_PY "${CMAKE_CURRENT_LIST_DIR}/../tools/kconfig/genconfig.py")
    if(NOT EXISTS "${KCONFIG_GENCONFIG_PY}")
        message(FATAL_ERROR "Neither native kconfig tools nor genconfig.py found")
    endif()
    message(STATUS "Using Python-based genconfig.py (native tools not available)")
endif()

# Function: kconfig_config
# Main entry point for Kconfig integration
#
# Parameters:
#   KCONFIG_FILE      - Path to root Kconfig file (default: Kconfig)
#   CONFIG_FILE       - Path to .config file (default: .config in source dir)
#   AUTOCONF_H        - Path to autoconf.h output (default: ${CMAKE_BINARY_DIR}/config/autoconf.h)
#   CMAKE_CONFIG      - Path to CMake config output (default: ${CMAKE_BINARY_DIR}/config/kconfig.cmake)
#   DEFCONFIG         - Optional defconfig to load if CONFIG_FILE doesn't exist
#   USE_NATIVE_TOOLS  - Use native kconfig tools instead of Python genconfig.py (default: auto-detect)
#
function(kconfig_config)
    set(options "")
    set(oneValueArgs KCONFIG_FILE CONFIG_FILE AUTOCONF_H CMAKE_CONFIG DEFCONFIG USE_NATIVE_TOOLS)
    set(multiValueArgs "")
    cmake_parse_arguments(KC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set defaults
    if(NOT KC_KCONFIG_FILE)
        set(KC_KCONFIG_FILE "${CMAKE_SOURCE_DIR}/Kconfig")
    endif()

    if(NOT KC_CONFIG_FILE)
        set(KC_CONFIG_FILE "${CMAKE_SOURCE_DIR}/.config")
    endif()

    if(NOT KC_AUTOCONF_H)
        set(KC_AUTOCONF_H "${CMAKE_BINARY_DIR}/config/autoconf.h")
    endif()

    if(NOT KC_CMAKE_CONFIG)
        set(KC_CMAKE_CONFIG "${CMAKE_BINARY_DIR}/config/kconfig.cmake")
    endif()

    # Determine which backend to use
    if(DEFINED KC_USE_NATIVE_TOOLS AND KC_USE_NATIVE_TOOLS)
        if(NOT USE_NATIVE_KCONFIG_TOOLS)
            message(FATAL_ERROR "Native Kconfig tools requested but not available. Include KconfigTools.cmake first.")
        endif()
        set(USE_NATIVE TRUE)
    elseif(USE_NATIVE_KCONFIG_TOOLS)
        set(USE_NATIVE TRUE)
    else()
        set(USE_NATIVE FALSE)
    endif()

    # Validate Kconfig file exists
    if(NOT EXISTS "${KC_KCONFIG_FILE}")
        message(FATAL_ERROR "Kconfig file not found: ${KC_KCONFIG_FILE}")
    endif()

    # Store paths in cache for custom targets
    set(KCONFIG_FILE "${KC_KCONFIG_FILE}" CACHE INTERNAL "Path to Kconfig file")
    set(CONFIG_FILE "${KC_CONFIG_FILE}" CACHE INTERNAL "Path to .config file")
    set(AUTOCONF_H "${KC_AUTOCONF_H}" CACHE INTERNAL "Path to autoconf.h")
    set(CMAKE_CONFIG "${KC_CMAKE_CONFIG}" CACHE INTERNAL "Path to CMake config")
    set(KCONFIG_USE_NATIVE "${USE_NATIVE}" CACHE INTERNAL "Use native kconfig tools")

    # Create output directories
    get_filename_component(config_dir "${KC_CONFIG_FILE}" DIRECTORY)
    get_filename_component(autoconf_dir "${KC_AUTOCONF_H}" DIRECTORY)
    file(MAKE_DIRECTORY "${config_dir}")
    file(MAKE_DIRECTORY "${autoconf_dir}")

    # Load configuration if it exists
    if(EXISTS "${KC_CONFIG_FILE}")
        message(STATUS "Loading Kconfig from: ${KC_CONFIG_FILE}")

        # Normalize configuration to generate derived values (e.g., CONFIG_PROJECT_NAME from choices)
        if(USE_NATIVE)
            _kconfig_normalize_config("${KC_KCONFIG_FILE}" "${KC_CONFIG_FILE}")
        endif()

        # Generate outputs (autoconf.h and CMake config)
        if(USE_NATIVE)
            _kconfig_generate_headers_native("${KC_KCONFIG_FILE}" "${KC_CONFIG_FILE}"
                                            "${KC_AUTOCONF_H}" "${KC_CMAKE_CONFIG}")
        else()
            _kconfig_generate_headers_python("${KC_KCONFIG_FILE}" "${KC_CONFIG_FILE}"
                                            "${KC_AUTOCONF_H}" "${KC_CMAKE_CONFIG}")
        endif()

        # Load configuration into CMake variables
        _kconfig_load_config("${KC_CONFIG_FILE}")

    elseif(KC_DEFCONFIG AND EXISTS "${KC_DEFCONFIG}")
        message(STATUS "Loading defconfig: ${KC_DEFCONFIG}")

        if(USE_NATIVE)
            _kconfig_load_defconfig_native("${KC_KCONFIG_FILE}" "${KC_DEFCONFIG}"
                                          "${KC_CONFIG_FILE}" "${KC_AUTOCONF_H}"
                                          "${KC_CMAKE_CONFIG}")
        else()
            _kconfig_load_defconfig_python("${KC_KCONFIG_FILE}" "${KC_DEFCONFIG}"
                                          "${KC_CONFIG_FILE}" "${KC_AUTOCONF_H}"
                                          "${KC_CMAKE_CONFIG}")
        endif()

        _kconfig_load_config("${KC_CONFIG_FILE}")

    else()
        message(WARNING "No Kconfig configuration found.")
        message(STATUS "Available defconfigs in configs/:")
        file(GLOB defconfigs "${CMAKE_SOURCE_DIR}/configs/*_defconfig")
        foreach(defconfig ${defconfigs})
            get_filename_component(defconfig_name ${defconfig} NAME)
            message(STATUS "  - ${defconfig_name}")
        endforeach()
        message(STATUS "")
        message(STATUS "To configure:")
        message(STATUS "  1. Run: python3 build.py config <name>")
        message(STATUS "  2. Or run: make menuconfig")
        message(STATUS "  3. Then re-run cmake")
    endif()

    # Load CMake config if it exists (contains all CONFIG_* variables)
    if(EXISTS "${KC_CMAKE_CONFIG}")
        include("${KC_CMAKE_CONFIG}")
        message(STATUS "Loaded CMake Kconfig: ${KC_CMAKE_CONFIG}")
    endif()

    # Create custom targets
    if(USE_NATIVE)
        _kconfig_create_targets_native("${KC_KCONFIG_FILE}" "${KC_CONFIG_FILE}"
                                       "${KC_AUTOCONF_H}" "${KC_CMAKE_CONFIG}")
    else()
        _kconfig_create_targets_python("${KC_KCONFIG_FILE}" "${KC_CONFIG_FILE}"
                                       "${KC_AUTOCONF_H}" "${KC_CMAKE_CONFIG}")
    endif()

    # Print configuration summary
    message(STATUS "Kconfig configuration:")
    message(STATUS "  Backend: ${USE_NATIVE}")
    message(STATUS "  Kconfig: ${KC_KCONFIG_FILE}")
    message(STATUS "  Config: ${KC_CONFIG_FILE}")
    message(STATUS "  autoconf.h: ${KC_AUTOCONF_H}")
    message(STATUS "  kconfig.cmake: ${KC_CMAKE_CONFIG}")

endfunction()

# ============================================================================
# Native Kconfig Tools Backend (Buildroot conf/mconf/nconf)
# ============================================================================

# Internal: Normalize configuration using olddefconfig
# This generates derived values (e.g., CONFIG_PROJECT_NAME from choice selections)
function(_kconfig_normalize_config kconfig_file config_file)
    if(NOT EXISTS "${config_file}")
        return()
    endif()

    # Check if conf executable is available
    if(NOT KCONFIG_CONF_EXECUTABLE OR NOT EXISTS "${KCONFIG_CONF_EXECUTABLE}")
        message(WARNING "Kconfig conf tool not available yet, skipping normalization")
        return()
    endif()

    # Run conf --olddefconfig to normalize the configuration
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            "KCONFIG_CONFIG=${config_file}"
            ${KCONFIG_CONF_EXECUTABLE}
            --olddefconfig
            "${kconfig_file}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_VARIABLE error
        TIMEOUT 30
    )

    if(NOT result EQUAL 0)
        message(WARNING "Failed to normalize config: ${error}")
    elseif(result EQUAL 124)
        message(WARNING "Config normalization timed out (30s)")
    endif()
endfunction()

# Internal: Generate autoconf.h manually from .config (optimized)
function(_kconfig_generate_autoconf_h config_file autoconf_h)
    if(NOT EXISTS "${config_file}")
        return()
    endif()

    # Check if autoconf.h already exists and is up-to-date
    if(EXISTS "${autoconf_h}")
        file(TIMESTAMP "${config_file}" CONFIG_TIME)
        file(TIMESTAMP "${autoconf_h}" HEADER_TIME)
        if(NOT CONFIG_TIME IS_NEWER_THAN HEADER_TIME)
            # Header is up-to-date, skip generation
            return()
        endif()
    endif()

    file(STRINGS "${config_file}" config_lines)

    # Generate header file
    set(header_content "/* Auto-generated from .config - DO NOT EDIT */\n")
    string(APPEND header_content "#ifndef AUTOCONF_H\n")
    string(APPEND header_content "#define AUTOCONF_H\n\n")

    foreach(line ${config_lines})
        # Skip comments and empty lines
        if(line MATCHES "^[ \t]*#" OR line STREQUAL "")
            continue()
        endif()

        # Parse CONFIG_XXX=value
        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=(.*)$")
            set(name ${CMAKE_MATCH_1})
            set(value ${CMAKE_MATCH_2})

            # Handle different value types
            if(value STREQUAL "y")
                # Boolean option enabled
                string(APPEND header_content "#define CONFIG_${name} 1\n")
            elseif(value MATCHES "^\"(.*)\"$")
                # String value
                string(APPEND header_content "#define CONFIG_${name} ${value}\n")
            elseif(value MATCHES "^[0-9]+$")
                # Numeric value
                string(APPEND header_content "#define CONFIG_${name} ${value}\n")
            else()
                # Other values (hex, etc.)
                string(APPEND header_content "#define CONFIG_${name} ${value}\n")
            endif()
        endif()
    endforeach()

    string(APPEND header_content "\n#endif /* AUTOCONF_H */\n")
    file(WRITE "${autoconf_h}" "${header_content}")
endfunction()

# Internal: Generate autoconf.h and kconfig.cmake using native conf tool
function(_kconfig_generate_headers_native kconfig_file config_file autoconf_h cmake_config)
    if(NOT EXISTS "${config_file}")
        return()
    endif()

    # Create output directories
    get_filename_component(autoconf_dir "${autoconf_h}" DIRECTORY)
    file(MAKE_DIRECTORY "${autoconf_dir}")

    # Step 1: Generate autoconf.h manually from .config
    # Note: --syncconfig expects Buildroot directory structure (include/generated/),
    # which causes segfaults with our custom paths. Generate manually instead.
    _kconfig_generate_autoconf_h("${config_file}" "${autoconf_h}")

    # Step 2: Generate kconfig.cmake from .config
    _kconfig_generate_cmake_config("${config_file}" "${cmake_config}")

    message(STATUS "Generated autoconf.h and kconfig.cmake")
endfunction()

# Internal: Load defconfig using native conf tool
function(_kconfig_load_defconfig_native kconfig_file defconfig config_file autoconf_h cmake_config)
    # Use conf --defconfig to load defconfig
    execute_process(
        COMMAND ${KCONFIG_CONF_EXECUTABLE}
            --defconfig="${defconfig}"
            "${kconfig_file}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_VARIABLE error
        ENVIRONMENT
            "KCONFIG_CONFIG=${config_file}"
    )

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to load defconfig ${defconfig}: ${error}")
    endif()

    message(STATUS "Loaded defconfig: ${defconfig}")

    # Generate headers
    _kconfig_generate_headers_native("${kconfig_file}" "${config_file}" "${autoconf_h}" "${cmake_config}")
endfunction()

# Internal: Generate kconfig.cmake from .config (optimized)
function(_kconfig_generate_cmake_config config_file cmake_config)
    if(NOT EXISTS "${config_file}")
        return()
    endif()

    # Check if kconfig.cmake already exists and is up-to-date
    if(EXISTS "${cmake_config}")
        file(TIMESTAMP "${config_file}" CONFIG_TIME)
        file(TIMESTAMP "${cmake_config}" CMAKE_CONFIG_TIME)
        if(NOT CONFIG_TIME IS_NEWER_THAN CMAKE_CONFIG_TIME)
            # CMake config is up-to-date, skip generation
            return()
        endif()
    endif()

    file(STRINGS "${config_file}" config_lines)

    set(cmake_content "# Auto-generated from .config - DO NOT EDIT\n")
    string(APPEND cmake_content "# Generated at: ${CMAKE_CURRENT_LIST_FILE}\n\n")

    foreach(line ${config_lines})
        # Skip comments and empty lines
        if(line MATCHES "^[ \t]*#" OR line STREQUAL "")
            continue()
        endif()

        # Parse CONFIG_XXX=value
        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=(.*)$")
            set(name ${CMAKE_MATCH_1})
            set(value ${CMAKE_MATCH_2})

            # Remove quotes from string values
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" value "${value}")

            # Convert y/n to ON/OFF for CMake
            if(value STREQUAL "y")
                set(value "ON")
            elseif(value STREQUAL "n")
                set(value "OFF")
            endif()

            string(APPEND cmake_content "set(CONFIG_${name} \"${value}\" CACHE INTERNAL \"Kconfig: ${name}\")\n")

        # Parse # CONFIG_XXX is not set
        elseif(line MATCHES "^# CONFIG_([A-Za-z0-9_]+) is not set$")
            set(name ${CMAKE_MATCH_1})
            string(APPEND cmake_content "set(CONFIG_${name} OFF CACHE INTERNAL \"Kconfig: ${name}\")\n")
        endif()
    endforeach()

    file(WRITE "${cmake_config}" "${cmake_content}")
endfunction()

# Internal: Create custom targets for native tools
function(_kconfig_create_targets_native kconfig_file config_file autoconf_h cmake_config)
    # Target: menuconfig (using mconf if available)
    if(KCONFIG_MCONF_EXECUTABLE AND EXISTS "${KCONFIG_MCONF_EXECUTABLE}")
        add_custom_target(menuconfig
            COMMAND ${CMAKE_COMMAND} -E env
                "KCONFIG_CONFIG=${config_file}"
                ${KCONFIG_MCONF_EXECUTABLE} "${kconfig_file}"
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                "Configuration saved to ${config_file}"
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow
                "Re-run cmake to apply changes"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Opening menuconfig..."
            VERBATIM
            USES_TERMINAL
        )
        message(STATUS "Created target: menuconfig (using mconf)")
    else()
        message(STATUS "menuconfig target not available (mconf not built)")
    endif()

    # Target: nconfig (using nconf if available)
    if(KCONFIG_NCONF_EXECUTABLE AND EXISTS "${KCONFIG_NCONF_EXECUTABLE}")
        add_custom_target(nconfig
            COMMAND ${CMAKE_COMMAND} -E env
                "KCONFIG_CONFIG=${config_file}"
                ${KCONFIG_NCONF_EXECUTABLE} "${kconfig_file}"
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                "Configuration saved to ${config_file}"
            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow
                "Re-run cmake to apply changes"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Opening nconfig..."
            VERBATIM
            USES_TERMINAL
        )
        message(STATUS "Created target: nconfig (using nconf)")
    endif()

    # Target: defconfig <defconfig_file>
    file(GLOB _defconfigs "${CMAKE_SOURCE_DIR}/configs/*_defconfig")

    add_custom_target(defconfig
        COMMAND ${CMAKE_COMMAND} -E echo "Usage: python3 build.py config <name>"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Available defconfigs:"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "List available defconfigs"
        VERBATIM
    )

    # Add echo commands for each defconfig
    foreach(_df ${_defconfigs})
        get_filename_component(_dfname ${_df} NAME)
        add_custom_command(TARGET defconfig POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ${_dfname}"
            VERBATIM
        )
    endforeach()

    # Target: savedefconfig
    add_custom_target(savedefconfig
        COMMAND ${KCONFIG_CONF_EXECUTABLE}
            --savedefconfig=defconfig
            "${kconfig_file}"
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
            "Minimal config saved to defconfig"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Saving minimal defconfig..."
        VERBATIM
        USES_TERMINAL
    )
    message(STATUS "Created target: savedefconfig")

    # Target: oldconfig (update config with new options)
    add_custom_target(oldconfig
        COMMAND ${CMAKE_COMMAND} -E env
            "KCONFIG_CONFIG=${config_file}"
            "KCONFIG_AUTOHEADER=${autoconf_h}"
            ${KCONFIG_CONF_EXECUTABLE}
                --oldconfig "${kconfig_file}"
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
            "Configuration updated"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Updating configuration..."
        VERBATIM
        USES_TERMINAL
    )
    message(STATUS "Created target: oldconfig")

    # Target: list_defconfigs
    file(GLOB _defconfigs "${CMAKE_SOURCE_DIR}/configs/*_defconfig")

    add_custom_target(list_defconfigs
        COMMAND ${CMAKE_COMMAND} -E echo "Available defconfigs:"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Listing available defconfigs"
        VERBATIM
        USES_TERMINAL
    )

    # Add echo commands for each defconfig
    foreach(_df ${_defconfigs})
        get_filename_component(_dfname ${_df} NAME)
        add_custom_command(TARGET list_defconfigs POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "  ${_dfname}"
            VERBATIM
        )
    endforeach()

    message(STATUS "Created target: list_defconfigs")
endfunction()

# ============================================================================
# Python genconfig.py Backend (Fallback)
# ============================================================================

# Internal: Generate headers using Python genconfig.py
function(_kconfig_generate_headers_python kconfig_file config_file autoconf_h cmake_config)
    set(cmd_args
        ${Python3_EXECUTABLE}
        ${KCONFIG_GENCONFIG_PY}
        --kconfig ${kconfig_file}
        --config ${config_file}
        --output header ${autoconf_h}
        --output cmake ${cmake_config}
    )

    set(ENV{srctree} "${CMAKE_SOURCE_DIR}")
    set(ENV{KCONFIG_CONFIG} "${config_file}")

    execute_process(
        COMMAND ${cmd_args}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_VARIABLE error
    )

    if(NOT result EQUAL 0)
        message(WARNING "Failed to generate headers: ${error}")
    endif()
endfunction()

# Internal: Load defconfig using Python genconfig.py
function(_kconfig_load_defconfig_python kconfig_file defconfig config_file autoconf_h cmake_config)
    set(cmd_args
        ${Python3_EXECUTABLE}
        ${KCONFIG_GENCONFIG_PY}
        --kconfig ${kconfig_file}
        --defaults ${defconfig}
        --output makefile ${config_file}
        --output header ${autoconf_h}
        --output cmake ${cmake_config}
    )

    set(ENV{srctree} "${CMAKE_SOURCE_DIR}")
    set(ENV{KCONFIG_CONFIG} "${config_file}")

    execute_process(
        COMMAND ${cmd_args}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_VARIABLE error
    )

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to load defconfig: ${error}")
    endif()

    message(STATUS "Loaded defconfig: ${defconfig}")
endfunction()

# Internal: Create custom targets for Python backend
function(_kconfig_create_targets_python kconfig_file config_file autoconf_h cmake_config)
    add_custom_target(menuconfig
        COMMAND ${Python3_EXECUTABLE} ${KCONFIG_GENCONFIG_PY}
            --kconfig ${kconfig_file}
            --menuconfig True
            --output makefile ${config_file}
            --output header ${autoconf_h}
            --output cmake ${cmake_config}
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
            "Configuration saved to ${config_file}"
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow
            "Re-run cmake to apply changes"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Opening menuconfig..."
        VERBATIM
        USES_TERMINAL
    )

    add_custom_target(list_defconfigs
        COMMAND ${CMAKE_COMMAND} -E echo "Available defconfigs:"
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ls -1 configs/*_defconfig
        COMMENT "Listing available defconfigs"
        VERBATIM
    )

    message(STATUS "Created targets: menuconfig, list_defconfigs (Python backend)")
endfunction()

# ============================================================================
# Common Functions
# ============================================================================

# Internal: Load and parse .config file into CMake variables
function(_kconfig_load_config config_file)
    if(NOT EXISTS "${config_file}")
        return()
    endif()

    file(STRINGS "${config_file}" config_lines)
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

            # Export to parent scope and cache
            set(CONFIG_${config_name} ${config_value} PARENT_SCOPE)
            set(CONFIG_${config_name} ${config_value} CACHE INTERNAL "Kconfig option: ${config_name}")
            math(EXPR config_count "${config_count} + 1")

        # Parse # CONFIG_XXX is not set
        elseif(line MATCHES "^# CONFIG_([A-Za-z0-9_]+) is not set$")
            set(config_name ${CMAKE_MATCH_1})
            set(CONFIG_${config_name} OFF PARENT_SCOPE)
            set(CONFIG_${config_name} OFF CACHE INTERNAL "Kconfig option: ${config_name}")
            math(EXPR config_count "${config_count} + 1")
        endif()
    endforeach()

    message(STATUS "Loaded ${config_count} Kconfig symbols from .config")
endfunction()

# ============================================================================
# Helper Functions
# ============================================================================

# Helper: Print all CONFIG_* variables
function(kconfig_print_config)
    message(STATUS "Current Kconfig configuration:")
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)
    foreach(_variableName ${_variableNames})
        if(_variableName MATCHES "^CONFIG_")
            message(STATUS "  ${_variableName}=${${_variableName}}")
        endif()
    endforeach()
endfunction()

# Helper: Check if a config option is enabled
function(kconfig_check_enabled config_name result_var)
    if(DEFINED CONFIG_${config_name})
        if(CONFIG_${config_name})
            set(${result_var} TRUE PARENT_SCOPE)
        else()
            set(${result_var} FALSE PARENT_SCOPE)
        endif()
    else()
        set(${result_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Print backend information
if(USE_NATIVE_KCONFIG_TOOLS)
    message(STATUS "Kconfig.cmake loaded (Native Buildroot tools backend)")
else()
    message(STATUS "Kconfig.cmake loaded (Python genconfig.py backend)")
endif()
