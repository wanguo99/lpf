#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
# @license Apache 2.0
#


file(TO_CMAKE_PATH "${SDK_PATH}" SDK_PATH)

get_filename_component(parent_dir ${CMAKE_PARENT_LIST_FILE} DIRECTORY)
get_filename_component(current_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(parent_dir_name ${parent_dir} NAME)

function(register_component)
    get_filename_component(component_dir ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    get_filename_component(component_name ${component_dir} NAME)

    # Get params: DYNAMIC/SHARED
    foreach(name ${ARGN})
        string(TOUPPER ${name} name)
        if(${name} STREQUAL "DYNAMIC" OR ${name} STREQUAL "SHARED")
            set(to_dynamic_lib true)
        endif()
    endforeach()
    # Add src to lib
    if(ADD_SRCS)
        if(to_dynamic_lib)
            add_library(${component_name} SHARED ${ADD_SRCS})
        else()
            add_library(${component_name} STATIC ${ADD_SRCS})
        endif()
        set(include_type PUBLIC)
    else()
        if(to_dynamic_lib)
            add_library(${component_name} SHARED)
            set(include_type PUBLIC)
        else()
            add_library(${component_name} INTERFACE)
            set(include_type INTERFACE)
        endif()
    endif()

    # Add include
    foreach(include_dir ${ADD_INCLUDE})
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${component_dir})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${component_name} ${include_type} ${abs_dir})
    endforeach()

    # Add private include
    foreach(include_dir ${ADD_PRIVATE_INCLUDE})
        if(${include_type} STREQUAL INTERFACE)
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ADD_PRIVATE_INCLUDE set but no source file！")
        endif()
        get_filename_component(abs_dir ${include_dir} ABSOLUTE BASE_DIR ${component_dir})
        if(NOT IS_DIRECTORY ${abs_dir})
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_FILE}: ${include_dir} not found!")
        endif()
        target_include_directories(${component_name} PRIVATE ${abs_dir})
    endforeach()

    # Add definitions public
    foreach(difinition ${ADD_DEFINITIONS})
        if(${include_type} STREQUAL INTERFACE)
            target_compile_options(${component_name} INTERFACE ${difinition})
            target_link_options(${component_name} INTERFACE ${difinition})
        else()
            target_compile_options(${component_name} PUBLIC ${difinition})
            target_link_options(${component_name} PUBLIC ${difinition})
        endif()
    endforeach()

    # Add definitions private
    foreach(difinition ${ADD_DEFINITIONS_PRIVATE})
        target_compile_options(${component_name} PRIVATE ${difinition})
        target_link_options(${component_name} PRIVATE ${difinition})
    endforeach()

    # Add requirements
    target_link_libraries(${component_name} ${include_type} ${ADD_REQUIREMENTS})

    # Add file depends
    if(ADD_FILE_DEPENDS)
        add_custom_target(${component_name}_file_depends DEPENDS ${ADD_FILE_DEPENDS})
        add_dependencies(${component_name} ${component_name}_file_depends)
    endif()

    # ========================================================================
    # Install rules for Buildroot integration
    # ========================================================================
    # Install libraries to staging directory for linking
    if(ADD_SRCS OR to_dynamic_lib)
        install(TARGETS ${component_name}
            ARCHIVE DESTINATION ${INSTALL_LIBDIR}
            LIBRARY DESTINATION ${INSTALL_LIBDIR}
            RUNTIME DESTINATION ${INSTALL_BINDIR}
            COMPONENT libraries
        )
    endif()

    # Install public headers as a separate component. Compile include paths can
    # contain parent/source directories, so only install explicit public trees.
    set(public_header_dirs "")
    if(IS_DIRECTORY "${component_dir}/include")
        list(APPEND public_header_dirs "${component_dir}/include")
    endif()
    list(APPEND public_header_dirs ${ADD_PUBLIC_HEADER_DIRS})
    list(REMOVE_DUPLICATES public_header_dirs)

    foreach(header_dir ${public_header_dirs})
        get_filename_component(abs_dir ${header_dir} ABSOLUTE BASE_DIR ${component_dir})
        if(IS_DIRECTORY ${abs_dir})
            install(DIRECTORY ${abs_dir}/
                DESTINATION ${INSTALL_INCLUDEDIR}
                COMPONENT headers
                EXCLUDE_FROM_ALL
                FILES_MATCHING PATTERN "*.h"
            )
        endif()
    endforeach()
endfunction()
