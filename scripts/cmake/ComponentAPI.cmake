# ==============================================================================
# ComponentAPI.cmake - High-level component registration helpers
# ==============================================================================
# This module provides simplified APIs for common component patterns,
# reducing boilerplate in component CMakeLists.txt files.
#
# Functions:
#   - register_app_component()      : Simplified app registration
#   - register_conditional_submodule() : Conditional submodule pattern
#   - register_component_with_shared_config() : Shared/static decision helper
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# register_app_component - Simplified application component registration
# ------------------------------------------------------------------------------
# Automatically handles common app patterns:
#   - GLOB_RECURSE for src/*.c files
#   - Standard dependency setup
#   - Component registration
#
# Usage:
#   register_app_component(
#       [PRODUCT <product_name>]
#       [REQUIREMENTS <dep1> <dep2> ...]
#       [SOURCES <src1> <src2> ...]          # Optional: explicit sources
#       [INCLUDE <dir1> <dir2> ...]          # Optional: additional includes
#       [SHARED]                              # Optional: build as shared library
#   )
#
# Examples:
#   # Standard PMC app
#   register_app_component(
#       PRODUCT pmc
#       REQUIREMENTS osal hal pconfig pdl aconfig libpmc
#   )
#
#   # App with explicit sources
#   register_app_component(
#       REQUIREMENTS osal hal
#       SOURCES custom1.c custom2.c
#   )
function(register_app_component)
    cmake_parse_arguments(
        APP
        "SHARED"                                    # Options
        "PRODUCT"                                   # Single value
        "REQUIREMENTS;SOURCES;INCLUDE"              # Multi-value
        ${ARGN}
    )

    # Auto-discover sources if not explicitly provided
    if(NOT APP_SOURCES)
        file(GLOB_RECURSE APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
    endif()

    # Set up component variables for register_component()
    set(ADD_SRCS ${APP_SOURCES})
    set(ADD_REQUIREMENTS ${APP_REQUIREMENTS})

    if(APP_INCLUDE)
        set(ADD_INCLUDE ${APP_INCLUDE})
    endif()

    # Register the component
    if(APP_SHARED)
        register_component(SHARED)
    else()
        register_component()
    endif()
endfunction()

# ------------------------------------------------------------------------------
# register_conditional_submodule - Conditional submodule pattern
# ------------------------------------------------------------------------------
# Simplifies the common pattern of enabling submodules based on Kconfig.
#
# Usage:
#   register_conditional_submodule(
#       CONFIG <config_variable>
#       SUBDIR <subdirectory>
#       [DEFINITION <definition_name>]
#       [PATTERN <file_pattern>]              # Default: *.c
#   )
#
# Example:
#   # PDL satellite submodule
#   register_conditional_submodule(
#       CONFIG CONFIG_PDL_SATELLITE
#       SUBDIR satellite
#       DEFINITION CONFIG_PDL_SATELLITE
#   )
#
#   # Equivalent to:
#   # if(CONFIG_PDL_SATELLITE)
#   #     file(GLOB sources satellite/*.c)
#   #     list(APPEND COMPONENT_SRCS ${sources})
#   #     list(APPEND ADD_DEFINITIONS -DCONFIG_PDL_SATELLITE=1)
#   # endif()
function(register_conditional_submodule)
    cmake_parse_arguments(
        MOD
        ""                                          # Options
        "CONFIG;SUBDIR;DEFINITION;PATTERN"          # Single value
        ""                                          # Multi-value
        ${ARGN}
    )

    # Validate required arguments
    if(NOT MOD_CONFIG)
        message(FATAL_ERROR "register_conditional_submodule: CONFIG is required")
    endif()
    if(NOT MOD_SUBDIR)
        message(FATAL_ERROR "register_conditional_submodule: SUBDIR is required")
    endif()

    # Default pattern
    if(NOT MOD_PATTERN)
        set(MOD_PATTERN "*.c")
    endif()

    # Only process if config is enabled
    if(${MOD_CONFIG})
        # Collect sources
        file(GLOB submodule_sources "${CMAKE_CURRENT_SOURCE_DIR}/${MOD_SUBDIR}/${MOD_PATTERN}")
        if(submodule_sources)
            list(APPEND COMPONENT_SRCS ${submodule_sources})
            set(COMPONENT_SRCS ${COMPONENT_SRCS} PARENT_SCOPE)
        endif()

        # Add compile definition if specified
        if(MOD_DEFINITION)
            list(APPEND ADD_DEFINITIONS "-D${MOD_DEFINITION}=1")
            set(ADD_DEFINITIONS ${ADD_DEFINITIONS} PARENT_SCOPE)
        endif()
    endif()
endfunction()

# ------------------------------------------------------------------------------
# register_component_with_shared_config - Shared/static decision helper
# ------------------------------------------------------------------------------
# Eliminates the if/else boilerplate for shared library configuration.
#
# Usage:
#   register_component_with_shared_config(
#       CONFIG <config_variable>
#       [... other register_component arguments ...]
#   )
#
# Example:
#   # Old way (4 lines)
#   # if(CONFIG_OSAL_BUILD_SHARED)
#   #     register_component(SHARED)
#   # else()
#   #     register_component()
#   # endif()
#
#   # New way (1 line)
#   register_component_with_shared_config(CONFIG CONFIG_OSAL_BUILD_SHARED)
function(register_component_with_shared_config)
    cmake_parse_arguments(
        COMP
        ""              # Options
        "CONFIG"        # Single value
        ""              # Multi-value
        ${ARGN}
    )

    if(NOT COMP_CONFIG)
        message(FATAL_ERROR "register_component_with_shared_config: CONFIG is required")
    endif()

    # Forward remaining arguments to register_component
    if(${COMP_CONFIG})
        register_component(SHARED ${COMP_UNPARSED_ARGUMENTS})
    else()
        register_component(${COMP_UNPARSED_ARGUMENTS})
    endif()
endfunction()
