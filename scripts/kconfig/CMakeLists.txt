# CMakeLists.txt for Kconfig tools
# Extracted from Buildroot support/kconfig/
cmake_minimum_required(VERSION 3.16)
project(kconfig C)

# Check for required dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncurses)

# Find optional programs for regenerating parsers
find_program(FLEX flex)
find_program(BISON bison)

# Determine whether to use shipped parser files or generate new ones
set(USE_SHIPPED_PARSERS TRUE)
if(FLEX AND BISON AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zconf.l" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zconf.y")
    option(KCONFIG_REGENERATE_PARSERS "Regenerate parser files with flex/bison instead of using shipped versions" OFF)
    if(KCONFIG_REGENERATE_PARSERS)
        set(USE_SHIPPED_PARSERS FALSE)
    endif()
endif()

if(USE_SHIPPED_PARSERS)
    message(STATUS "Using shipped parser files (zconf.*.c_shipped)")

    # Extract header from zconf.tab.c_shipped FIRST
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/extract_header.sh ${CMAKE_CURRENT_SOURCE_DIR}/zconf.tab.c_shipped ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.tab.c_shipped ${CMAKE_CURRENT_SOURCE_DIR}/extract_header.sh
        COMMENT "Extracting zconf.tab.h from zconf.tab.c_shipped"
    )

    # Copy zconf.lex.c_shipped (depends on header being available)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/zconf.lex.c_shipped ${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.lex.c_shipped ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
        COMMENT "Copying zconf.lex.c_shipped"
    )

    # Copy zconf.tab.c_shipped (depends on header and lexer)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/zconf.tab.c_shipped ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.tab.c_shipped ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h ${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c
        COMMENT "Copying zconf.tab.c_shipped"
    )

    # The shipped zconf.tab.c includes all other source files at the end
    set(COMMON_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c
    )

    set(PARSER_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
    )
else()
    message(STATUS "Regenerating parser files with flex and bison")

    # Generate parser first (creates both .c and .h)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
        COMMAND ${BISON} -l -p zconf -d -o ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c ${CMAKE_CURRENT_SOURCE_DIR}/zconf.y
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.y
        COMMENT "Generating parser with bison"
    )

    # Generate lexer (depends on parser header)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c
        COMMAND ${FLEX} -L -Pzconf -o${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c ${CMAKE_CURRENT_SOURCE_DIR}/zconf.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.l ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
        COMMENT "Generating lexer with flex"
    )

    set(COMMON_SOURCES
        confdata.c
        expr.c
        symbol.c
        menu.c
        util.c
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.lex.c
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.c
    )

    set(PARSER_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.tab.h
    )
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lxdialog
    ${CMAKE_CURRENT_BINARY_DIR}
    ${NCURSES_INCLUDE_DIRS}
)

# Compiler flags
add_compile_definitions(_GNU_SOURCE)
add_compile_options(-Wall -Wno-unused-function -Wno-format-security -Wno-format-overflow)

# Build lxdialog library for menuconfig
set(LXDIALOG_SOURCES
    lxdialog/checklist.c
    lxdialog/inputbox.c
    lxdialog/menubox.c
    lxdialog/textbox.c
    lxdialog/util.c
    lxdialog/yesno.c
)

add_library(lxdialog STATIC ${LXDIALOG_SOURCES})
target_compile_options(lxdialog PRIVATE "-DCURSES_LOC=<ncurses.h>")
target_link_libraries(lxdialog ${NCURSES_LIBRARIES})

# Build conf (console configuration tool)
add_executable(conf conf.c ${COMMON_SOURCES})

# Build mconf (menuconfig - ncurses-based GUI)
add_executable(mconf mconf.c ${COMMON_SOURCES})
target_compile_options(mconf PRIVATE "-DCURSES_LOC=<ncurses.h>")
target_link_libraries(mconf lxdialog ${NCURSES_LIBRARIES})

# Build nconf (nconfig - alternative ncurses GUI)
add_executable(nconf nconf.c nconf.gui.c ${COMMON_SOURCES})
target_compile_options(nconf PRIVATE "-DCURSES_LOC=<ncurses.h>")
target_link_libraries(nconf ${NCURSES_LIBRARIES} menu panel)

# Optional: Build gconf (GTK-based GUI) if GTK2 is available
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK2 gtk+-2.0 gmodule-2.0 glib-2.0)
    if(GTK2_FOUND)
        add_executable(gconf gconf.c images.c ${COMMON_SOURCES})
        target_include_directories(gconf PRIVATE ${GTK2_INCLUDE_DIRS})
        target_link_libraries(gconf ${GTK2_LIBRARIES})
        message(STATUS "GTK2 found, will build gconf")
    else()
        message(STATUS "GTK2 not found, skipping gconf")
    endif()
endif()

# Installation rules
install(TARGETS conf mconf nconf
    RUNTIME DESTINATION bin
)

# Install shell scripts
install(PROGRAMS
    merge_config.sh
    DESTINATION bin
)

# Print build information
message(STATUS "============================================")
message(STATUS "Kconfig tools configuration:")
message(STATUS "  - conf: Console configuration tool")
message(STATUS "  - mconf: Menuconfig (ncurses)")
message(STATUS "  - nconf: Nconfig (ncurses alternative)")
message(STATUS "  - NCURSES: ${NCURSES_FOUND}")
message(STATUS "  - Using shipped parsers: ${USE_SHIPPED_PARSERS}")
if(FLEX)
    message(STATUS "  - FLEX: ${FLEX}")
endif()
if(BISON)
    message(STATUS "  - BISON: ${BISON}")
endif()
message(STATUS "============================================")
