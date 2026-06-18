/************************************************************************
 * OSAL Linux platform configuration
 *
 * Provides platform macros for the Linux-only OSAL backend.
 * Kconfig is the primary source of truth; compiler detection is kept as a
 * fallback for direct header use.
 ************************************************************************/

#ifndef OSAL_PLATFORM_H
#define OSAL_PLATFORM_H

/*
 * ========================================================================
 * Operating system detection
 * ========================================================================
 */
#if defined(CONFIG_OSAL_OS_POSIX) || defined(CONFIG_OS_LINUX) || defined(__linux__)
#define OSAL_PLATFORM_POSIX
#define OSAL_PLATFORM_LINUX
#else
#error "OSAL supports Linux only"
#endif

/*
 * ========================================================================
 * Architecture detection
 * ========================================================================
 */
#if defined(CONFIG_OSAL_ARCH_32BIT)
#define OSAL_ARCH_BITS 0x20
#elif defined(CONFIG_OSAL_ARCH_64BIT)
#define OSAL_ARCH_BITS 0x40
#endif

#if defined(CONFIG_ARCH_X86_64) || defined(__x86_64__) || defined(__amd64__)
#define OSAL_ARCH_X86_64
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x40
#endif
#elif defined(CONFIG_ARCH_ARM64) || defined(__aarch64__)
#define OSAL_ARCH_ARM64
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x40
#endif
#elif defined(CONFIG_ARCH_ARM32) || defined(__arm__)
#define OSAL_ARCH_ARM32
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x20
#endif
#elif defined(CONFIG_ARCH_RISCV64) || (defined(__riscv) && (__riscv_xlen == 64))
#define OSAL_ARCH_RISCV64
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x40
#endif
#elif defined(__i386__)
#define OSAL_ARCH_X86
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x20
#endif
#elif defined(__riscv) && (__riscv_xlen == 32)
#define OSAL_ARCH_RISCV32
#ifndef OSAL_ARCH_BITS
#define OSAL_ARCH_BITS 0x20
#endif
#endif

#ifndef OSAL_ARCH_BITS
#error "Unsupported Linux architecture"
#endif

/*
 * ========================================================================
 * Compiler detection
 * ========================================================================
 */
#if defined(__GNUC__) || defined(__clang__)
#define OSAL_COMPILER_GCC
#endif

/*
 * ========================================================================
 * Platform-related macros
 * ========================================================================
 */

/* Byte order detection */
#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define OSAL_LITTLE_ENDIAN 0x1
#define OSAL_BIG_ENDIAN 0x0
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define OSAL_LITTLE_ENDIAN 0x0
#define OSAL_BIG_ENDIAN 0x1
#endif
#else
#define OSAL_LITTLE_ENDIAN 0x1
#define OSAL_BIG_ENDIAN 0x0
#endif

/* Packed attribute */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_PACKED __attribute__((packed))
#else
#define OSAL_PACKED
#endif

/* Inline macro */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_INLINE static inline __attribute__((always_inline))
#else
#define OSAL_INLINE static inline
#endif

/* Exported symbol macro */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_API __attribute__((visibility("default")))
#else
#define OSAL_API
#endif

/*
 * ========================================================================
 * Platform-related type definitions
 * ========================================================================
 */

/* Pointer-sized integer aliases */
#if OSAL_ARCH_BITS == 64
typedef unsigned long long osal_ptr_t;
typedef long long osal_sptr_t;
#else
typedef unsigned long osal_ptr_t;
typedef long osal_sptr_t;
#endif

/* Atomic scalar type */
#if OSAL_ARCH_BITS == 64
typedef long long osal_atomic_t;
#else
typedef int osal_atomic_t;
#endif

/* Alignment macro */
#if defined(OSAL_COMPILER_GCC)
#define OSAL_ALIGNED(x) __attribute__((aligned(x)))
#else
#define OSAL_ALIGNED(x)
#endif

#endif /* OSAL_PLATFORM_H */
