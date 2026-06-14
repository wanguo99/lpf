/*
 * Example program demonstrating ES-Middleware version information usage
 * Shows how to use OSAL version APIs and convenience macros
 */

#include "osal.h"

int main(void)
{
    /* Method 1: Use the high-level print function */
    OSAL_printf("\n=== Method 1: Using OSAL_PrintVersionInfo() ===\n\n");
    OSAL_PrintVersionInfo();

    /* Method 2: Query individual version fields */
    OSAL_printf("\n=== Method 2: Using individual query APIs ===\n\n");
    OSAL_printf("Version:      %s\n", OSAL_GetVersion());
    OSAL_printf("Full Version: %s\n", OSAL_GetVersionFull());
    OSAL_printf("Git Commit:   %s\n", OSAL_GetGitCommit());
    OSAL_printf("Build Time:   %s\n", OSAL_GetBuildTime());
    OSAL_printf("Built by:     %s\n", OSAL_GetBuildBy());
    OSAL_printf("Compiler:     %s\n", OSAL_GetCompiler());
    OSAL_printf("Architecture: %s\n", OSAL_GetArch());
    OSAL_printf("Kernel:       %s\n", OSAL_GetKernel());

    /* Method 3: Use convenience macros (kernel-style) */
    OSAL_printf("\n=== Method 3: Using convenience macros ===\n\n");
    OSAL_printf("OSAL_VERSION:      %s\n", OSAL_VERSION);
    OSAL_printf("OSAL_VERSION_FULL: %s\n", OSAL_VERSION_FULL);
    OSAL_printf("OSAL_GIT_COMMIT:   %s\n", OSAL_GIT_COMMIT);
    OSAL_printf("OSAL_BUILD_TIME:   %s\n", OSAL_BUILD_TIME);

    /* Method 4: Use location macros (useful for logging) */
    OSAL_printf("\n=== Method 4: Location and debug macros ===\n\n");
    OSAL_printf("Current file:     %s\n", OSAL_FILE);
    OSAL_printf("Current line:     %d\n", OSAL_LINE);
    OSAL_printf("Current function: %s\n", OSAL_FUNC);
    OSAL_printf("Location string:  %s\n", OSAL_LOCATION);
    OSAL_printf("Version+Location: %s\n", OSAL_VERSION_LOCATION);

    OSAL_printf("\n");
    return 0;
}
