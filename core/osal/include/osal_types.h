/************************************************************************
 * 通用类型定义
 *
 * 跨平台基础类型定义
 * 支持多平台：Linux/RTOS/VxWorks/FreeRTOS等
 ************************************************************************/

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

/*===========================================================================
 * 平台兼容性处理：C99标准类型
 *===========================================================================*/

/* 优先使用C99标准头文件 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    /* C99或更高版本 */
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC/Clang通常提供stdint.h */
    #include <stdint.h>
    #include <stdbool.h>
    #include <stddef.h>
#else
    /* 平台不支持stdint.h，手动定义固定宽度类型 */

    /* 有符号整数类型 */
    typedef signed char        int8_t;
    typedef signed short       int16_t;
    typedef signed int         int32_t;
    typedef signed long long   int64_t;

    /* 无符号整数类型 */
    typedef unsigned char      uint8_t;
    typedef unsigned short     uint16_t;
    typedef unsigned int       uint32_t;
    typedef unsigned long long uint64_t;

    /* 布尔类型 */
    #ifndef __cplusplus
        #ifndef bool
            typedef unsigned char bool;
            #define true  1
            #define false 0
        #endif
    #endif

    /* size_t和NULL */
    #ifndef NULL
        #define NULL ((void *)0)
    #endif

    #ifndef _SIZE_T_DEFINED
        #define _SIZE_T_DEFINED
        typedef unsigned long size_t;
    #endif
#endif

/*===========================================================================
 * OSAL对象ID类型
 *===========================================================================*/

typedef uint32_t osal_id_t;

#define OS_OBJECT_ID_UNDEFINED  ((osal_id_t)0)

/*===========================================================================
 * 平台相关的大小类型
 *===========================================================================*/

/*
 * 平台位宽检测
 * 支持：16位(MSP430/AVR), 32位(ARM32/RISC-V32), 64位(x86_64/ARM64/RISC-V64)
 */
#if defined(__LP64__) || defined(_WIN64) || \
    defined(__x86_64__) || defined(__amd64__) || \
    defined(__aarch64__) || \
    (defined(__riscv) && (__riscv_xlen == 64))
    #define OSAL_PLATFORM_BITS 64
#elif defined(__ILP32__) || defined(_WIN32) || \
      defined(__arm__) || defined(__i386__) || \
      (defined(__riscv) && (__riscv_xlen == 32))
    #define OSAL_PLATFORM_BITS 32
#elif defined(__MSP430__) || defined(__AVR__)
    #define OSAL_PLATFORM_BITS 16
#else
    /* 默认假设32位平台 */
    #define OSAL_PLATFORM_BITS 32
#endif

/*
 * osal_size_t / osal_ssize_t: 平台相关的大小类型
 * - 16位平台：使用16位类型（嵌入式MCU）
 * - 32位平台：使用32位类型（ARM32, RISC-V 32）
 * - 64位平台：使用64位类型（x86_64, ARM64, RISC-V 64）
 * - 保证应用层代码在不同平台间兼容
 */
#if OSAL_PLATFORM_BITS == 64
    typedef uint64_t osal_size_t;
    typedef int64_t  osal_ssize_t;
#elif OSAL_PLATFORM_BITS == 32
    typedef uint32_t osal_size_t;
    typedef int32_t  osal_ssize_t;
#else /* 16-bit */
    typedef uint16_t osal_size_t;
    typedef int16_t  osal_ssize_t;
#endif

/*
 * osal_uintptr_t / osal_intptr_t: 指针大小的整数类型
 * - 用于指针与整数之间的安全转换
 * - 保证能够存储任何指针值
 * - 典型用途：地址计算、句柄存储、对齐检查
 */
#if OSAL_PLATFORM_BITS == 64
    typedef uint64_t osal_uintptr_t;
    typedef int64_t  osal_intptr_t;
#elif OSAL_PLATFORM_BITS == 32
    typedef uint32_t osal_uintptr_t;
    typedef int32_t  osal_intptr_t;
#else /* 16-bit */
    typedef uint16_t osal_uintptr_t;
    typedef int16_t  osal_intptr_t;
#endif

/*
 * osal_ptrdiff_t: 指针差值类型
 * - 用于表示两个指针之间的距离
 * - 保证能够表示任意两个指针的差值
 */
#if OSAL_PLATFORM_BITS == 64
    typedef int64_t osal_ptrdiff_t;
#elif OSAL_PLATFORM_BITS == 32
    typedef int32_t osal_ptrdiff_t;
#else /* 16-bit */
    typedef int16_t osal_ptrdiff_t;
#endif

/*===========================================================================
 * 文件偏移类型（大文件支持）
 *===========================================================================*/

/*
 * osal_off_t: 文件偏移量类型
 * - 64位平台：使用64位（支持大文件）
 * - 32位平台：使用64位（支持大文件，即使在32位系统上）
 * - 16位平台：使用32位（嵌入式系统通常不需要大文件）
 */
#if OSAL_PLATFORM_BITS >= 32
    typedef int64_t osal_off_t;
#else /* 16-bit */
    typedef int32_t osal_off_t;
#endif

/*===========================================================================
 * 时间类型（跨平台时间表示）
 *===========================================================================*/

/*
 * osal_time_t: 时间戳类型（秒）
 * - 使用64位有符号整数，避免2038年问题
 * - 可表示范围：约 -2920亿年 到 +2920亿年
 */
typedef int64_t osal_time_t;

/*
 * osal_usec_t: 微秒时间类型
 * - 用于高精度时间测量
 * - 64位可表示约584,942年的微秒数
 */
typedef int64_t osal_usec_t;

/*
 * osal_nsec_t: 纳秒时间类型
 * - 用于超高精度时间测量
 * - 64位可表示约584年的纳秒数
 */
typedef int64_t osal_nsec_t;

/*===========================================================================
 * 原子类型（多线程安全）
 *===========================================================================*/

/*
 * 原子类型和操作由 osal/include/ipc/osal_atomic.h 提供
 *
 * 主要类型：
 * - osal_atomic_uint32_t: 原子无符号32位整数
 *
 * 主要接口：
 * - OSAL_AtomicInit()          初始化
 * - OSAL_AtomicLoad()          原子读取
 * - OSAL_AtomicStore()         原子写入
 * - OSAL_AtomicFetchAdd()      原子加法
 * - OSAL_AtomicFetchSub()      原子减法
 * - OSAL_AtomicIncrement()     原子自增
 * - OSAL_AtomicDecrement()     原子自减
 * - OSAL_AtomicCompareExchange() 原子CAS
 *
 * 详见：osal/include/ipc/osal_atomic.h
 */

/*===========================================================================
 * 对齐类型（硬件访问和性能优化）
 *===========================================================================*/

/*
 * 对齐宏
 * - 用于确保数据结构按特定边界对齐
 * - 对于DMA、硬件寄存器访问等场景至关重要
 */
#if defined(__GNUC__) || defined(__clang__)
    #define OSAL_ALIGNED(n)  __attribute__((aligned(n)))
    #define OSAL_PACKED      __attribute__((packed))
#elif defined(_MSC_VER)
    #define OSAL_ALIGNED(n)  __declspec(align(n))
    #define OSAL_PACKED      /* MSVC使用 #pragma pack */
#else
    #define OSAL_ALIGNED(n)  /* 不支持对齐 */
    #define OSAL_PACKED      /* 不支持紧凑 */
#endif

/*
 * 缓存行大小（用于避免伪共享）
 * - x86/ARM: 通常64字节
 * - 某些ARM: 128字节
 */
#ifndef OSAL_CACHE_LINE_SIZE
    #define OSAL_CACHE_LINE_SIZE 64
#endif

/*
 * 对齐辅助宏
 */
#define OSAL_ALIGN_UP(x, align)    (((x) + ((align) - 1)) & ~((align) - 1))
#define OSAL_ALIGN_DOWN(x, align)  ((x) & ~((align) - 1))
#define OSAL_IS_ALIGNED(x, align)  (((osal_uintptr_t)(x) & ((align) - 1)) == 0)

/*===========================================================================
 * 字节序转换宏（支持大小端平台）
 *===========================================================================*/

/*
 * 字节序检测
 * - __BYTE_ORDER__ 是 GCC/Clang 内置宏
 * - 支持 x86_64(小端), ARM32/ARM64(小端), RISC-V(小端/大端可配置)
 */
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define OSAL_LITTLE_ENDIAN 1
        #define OSAL_BIG_ENDIAN    0
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define OSAL_LITTLE_ENDIAN 0
        #define OSAL_BIG_ENDIAN    1
    #else
        #error "Unknown byte order"
    #endif
#else
    /* 如果编译器不提供字节序宏，假设小端（x86/ARM/RISC-V 默认） */
    #warning "Byte order not detected, assuming little-endian"
    #define OSAL_LITTLE_ENDIAN 1
    #define OSAL_BIG_ENDIAN    0
#endif

/*
 * 字节序转换宏
 * - OSAL_HTONS/HTONL: 主机序 -> 网络序（大端）
 * - OSAL_NTOHS/NTOHL: 网络序（大端）-> 主机序
 */
#if OSAL_LITTLE_ENDIAN
    /* 小端平台需要字节交换 */
    #if defined(__GNUC__) || defined(__clang__)
        /* GCC/Clang 内置函数（编译器优化为单条指令） */
        #define OSAL_HTONS(x)  __builtin_bswap16(x)
        #define OSAL_HTONL(x)  __builtin_bswap32(x)
        #define OSAL_HTONLL(x) __builtin_bswap64(x)
        #define OSAL_NTOHS(x)  __builtin_bswap16(x)
        #define OSAL_NTOHL(x)  __builtin_bswap32(x)
        #define OSAL_NTOHLL(x) __builtin_bswap64(x)
    #else
        /* 手动实现字节交换 */
        #define OSAL_HTONS(x)  ((uint16_t)(((x) >> 8) | ((x) << 8)))
        #define OSAL_HTONL(x)  ((uint32_t)(((x) >> 24) | (((x) & 0x00FF0000) >> 8) | \
                                           (((x) & 0x0000FF00) << 8) | ((x) << 24)))
        #define OSAL_HTONLL(x) ((uint64_t)(((uint64_t)OSAL_HTONL((x) & 0xFFFFFFFF) << 32) | \
                                           OSAL_HTONL((x) >> 32)))
        #define OSAL_NTOHS(x)  OSAL_HTONS(x)
        #define OSAL_NTOHL(x)  OSAL_HTONL(x)
        #define OSAL_NTOHLL(x) OSAL_HTONLL(x)
    #endif
#else
    /* 大端平台无需转换 */
    #define OSAL_HTONS(x)  (x)
    #define OSAL_HTONL(x)  (x)
    #define OSAL_HTONLL(x) (x)
    #define OSAL_NTOHS(x)  (x)
    #define OSAL_NTOHL(x)  (x)
    #define OSAL_NTOHLL(x) (x)
#endif

/*===========================================================================
 * 返回值类型
 * 注意：错误码定义已移至 osal_errno.h
 *===========================================================================*/


/*
 * 配置常量
 */
#define OS_MAX_TASKS              64
#define OS_MAX_QUEUES             64
#define OS_MAX_MUTEXES            64
#define OS_MAX_API_NAME           20

/*
 * 超时常量
 */
#define OS_PEND                   0
#define OS_CHECK                  (-1)

/*
 * 任务优先级
 */
#define OS_TASK_PRIORITY_MIN      1
#define OS_TASK_PRIORITY_MAX      255

/*===========================================================================
 * 编译时断言（类型安全检查）
 *===========================================================================*/

/*
 * 编译时断言宏
 * - 用于在编译期检查类型大小和对齐
 * - 如果条件不满足，编译失败
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define OSAL_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
    #define OSAL_STATIC_ASSERT(cond, msg) \
        typedef char osal_static_assert_##msg[(cond) ? 1 : -1]
#endif

/*
 * 类型大小验证（确保跨平台一致性）
 */
OSAL_STATIC_ASSERT(sizeof(int8_t)   == 1, int8_must_be_1_byte);
OSAL_STATIC_ASSERT(sizeof(int16_t)  == 2, int16_must_be_2_bytes);
OSAL_STATIC_ASSERT(sizeof(int32_t)  == 4, int32_must_be_4_bytes);
OSAL_STATIC_ASSERT(sizeof(int64_t)  == 8, int64_must_be_8_bytes);
OSAL_STATIC_ASSERT(sizeof(uint8_t)  == 1, uint8_must_be_1_byte);
OSAL_STATIC_ASSERT(sizeof(uint16_t) == 2, uint16_must_be_2_bytes);
OSAL_STATIC_ASSERT(sizeof(uint32_t) == 4, uint32_must_be_4_bytes);
OSAL_STATIC_ASSERT(sizeof(uint64_t) == 8, uint64_must_be_8_bytes);

/*
 * 指针类型大小验证
 */
#if OSAL_PLATFORM_BITS == 64
    OSAL_STATIC_ASSERT(sizeof(osal_uintptr_t) == 8, uintptr_must_match_pointer_size);
    OSAL_STATIC_ASSERT(sizeof(osal_size_t) == 8, size_must_be_8_bytes_on_64bit);
#elif OSAL_PLATFORM_BITS == 32
    OSAL_STATIC_ASSERT(sizeof(osal_uintptr_t) == 4, uintptr_must_match_pointer_size);
    OSAL_STATIC_ASSERT(sizeof(osal_size_t) == 4, size_must_be_4_bytes_on_32bit);
#elif OSAL_PLATFORM_BITS == 16
    OSAL_STATIC_ASSERT(sizeof(osal_uintptr_t) == 2, uintptr_must_match_pointer_size);
    OSAL_STATIC_ASSERT(sizeof(osal_size_t) == 2, size_must_be_2_bytes_on_16bit);
#endif

/*===========================================================================
 * 类型转换辅助宏（安全的类型转换）
 *===========================================================================*/

/*
 * 指针与整数之间的安全转换
 */
#define OSAL_PTR_TO_UINT(ptr)   ((osal_uintptr_t)(ptr))
#define OSAL_UINT_TO_PTR(val)   ((void *)(osal_uintptr_t)(val))

/*
 * 数组元素个数
 */
#define OSAL_ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]))

/*
 * 结构体成员偏移量
 */
#define OSAL_OFFSETOF(type, member)  ((osal_size_t)&(((type *)0)->member))

/*
 * 通过成员指针获取结构体指针
 */
#define OSAL_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - OSAL_OFFSETOF(type, member)))

/*
 * 最小值/最大值宏（类型安全）
 */
#define OSAL_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define OSAL_MAX(a, b)  (((a) > (b)) ? (a) : (b))

/*
 * 位操作宏
 */
#define OSAL_BIT(n)              (1U << (n))
#define OSAL_BIT_SET(val, bit)   ((val) |= OSAL_BIT(bit))
#define OSAL_BIT_CLR(val, bit)   ((val) &= ~OSAL_BIT(bit))
#define OSAL_BIT_TEST(val, bit)  (((val) & OSAL_BIT(bit)) != 0)
#define OSAL_BIT_TOGGLE(val, bit) ((val) ^= OSAL_BIT(bit))

#endif /* COMMON_TYPES_H */
