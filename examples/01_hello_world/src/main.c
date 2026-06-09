#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("=================================\n");
    printf("  Hello, ES-Middleware SDK!\n");
    printf("=================================\n\n");

    // 使用 OSAL API 打印版本信息
    print_version_info();

    // 你的代码
    printf("\nHello from ES-Middleware SDK!\n");
    printf("This is the simplest example.\n");
    printf("\n");
    printf("Next steps:\n");
    printf("  1. Modify this code and rebuild\n");
    printf("  2. Try example 02_osal_thread\n");
    printf("  3. Read the documentation\n");

    return 0;
}
