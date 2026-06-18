/************************************************************************
 * OSAL 平台检测和配置
 *
 * 提供统一的平台检测宏，用于条件编译
 * 支持 Kconfig 配置和编译器自动检测
 ************************************************************************/

#ifndef OSAL_PLATFORM_H
#define OSAL_PLATFORM_H

/*
 * ========================================================================
 * 操作系统检测（优先使用 Kconfig 配置）
 * ========================================================================
 */
#if defined(OSAL_OS_POSIX)
/* POSIX 平台（Linux、macOS 等） */
#define OSAL_PLATFORM_POSIX
#if defined(__linux__)
#define OSAL_PLATFORM_LINUX
#elif defined(__APPLE__)
#define OSAL_PLATFORM_MACOS
#endif
#elif defined(OSAL_OS_WIN32)
/* Windows 平台 */
#define OSAL_PLATFORM_WINDOWS
#elif defined(OSAL_OS_RTOS)
/* RTOS 平台 */
#define OSAL_PLATFORM_RTOS
#if defined(__FREERTOS__)
#define OSAL_PLATFORM_FREERTOS
#elif defined(__RTEMS__)
#define OSAL_PLATFORM_RTEMS
#endif
#elif defined(OSAL_OS_BARE)
/* 裸机平台 */
#define OSAL_PLATFORM_BARE
#else
/* 如果没有 Kconfig 配置，使用编译器自动检测 */
#if defined(__linux__)
#define OSAL_PLATFORM_POSIX
#define OSAL_PLATFORM_LINUX
#elif defined(__APPLE__)
#define OSAL_PLATFORM_POSIX
#define OSAL_PLATFORM_MACOS
#elif defined(_WIN32) || defined(_WIN64)
#define OSAL_PLATFORM_WINDOWS
#elif defined(__FREERTOS__)
#define OSAL_PLATFORM_RTOS
#define OSAL_PLATFORM_FREERTOS
#elif defined(__RTEMS__)
#define OSAL_PLATFORM_RTOS
#define OSAL_PLATFORM_RTEMS
#else
#warning "Unknown platform, assuming POSIX"
#define OSAL_PLATFORM_POSIX
#endif
#endif

/*
 * ========================================================================
 * 架构检测（优先使用 Kconfig 配置）
 * ========================================================================
 */
#if defined(OSAL_ARCH_32BIT)
/* 32 位架构 */
#define OSAL_ARCH_BITS 0x20
#if defined(__arm__) || defined(_M_ARM)
#define OSAL_ARCH_ARM32
#elif defined(__i386__) || defined(_M_IX86)
#define OSAL_ARCH_X86
#elif defined(__riscv) && (__riscv_xlen == 32)
#define OSAL_ARCH_RISCV32
#endif
#elif defined(OSAL_ARCH_64BIT)
/* 64 位架构 */
#define OSAL_ARCH_BITS 0x40
#if defined(__x86_64__) || defined(_M_X64)
#define OSAL_ARCH_X86_64
#elif defined(__aarch64__) || defined(_M_ARM64)
#define OSAL_ARCH_ARM64
#elif defined(__riscv) && (__riscv_xlen == 64)
#define OSAL_ARCH_RISCV64
#endif
#else
/* 如果没有 Kconfig 配置，使用编译器自动检测 */
#if defined(__x86_64__) || defined(_M_X64)
#define OSAL_ARCH_X86_64
#define OSAL_ARCH_BITS 0x40
#elif defined(__i386__) || defined(_M_IX86)
#define OSAL_ARCH_X86
#define OSAL_ARCH_BITS 0x20
#elif defined(__aarch64__) || defined(_M_ARM64)
#define OSAL_ARCH_ARM64
#define OSAL_ARCH_BITS 0x40
#elif defined(__arm__) || defined(_M_ARM)
#define OSAL_ARCH_ARM32
#define OSAL_ARCH_BITS 0x20
#elif defined(__riscv) && (__riscv_xlen == 64)
#define OSAL_ARCH_RISCV64
#define OSAL_ARCH_BITS 0x40
#elif defined(__riscv) && (__riscv_xlen == 32)
#define OSAL_ARCH_RISCV32
#define OSAL_ARCH_BITS 0x20
#endif
#endif

/*
 * ========================================================================
 * 编译器检测
 * ========================================================================
 */
#if defined(__GNUC__) || defined(__clang__)
#define OSAL_COMPILER_GCC
#elif defined(_MSC_VER)
#define OSAL_COMPILER_MSVC
#endif

/*
 * ========================================================================
 * 平台相关宏
 * ========================================================================
 */

/* 字节序检测 */
#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define OSAL_LITTLE_ENDIAN 0x1
#define OSAL_BIG_ENDIAN 0x0
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define OSAL_LITTLE_ENDIAN 0x0
#define OSAL_BIG_ENDIAN 0x1
#endif
#else
/* 默认假设小端 */
#define OSAL_LITTLE_ENDIAN 0x1
#define OSAL_BIG_ENDIAN 0x0
#endif

/* 打包属性宏 */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_PACKED __attribute__((packed))
#elif defined(OSAL_COMPILER_MSVC)
#define OSAL_PACKED
#else
#define OSAL_PACKED
#endif

/* 内联宏 */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_INLINE static inline __attribute__((always_inline))
#elif defined(OSAL_COMPILER_MSVC)
#define OSAL_INLINE static __forceinline
#else
#define OSAL_INLINE static inline
#endif

/* 导出符号宏 */
#if defined(OSAL_PLATFORM_WINDOWS)
#ifdef OSAL_BUILD_SHARED
#define OSAL_API __declspec(dllexport)
#else
#define OSAL_API __declspec(dllimport)
#endif
#elif defined(OSAL_COMPILER_GCC)
#define OSAL_API __attribute__((visibility("default")))
#else
#define OSAL_API
#endif

/*
 * ========================================================================
 * 平台相关类型定义
 * ========================================================================
 */

/* 指针大小 */
#if OSAL_ARCH_BITS == 64
typedef unsigned long long osal_ptr_t;
typedef long long osal_sptr_t;
#else
typedef unsigned long osal_ptr_t;
typedef long osal_sptr_t;
#endif

/* 原子类型 */
#if OSAL_ARCH_BITS == 64
typedef long long osal_atomic_t;
#else
typedef int osal_atomic_t;
#endif

/* 对齐宏 */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_ALIGNED(x) __attribute__((aligned(x)))
#elif defined(OSAL_COMPILER_MSVC)
#define OSAL_ALIGNED(x) __declspec(align(x))
#else
#define OSAL_ALIGNED(x)
#endif

#endif /* OSAL_PLATFORM_H */
