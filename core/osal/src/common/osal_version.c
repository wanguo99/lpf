/*
 * ES-Middleware Version Information Utility
 *
 * This file demonstrates how to use version information in your code.
 */

#include <stdio.h>
#include "global_build_info_version.h"
#include "global_build_info_time.h"

// 构建版本字符串
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ES_MIDDLEWARE_VERSION_STRING TOSTRING(BUILD_VERSION_MAJOR) "." TOSTRING(BUILD_VERSION_MINOR) "." TOSTRING(BUILD_VERSION_MICRO)
#define ES_MIDDLEWARE_FULL_VERSION ES_MIDDLEWARE_VERSION_STRING
#define ES_MIDDLEWARE_GIT_HASH BUILD_GIT_COMMIT_ID
#define ES_MIDDLEWARE_BUILD_DATE TOSTRING(BUILD_TIME_YEAR) "-" TOSTRING(BUILD_TIME_MONTH) "-" TOSTRING(BUILD_TIME_DAY)
#define ES_MIDDLEWARE_BUILD_TIME TOSTRING(BUILD_TIME_HOUR) ":" TOSTRING(BUILD_TIME_MINUTE) ":" TOSTRING(BUILD_TIME_SECOND)

#ifdef DEBUG
#define ES_MIDDLEWARE_BUILD_TYPE "Debug"
#else
#define ES_MIDDLEWARE_BUILD_TYPE "Release"
#endif

#define ES_MIDDLEWARE_COMPILER __VERSION__

#ifdef __linux__
#define ES_MIDDLEWARE_SYSTEM "Linux"
#elif defined(__APPLE__)
#define ES_MIDDLEWARE_SYSTEM "macOS"
#elif defined(_WIN32)
#define ES_MIDDLEWARE_SYSTEM "Windows"
#else
#define ES_MIDDLEWARE_SYSTEM "Unknown"
#endif

void print_version_info(void)
{
    printf("=== ES-Middleware Version Information ===\n");
    printf("Version:      %s\n", ES_MIDDLEWARE_VERSION_STRING);
    printf("Full Version: %s\n", ES_MIDDLEWARE_FULL_VERSION);
    printf("Git Hash:     %s\n", ES_MIDDLEWARE_GIT_HASH);
    printf("Build Date:   %s\n", ES_MIDDLEWARE_BUILD_DATE);
    printf("Build Time:   %s\n", ES_MIDDLEWARE_BUILD_TIME);
    printf("Build Type:   %s\n", ES_MIDDLEWARE_BUILD_TYPE);
    printf("Compiler:     %s\n", ES_MIDDLEWARE_COMPILER);
    printf("System:       %s\n", ES_MIDDLEWARE_SYSTEM);
    printf("==========================================\n");
}

const char* get_version_string(void)
{
    return ES_MIDDLEWARE_FULL_VERSION;
}
