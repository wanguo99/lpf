# ============================================================================
# Component Utility Functions
# ============================================================================
# Common CMake functions for component registration to reduce code duplication
# ============================================================================

# Register a core component with standard include paths and build options
# Usage: register_core_component(SHARED)
function(register_core_component)
    # Parse arguments
    set(to_dynamic_lib FALSE)
    foreach(arg ${ARGN})
        string(TOUPPER ${arg} arg_upper)
        if(${arg_upper} STREQUAL "DYNAMIC" OR ${arg_upper} STREQUAL "SHARED")
            set(to_dynamic_lib TRUE)
        endif()
    endforeach()

    # Get component name from directory
    get_filename_component(component_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    get_filename_component(component_name ${component_dir} NAME)

    # Standard include paths for core components
    if(NOT ADD_INCLUDE)
        list(APPEND ADD_INCLUDE "include")
    endif()

    # Add parent directory for #include "component/xxx.h" style
    get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
    list(APPEND ADD_INCLUDE "${PARENT_DIR}")

    # Set variables in parent scope
    set(ADD_INCLUDE ${ADD_INCLUDE} PARENT_SCOPE)

    # Call the original register_component
    if(to_dynamic_lib)
        register_component(SHARED)
    else()
        register_component()
    endif()
endfunction()

# Load platform configuration for products
# Usage: load_platform_config(CONFIG_VAR_NAME BASE_DIR)
# Example: load_platform_config(CONFIG_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR}/configs/platforms)
function(load_platform_config config_var base_dir)
    if(${config_var})
        set(PLATFORM_CONFIG_DIR ${base_dir}/${${config_var}})

        if(EXISTS ${PLATFORM_CONFIG_DIR})
            message("-- Loading platform config: ${${config_var}}")

            # Collect ACL config files
            file(GLOB _acl_srcs "${PLATFORM_CONFIG_DIR}/acl/*.c" "${PLATFORM_CONFIG_DIR}/acl/*/*.c")
            if(_acl_srcs)
                set(PLATFORM_ACL_SRCS ${_acl_srcs} PARENT_SCOPE)
                set(PLATFORM_CONFIG_INCLUDE ${PLATFORM_CONFIG_DIR}/acl PARENT_SCOPE)
            endif()

            # Collect PCL config files
            file(GLOB _pcl_srcs "${PLATFORM_CONFIG_DIR}/pcl/*.c")
            if(_pcl_srcs)
                set(PLATFORM_PCL_SRCS ${_pcl_srcs} PARENT_SCOPE)
            endif()
        else()
            message(WARNING "Platform config directory not found: ${PLATFORM_CONFIG_DIR}")
        endif()
    endif()
endfunction()

# Register an application component with standard dependencies
# Usage: register_app_component(dep1 dep2 ...)
# Example: register_app_component(osal hal pcl pdl acl libccm)
function(register_app_component)
    # Collect all source files
    if(NOT ADD_SRCS)
        file(GLOB_RECURSE ADD_SRCS "src/*.c")
    endif()

    # Add dependencies
    if(ARGN)
        list(APPEND ADD_REQUIREMENTS ${ARGN})
    endif()

    # Set variables in parent scope
    set(ADD_SRCS ${ADD_SRCS} PARENT_SCOPE)
    set(ADD_REQUIREMENTS ${ADD_REQUIREMENTS} PARENT_SCOPE)

    # Register component
    register_component()
endfunction()
