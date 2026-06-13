/*
 * Example program demonstrating ES-Middleware version information usage
 * Similar to Linux kernel's version display
 */

#include <stdio.h>
#include "version.h"
#include "autoconf.h"

int main(void)
{
    printf("=====================================================\n");
    printf("ES-Middleware Version Information\n");
    printf("=====================================================\n\n");

    /* Display full version banner (like Linux kernel) */
    printf("Banner:\n");
    printf("  %s\n\n", ES_MIDDLEWARE_BANNER);

    /* Display detailed version information */
    printf("Version Details:\n");
    printf("  Version:           %s\n", ES_MIDDLEWARE_VERSION);
    printf("  Version String:    %s\n", ES_MIDDLEWARE_VERSION_STRING);
    printf("  Git Commit:        %s\n\n", ES_MIDDLEWARE_GIT_COMMIT);

    /* Display build information */
    printf("Build Information:\n");
    printf("  Built by:          %s@%s\n",
           ES_MIDDLEWARE_COMPILE_BY, ES_MIDDLEWARE_COMPILE_HOST);
    printf("  Compiler:          %s\n", ES_MIDDLEWARE_COMPILER);
    printf("  Build time:        %s\n", ES_MIDDLEWARE_COMPILE_TIME);
    printf("  Build timestamp:   %ld\n\n", ES_MIDDLEWARE_COMPILE_TIMESTAMP);

    /* Display platform information */
    printf("Platform Information:\n");
    printf("  Architecture:      %s\n", ES_MIDDLEWARE_BUILD_ARCH);
    printf("  Kernel:            %s\n\n", ES_MIDDLEWARE_BUILD_KERNEL);

    /* Display some configuration from autoconf.h */
    printf("Configuration:\n");
#ifdef CONFIG_OSAL_POSIX
    printf("  OSAL:              POSIX\n");
#endif
#ifdef CONFIG_HAL_X86
    printf("  HAL:               x86\n");
#endif
#ifdef CONFIG_DEBUG
    printf("  Debug:             Enabled\n");
#else
    printf("  Debug:             Disabled\n");
#endif

    printf("\n=====================================================\n");

    return 0;
}
