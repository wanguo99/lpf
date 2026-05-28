/*
 * EMS Version Information Utility
 *
 * This file demonstrates how to use version information in your code.
 */

#include <stdio.h>
#include "version.h"

void print_version_info(void)
{
    printf("=== EMS Version Information ===\n");
    printf("Version:      %s\n", EMS_VERSION_STRING);
    printf("Full Version: %s\n", EMS_FULL_VERSION);
    printf("Git Hash:     %s\n", EMS_GIT_HASH);
    printf("Git Branch:   %s\n", EMS_GIT_BRANCH);
    printf("Build Date:   %s\n", EMS_BUILD_DATE);
    printf("Build Time:   %s\n", EMS_BUILD_TIME);
    printf("Build Type:   %s\n", EMS_BUILD_TYPE);
    printf("Compiler:     %s\n", EMS_COMPILER);
    printf("System:       %s\n", EMS_SYSTEM);
    printf("Defconfig:    %s\n", EMS_DEFCONFIG);
    printf("===============================\n");
}

const char* get_version_string(void)
{
    return EMS_FULL_VERSION;
}
