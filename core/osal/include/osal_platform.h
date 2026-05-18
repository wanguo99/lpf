/************************************************************************
 * OSAL 平台检测
 *
 * 提供统一的平台检测宏，用于条件编译
 ************************************************************************/

#ifndef OSAL_PLATFORM_H
#define OSAL_PLATFORM_H

/*
 * 平台检测宏
 */
#if defined(__linux__)
    #define OSAL_PLATFORM_POSIX
    #define OSAL_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define OSAL_PLATFORM_POSIX
    #define OSAL_PLATFORM_MACOS
#elif defined(_WIN32) || defined(_WIN64)
    #define OSAL_PLATFORM_WINDOWS
#elif defined(__FREERTOS__)
    #define OSAL_PLATFORM_FREERTOS
#elif defined(__RTEMS__)
    #define OSAL_PLATFORM_RTEMS
#else
    #warning "Unknown platform, assuming POSIX"
    #define OSAL_PLATFORM_POSIX
#endif

/*
 * 编译器检测
 */
#if defined(__GNUC__) || defined(__clang__)
    #define OSAL_COMPILER_GCC
#elif defined(_MSC_VER)
    #define OSAL_COMPILER_MSVC
#endif

/*
 * 架构检测
 */
#if defined(__x86_64__) || defined(_M_X64)
    #define OSAL_ARCH_X86_64
#elif defined(__i386__) || defined(_M_IX86)
    #define OSAL_ARCH_X86
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define OSAL_ARCH_ARM64
#elif defined(__arm__) || defined(_M_ARM)
    #define OSAL_ARCH_ARM32
#elif defined(__riscv) && (__riscv_xlen == 64)
    #define OSAL_ARCH_RISCV64
#elif defined(__riscv) && (__riscv_xlen == 32)
    #define OSAL_ARCH_RISCV32
#endif

#endif /* OSAL_PLATFORM_H */
