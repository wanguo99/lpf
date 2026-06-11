# ============================================================================
# Test Discovery and Auto-Registration
# ============================================================================
# This module provides functions to automatically discover test files and
# match them to Kconfig options, eliminating the need for manual file lists.
#
# Design Philosophy:
# - Convention over configuration: test file names map directly to Kconfig options
# - Automatic validation: CMake verifies that enabled tests have corresponding files
# - Clear error messages: actionable guidance when configuration is inconsistent
#
# File Naming Convention:
#   test_<module>_<name>.c  -->  CONFIG_TEST_<MODULE>_<NAME>
#
# Examples:
#   test_osal_mutex.c       -->  CONFIG_TEST_OSAL_MUTEX
#   test_hal_can.c          -->  CONFIG_TEST_HAL_CAN
#   test_pdl_bmc.c          -->  CONFIG_TEST_PDL_BMC
#
# Error Detection and Reporting:
# - Missing Kconfig options: Warns when test file exists but config is undefined
# - Naming violations: Warns when test file doesn't follow convention
# - Configuration statistics: Reports enabled/disabled/missing counts
# - Actionable guidance: Provides step-by-step fix instructions
#
# For detailed documentation on error handling and validation, see:
#   products/tests/docs/ERROR_HANDLING.md
# ============================================================================

# ----------------------------------------------------------------------------
# test_discover_and_add
# ----------------------------------------------------------------------------
# Discover test files in a directory and add them to the target if their
# corresponding Kconfig options are enabled.
#
# Usage:
#   test_discover_and_add(
#       TARGET <target_name>
#       DIRECTORY <path_to_test_files>
#       MODULE <module_name>
#       [LIBRARY <library_to_link>]
#       [INCLUDE_DIRS <additional_include_dirs>...]
#   )
#
# Parameters:
#   TARGET - CMake target to add test sources to (e.g., es-middleware-test)
#   DIRECTORY - Absolute path to directory containing test_*.c files
#   MODULE - Module name used to derive Kconfig option (e.g., OSAL, HAL)
#   LIBRARY - Optional library to link (e.g., osal, hal)
#   INCLUDE_DIRS - Optional additional include directories
#
# Behavior:
#   1. Scans DIRECTORY for test_<module>_*.c files
#   2. Derives Kconfig option name: test_<module>_<name>.c -> CONFIG_TEST_<MODULE>_<NAME>
#   3. If CONFIG option is enabled, adds file to TARGET
#   4. If CONFIG option is not defined in .config, warns user
#   5. Optionally links LIBRARY if provided
#   6. Tracks statistics (files found, enabled, disabled)
# ----------------------------------------------------------------------------
function(test_discover_and_add)
    # Parse arguments
    set(options "")
    set(oneValueArgs TARGET DIRECTORY MODULE LIBRARY)
    set(multiValueArgs INCLUDE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_TARGET)
        message(FATAL_ERROR "test_discover_and_add: TARGET is required")
    endif()
    if(NOT ARG_DIRECTORY)
        message(FATAL_ERROR "test_discover_and_add: DIRECTORY is required")
    endif()
    if(NOT ARG_MODULE)
        message(FATAL_ERROR "test_discover_and_add: MODULE is required")
    endif()

    # Convert module name to uppercase for Kconfig matching
    string(TOUPPER "${ARG_MODULE}" MODULE_UPPER)

    # Glob all test files in the directory
    file(GLOB TEST_FILES "${ARG_DIRECTORY}/test_*.c")

    # Statistics tracking
    set(FOUND_COUNT 0)
    set(ENABLED_COUNT 0)
    set(DISABLED_COUNT 0)
    set(MISSING_CONFIG_COUNT 0)
    set(ENABLED_FILES "")

    # Process each test file
    foreach(TEST_FILE ${TEST_FILES})
        math(EXPR FOUND_COUNT "${FOUND_COUNT} + 1")

        # Extract filename without extension
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

        # Validate file naming convention
        if(NOT TEST_NAME MATCHES "^test_")
            message(WARNING
                "Test file '${TEST_NAME}.c' does not follow naming convention.\n"
                "  File: ${TEST_FILE}\n"
                "  Expected: test_<module>_<name>.c\n"
                "  Action: Rename the file to match convention, or move to non-test directory."
            )
        endif()

        # Derive Kconfig option name from filename
        # Example: test_osal_mutex -> CONFIG_TEST_OSAL_MUTEX
        string(TOUPPER "${TEST_NAME}" CONFIG_NAME)
        set(CONFIG_NAME "CONFIG_${CONFIG_NAME}")

        # Check if the Kconfig option is defined and enabled
        if(DEFINED ${CONFIG_NAME})
            if(${CONFIG_NAME})
                # Config is enabled - add the test file
                target_sources(${ARG_TARGET} PRIVATE ${TEST_FILE})
                list(APPEND ENABLED_FILES ${TEST_NAME})
                math(EXPR ENABLED_COUNT "${ENABLED_COUNT} + 1")
            else()
                # Config is defined but disabled
                math(EXPR DISABLED_COUNT "${DISABLED_COUNT} + 1")
            endif()
        else()
            # Config option not found in .config - this is a configuration error
            math(EXPR MISSING_CONFIG_COUNT "${MISSING_CONFIG_COUNT} + 1")

            # Determine the correct Kconfig file path based on directory structure
            get_filename_component(PARENT_DIR ${ARG_DIRECTORY} NAME)
            get_filename_component(GRANDPARENT_DIR ${ARG_DIRECTORY} DIRECTORY)
            get_filename_component(CATEGORY ${GRANDPARENT_DIR} NAME)

            message(WARNING
                "Test file '${TEST_NAME}.c' found but ${CONFIG_NAME} not defined in Kconfig.\n"
                "  File: ${TEST_FILE}\n"
                "  Expected config: ${CONFIG_NAME}\n"
                "  Location: products/tests/${CATEGORY}/${PARENT_DIR}/Kconfig\n"
                "\n"
                "  Actionable steps:\n"
                "  1. Add the following to products/tests/${CATEGORY}/${PARENT_DIR}/Kconfig:\n"
                "\n"
                "     config ${CONFIG_NAME}\n"
                "         bool \"<Test description>\"\n"
                "         default y\n"
                "         help\n"
                "           Test <functionality description>.\n"
                "           Runtime: <estimated time>\n"
                "           Hardware: <hardware requirements>\n"
                "           Dependencies: CONFIG_${MODULE_UPPER}\n"
                "\n"
                "  2. Update the ${CONFIG_NAME}_ALL option to select ${CONFIG_NAME}\n"
                "  3. Run: make menuconfig\n"
                "  4. Or remove the test file if it's obsolete or incorrectly named."
            )
        endif()
    endforeach()

    # Link optional library if provided and enabled
    if(ARG_LIBRARY AND TARGET ${ARG_LIBRARY})
        if(ENABLED_COUNT GREATER 0)
            target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_LIBRARY})
        endif()
    endif()

    # Add optional include directories
    if(ARG_INCLUDE_DIRS)
        if(ENABLED_COUNT GREATER 0)
            target_include_directories(${ARG_TARGET} PRIVATE ${ARG_INCLUDE_DIRS})
        endif()
    endif()

    # Report discovery statistics
    if(FOUND_COUNT GREATER 0)
        message(STATUS "  ${ARG_MODULE}: ${ENABLED_COUNT}/${FOUND_COUNT} tests enabled")
        if(ENABLED_COUNT GREATER 0)
            # Show list of enabled tests for verification
            string(REPLACE ";" ", " ENABLED_LIST "${ENABLED_FILES}")
            message(STATUS "    Enabled: ${ENABLED_LIST}")
        endif()
        if(DISABLED_COUNT GREATER 0)
            message(STATUS "    Disabled: ${DISABLED_COUNT} test(s)")
        endif()
    else()
        message(STATUS "  ${ARG_MODULE}: No test files found in ${ARG_DIRECTORY}")
    endif()

    # Report configuration issues with severity level
    if(MISSING_CONFIG_COUNT GREATER 0)
        message(WARNING
            "${ARG_MODULE}: ${MISSING_CONFIG_COUNT} test file(s) lack corresponding Kconfig options.\n"
            "  This indicates a mismatch between test files and Kconfig definitions.\n"
            "  Review the warnings above for detailed instructions.\n"
            "  Run 'make menuconfig' to verify configuration after fixing."
        )
    endif()

    # Export statistics for potential use by parent scope
    set(TEST_STATS_${MODULE_UPPER}_FOUND ${FOUND_COUNT} PARENT_SCOPE)
    set(TEST_STATS_${MODULE_UPPER}_ENABLED ${ENABLED_COUNT} PARENT_SCOPE)
    set(TEST_STATS_${MODULE_UPPER}_DISABLED ${DISABLED_COUNT} PARENT_SCOPE)
    set(TEST_STATS_${MODULE_UPPER}_MISSING ${MISSING_CONFIG_COUNT} PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------------
# test_discover_category
# ----------------------------------------------------------------------------
# Discover and add all test files for a test category (unit, performance, etc.)
# across multiple modules/layers.
#
# Usage:
#   test_discover_category(
#       TARGET <target_name>
#       CATEGORY <category_path>
#       MODULES <module1> <module2> ...
#   )
#
# Parameters:
#   TARGET - CMake target to add test sources to
#   CATEGORY - Test category (unit, performance, stress, system)
#   MODULES - List of module subdirectories to scan
#
# Example:
#   test_discover_category(
#       TARGET es-middleware-test
#       CATEGORY "unit"
#       MODULES osal hal pdl prl pconfig aconfig
#   )
# ----------------------------------------------------------------------------
function(test_discover_category)
    set(options "")
    set(oneValueArgs TARGET CATEGORY)
    set(multiValueArgs MODULES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "test_discover_category: TARGET is required")
    endif()
    if(NOT ARG_CATEGORY)
        message(FATAL_ERROR "test_discover_category: CATEGORY is required")
    endif()
    if(NOT ARG_MODULES)
        message(FATAL_ERROR "test_discover_category: MODULES is required")
    endif()

    message(STATUS "Discovering ${ARG_CATEGORY} tests:")

    foreach(MODULE ${ARG_MODULES})
        string(TOUPPER "${MODULE}" MODULE_UPPER)
        set(MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_CATEGORY}/${MODULE}")

        # Check if module directory exists
        if(EXISTS "${MODULE_DIR}")
            # Determine library name (handle special cases)
            set(LIBRARY_NAME "${MODULE}")
            if(MODULE STREQUAL "pcl")
                set(LIBRARY_NAME "pconfig")
            elseif(MODULE STREQUAL "acl")
                set(LIBRARY_NAME "aconfig")
            endif()

            # Call test_discover_and_add for this module
            test_discover_and_add(
                TARGET ${ARG_TARGET}
                DIRECTORY ${MODULE_DIR}
                MODULE ${MODULE}
                LIBRARY ${LIBRARY_NAME}
            )
        endif()
    endforeach()
endfunction()

# ----------------------------------------------------------------------------
# test_validate_naming
# ----------------------------------------------------------------------------
# Validate that test file names follow the required convention and match
# their Kconfig options.
#
# Usage:
#   test_validate_naming(
#       DIRECTORY <path_to_test_files>
#       MODULE <module_name>
#   )
#
# Checks:
#   1. All test files start with "test_"
#   2. Test file names are valid C identifiers (no spaces, special chars)
#   3. Corresponding Kconfig options exist
# ----------------------------------------------------------------------------
function(test_validate_naming)
    set(options "")
    set(oneValueArgs DIRECTORY MODULE)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_DIRECTORY OR NOT ARG_MODULE)
        message(FATAL_ERROR "test_validate_naming: DIRECTORY and MODULE are required")
    endif()

    file(GLOB TEST_FILES "${ARG_DIRECTORY}/*.c")
    string(TOUPPER "${ARG_MODULE}" MODULE_UPPER)

    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME)

        # Check if filename starts with "test_"
        if(NOT TEST_NAME MATCHES "^test_")
            message(WARNING
                "Test file '${TEST_NAME}' does not follow naming convention.\n"
                "  Expected: test_<module>_<name>.c\n"
                "  Location: ${TEST_FILE}"
            )
        endif()

        # Check for invalid characters (anything not alphanumeric or underscore)
        if(TEST_NAME MATCHES "[^a-zA-Z0-9_\\.]")
            message(WARNING
                "Test file '${TEST_NAME}' contains invalid characters.\n"
                "  Use only: a-z, A-Z, 0-9, underscore\n"
                "  Location: ${TEST_FILE}"
            )
        endif()
    endforeach()
endfunction()

# ----------------------------------------------------------------------------
# test_validate_kconfig_consistency
# ----------------------------------------------------------------------------
# Validate consistency between Kconfig options and test files.
# Detects orphaned Kconfig options (configs without corresponding test files).
#
# Usage:
#   test_validate_kconfig_consistency(
#       DIRECTORY <path_to_test_files>
#       MODULE <module_name>
#       CONFIGS <list_of_config_names>
#   )
#
# Parameters:
#   DIRECTORY - Path to test files
#   MODULE - Module name
#   CONFIGS - List of CONFIG_TEST_* option names (without CONFIG_ prefix)
#
# Example:
#   test_validate_kconfig_consistency(
#       DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/osal
#       MODULE OSAL
#       CONFIGS TEST_OSAL_MUTEX TEST_OSAL_THREAD TEST_OSAL_FILE
#   )
# ----------------------------------------------------------------------------
function(test_validate_kconfig_consistency)
    set(options "")
    set(oneValueArgs DIRECTORY MODULE)
    set(multiValueArgs CONFIGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_DIRECTORY OR NOT ARG_MODULE)
        message(FATAL_ERROR "test_validate_kconfig_consistency: DIRECTORY and MODULE required")
    endif()

    # Get all test files in directory
    file(GLOB TEST_FILES "${ARG_DIRECTORY}/test_*.c")
    set(EXISTING_FILES "")
    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        string(TOUPPER "${TEST_NAME}" CONFIG_NAME)
        list(APPEND EXISTING_FILES "CONFIG_${CONFIG_NAME}")
    endforeach()

    # Check each provided config for corresponding file
    set(ORPHANED_CONFIGS "")
    foreach(CONFIG_NAME ${ARG_CONFIGS})
        # Skip meta configs like TEST_*_ALL
        if(CONFIG_NAME MATCHES "_ALL$")
            continue()
        endif()

        # Add CONFIG_ prefix if not present
        if(NOT CONFIG_NAME MATCHES "^CONFIG_")
            set(FULL_CONFIG_NAME "CONFIG_${CONFIG_NAME}")
        else()
            set(FULL_CONFIG_NAME "${CONFIG_NAME}")
        endif()

        # Check if this config has a corresponding file
        list(FIND EXISTING_FILES "${FULL_CONFIG_NAME}" FILE_INDEX)
        if(FILE_INDEX EQUAL -1)
            list(APPEND ORPHANED_CONFIGS "${FULL_CONFIG_NAME}")
        endif()
    endforeach()

    # Report orphaned configs
    if(ORPHANED_CONFIGS)
        list(LENGTH ORPHANED_CONFIGS ORPHAN_COUNT)
        string(REPLACE ";" "\n      " ORPHAN_LIST "${ORPHANED_CONFIGS}")
        message(WARNING
            "${ARG_MODULE}: ${ORPHAN_COUNT} Kconfig option(s) have no corresponding test files:\n"
            "      ${ORPHAN_LIST}\n"
            "\n"
            "  Possible causes:\n"
            "  - Test file was deleted but Kconfig option remains\n"
            "  - Test file name doesn't match Kconfig option\n"
            "  - Kconfig option name has typo\n"
            "\n"
            "  Actionable steps:\n"
            "  1. Create missing test files, or\n"
            "  2. Remove orphaned Kconfig options from products/tests/.../Kconfig\n"
            "  3. Update corresponding _ALL selection lists"
        )
    endif()
endfunction()

# ============================================================================
# Usage Documentation
# ============================================================================
#
# To migrate from manual file lists to auto-discovery:
#
# 1. Replace this:
#    if(CONFIG_TEST_OSAL)
#        target_sources(es-middleware-test PRIVATE
#            unit/osal/test_osal_mutex.c
#            unit/osal/test_osal_thread.c
#            # ... 17 more files ...
#        )
#        target_link_libraries(es-middleware-test PRIVATE osal)
#    endif()
#
# 2. With this:
#    if(CONFIG_TEST_OSAL)
#        test_discover_and_add(
#            TARGET es-middleware-test
#            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit/osal
#            MODULE osal
#            LIBRARY osal
#        )
#    endif()
#
# 3. Or use the category-level function:
#    if(CONFIG_TEST_UNIT)
#        test_discover_category(
#            TARGET es-middleware-test
#            CATEGORY "unit"
#            MODULES osal hal pdl prl pconfig aconfig
#        )
#    endif()
#
# Benefits:
# - No manual file list maintenance
# - Automatic validation of Kconfig options
# - Clear error messages when configuration is wrong
# - Statistics show exactly which tests are enabled
# ============================================================================
