/*
 * EMS Version Information Utility
 *
 * This file demonstrates how to use version information in your code.
 */

#include <stdio.h>
#include "global_build_info_version.h"
#include "global_build_info_time.h"

// 构建版本字符串
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define EMS_VERSION_STRING TOSTRING(BUILD_VERSION_MAJOR) "." TOSTRING(BUILD_VERSION_MINOR) "." TOSTRING(BUILD_VERSION_MICRO)
#define EMS_FULL_VERSION EMS_VERSION_STRING
#define EMS_GIT_HASH BUILD_GIT_COMMIT_ID
#define EMS_BUILD_DATE TOSTRING(BUILD_TIME_YEAR) "-" TOSTRING(BUILD_TIME_MONTH) "-" TOSTRING(BUILD_TIME_DAY)
#define EMS_BUILD_TIME TOSTRING(BUILD_TIME_HOUR) ":" TOSTRING(BUILD_TIME_MINUTE) ":" TOSTRING(BUILD_TIME_SECOND)

#ifdef DEBUG
#define EMS_BUILD_TYPE "Debug"
#else
#define EMS_BUILD_TYPE "Release"
#endif

#define EMS_COMPILER __VERSION__

#ifdef __linux__
#define EMS_SYSTEM "Linux"
#elif defined(__APPLE__)
#define EMS_SYSTEM "macOS"
#elif defined(_WIN32)
#define EMS_SYSTEM "Windows"
#else
#define EMS_SYSTEM "Unknown"
#endif

void print_version_info(void)
{
    printf("=== EMS Version Information ===\n");
    printf("Version:      %s\n", EMS_VERSION_STRING);
    printf("Full Version: %s\n", EMS_FULL_VERSION);
    printf("Git Hash:     %s\n", EMS_GIT_HASH);
    printf("Build Date:   %s\n", EMS_BUILD_DATE);
    printf("Build Time:   %s\n", EMS_BUILD_TIME);
    printf("Build Type:   %s\n", EMS_BUILD_TYPE);
    printf("Compiler:     %s\n", EMS_COMPILER);
    printf("System:       %s\n", EMS_SYSTEM);
    printf("===============================\n");
}

const char* get_version_string(void)
{
    return EMS_FULL_VERSION;
}
