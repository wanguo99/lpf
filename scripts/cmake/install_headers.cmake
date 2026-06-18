# ES-Middleware Headers Installation Script
# This script installs development headers for all enabled modules
# Usage: cmake -P install_headers.cmake

# Get DESTDIR from environment or use empty string
if(DEFINED ENV{DESTDIR})
    set(DESTDIR "$ENV{DESTDIR}")
else()
    set(DESTDIR "")
endif()

# Get CMAKE_INSTALL_PREFIX from environment or use default
# Buildroot sets this via environment or toolchainfile.cmake
if(DEFINED ENV{CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "$ENV{CMAKE_INSTALL_PREFIX}")
elseif(DEFINED ENV{BR2_EXTERNAL} OR DEFINED ENV{BR_BUILDING})
    # Buildroot environment detected
    set(CMAKE_INSTALL_PREFIX "/usr")
else()
    # Standalone build
    set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()

# Full installation path
set(INSTALL_PATH "${DESTDIR}${CMAKE_INSTALL_PREFIX}/include")

message(STATUS "Installing ES-Middleware headers")
message(STATUS "  Destination: ${INSTALL_PATH}")

# Load configuration to determine which modules are enabled
set(SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")
set(CONFIG_FILE "${SDK_PATH}/.config")

if(EXISTS ${CONFIG_FILE})
    file(STRINGS ${CONFIG_FILE} config_lines)
    foreach(line ${config_lines})
        # Parse CONFIG_XXX=y
        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=(.+)$")
            set(config_name ${CMAKE_MATCH_1})
            set(config_value ${CMAKE_MATCH_2})

            # Strip quotes
            string(REGEX REPLACE "^\"(.*)\"$" "\\1" config_value "${config_value}")

            # Convert to boolean
            if(config_value STREQUAL "y")
                set(config_value ON)
            elseif(config_value STREQUAL "n")
                set(config_value OFF)
            endif()

            set(CONFIG_${config_name} ${config_value})
        endif()
    endforeach()
endif()

# Function to install headers for a module
function(install_module_headers module_name module_path)
    if(CONFIG_${module_name})
        set(header_src "${SDK_PATH}/${module_path}/include")
        if(EXISTS ${header_src})
            message(STATUS "  Installing ${module_name} headers: ${header_src}")
            file(COPY ${header_src}/
                 DESTINATION ${INSTALL_PATH}
                 FILES_MATCHING PATTERN "*.h")
        endif()
    endif()
endfunction()

function(install_header_tree module_name header_path)
    if(CONFIG_${module_name})
        set(header_src "${SDK_PATH}/${header_path}")
        if(EXISTS ${header_src})
            message(STATUS "  Installing ${module_name} headers: ${header_src}")
            file(COPY ${header_src}/
                 DESTINATION ${INSTALL_PATH}
                 FILES_MATCHING PATTERN "*.h")
        endif()
    endif()
endfunction()

# Install core module headers based on Kconfig
install_module_headers(OSAL "core/user/osal")
install_module_headers(ACONFIG "core/user/aconfig")
install_module_headers(PDI "core/user/pdi")
install_header_tree(PDI "core/uapi")
install_module_headers(PRL "core/prl")

message(STATUS "Headers installation complete")
