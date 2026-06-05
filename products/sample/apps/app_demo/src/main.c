#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("=================================\n");
    printf("  Sample Product Demo Application\n");
    printf("=================================\n\n");

    // 打印版本信息
    print_version_info();

    printf("\nDemo application running...\n");
    printf("This is a sample application for demonstration purposes.\n");

    return 0;
}
