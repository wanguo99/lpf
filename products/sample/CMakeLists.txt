cmake_minimum_required(VERSION 3.16)

set(SDK_ENV_NAME "EMS_SDK_PATH")
set(CUSTOM_COMPONENTS_PATH_ENV_NAME "CUSTOM_COMPONENTS_PATH")

set(SDK_PATH_ENV $ENV{${SDK_ENV_NAME}})
set(CUSTOM_COMPONENTS_PATH_ENV $ENV{${CUSTOM_COMPONENTS_PATH_ENV_NAME}})

# Get SDK path
if(NOT SDK_PATH)
    # From products/sample/ to EMS root: ../../
    get_filename_component(SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
    if(SDK_PATH_ENV)
        if(EXISTS ${SDK_PATH_ENV})
            set(SDK_PATH ${SDK_PATH_ENV})
        else()
            message(FATAL_ERROR "Env variable '${SDK_ENV_NAME}' set, but '${SDK_PATH_ENV}', path not exists")
        endif()
    endif()
endif()

if(NOT CUSTOM_COMPONENTS_PATH)
    # Set product-level components path: products/sample/components
    get_filename_component(CUSTOM_COMPONENTS_PATH "${CMAKE_CURRENT_LIST_DIR}/components" ABSOLUTE)
    if(CUSTOM_COMPONENTS_PATH_ENV)
        if(EXISTS ${CUSTOM_COMPONENTS_PATH_ENV})
            set(CUSTOM_COMPONENTS_PATH ${CUSTOM_COMPONENTS_PATH_ENV})
        else()
            message(FATAL_ERROR "Env variable '${CUSTOM_COMPONENTS_PATH_ENV_NAME}' set, but '${CUSTOM_COMPONENTS_PATH_ENV}', path not exists")
        endif()
    endif()
endif()

# Check SDK Path
if(NOT EXISTS ${SDK_PATH})
    message(FATAL_ERROR "SDK path Error, Please set SDK_PATH or ${SDK_ENV_NAME} variable")
endif()

# Call compile
include(${SDK_PATH}/tools/cmake/compile.cmake)

# Project Name
set(PROJECT_NAME sample)

message("-- PROJECT_NAME:${PROJECT_NAME}")
project(${PROJECT_NAME})
