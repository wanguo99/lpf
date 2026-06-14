/************************************************************************
 * OSAL Time API
 *
 * Time management, sleep functions, and high-resolution timers
 ************************************************************************/

#ifndef OSAL_TIME_H
#define OSAL_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 时间转换常量
 *===========================================================================*/

/** 毫秒到秒的转换因子 */
#define OSAL_MSEC_PER_SEC       0x3E8U

/** 微秒到秒的转换因子 */
#define OSAL_USEC_PER_SEC       0xF4240U

/** 纳秒到秒的转换因子 */
#define OSAL_NSEC_PER_SEC       0x3B9ACA00U

/** 微秒到毫秒的转换因子 */
#define OSAL_USEC_PER_MSEC      0x3E8U

/** 纳秒到毫秒的转换因子 */
#define OSAL_NSEC_PER_MSEC      0xF4240U

/** 纳秒到微秒的转换因子 */
#define OSAL_NSEC_PER_USEC      0x3E8U

/*===========================================================================
 * 时间延迟 API
 *===========================================================================*/

/**
 * @brief 毫秒级延迟（推荐使用）
 * @param msec 延迟时间（毫秒）
 * @return 0成功，-1失败
 * @note 这是最常用的延迟接口，精度为毫秒
 */
int32_t OSAL_msleep(uint32_t msec);

/**
 * @brief 微秒级延迟
 * @param usec 延迟时间（微秒）
 * @return 0成功，-1失败
 * @note 精度为微秒，适用于需要高精度延迟的场景
 */
int32_t OSAL_usleep(uint32_t usec);

/**
 * @brief 秒级延迟
 * @param sec 延迟时间（秒）
 * @return 0成功，-1失败
 * @note 精度为秒，适用于长时间延迟
 */
int32_t OSAL_sleep(uint32_t sec);

/**
 * @brief 纳秒级延迟
 * @param nsec 延迟时间（纳秒）
 * @return 0成功，-1失败
 * @note 精度为纳秒，适用于极高精度延迟场景
 */
int32_t OSAL_nanosleep(uint64_t nsec);

/*===========================================================================
 * 时间获取 API
 *===========================================================================*/

/**
 * @brief 获取单调时钟时间（不受系统时间调整影响）
 * @return 时间戳（微秒），从系统启动开始计时
 * @note 用于超时计算、性能测量等场景，不受NTP同步影响
 *       封装clock_gettime(CLOCK_MONOTONIC)
 */
int64_t OSAL_get_monotonic_time(void);

/**
 * @brief 获取启动时钟时间（包含休眠时间）
 * @return 时间戳（微秒），从系统启动开始计时（包含休眠）
 * @note 与CLOCK_MONOTONIC类似，但包含系统休眠时间
 *       封装clock_gettime(CLOCK_BOOTTIME)
 */
int64_t OSAL_get_boot_time(void);

/**
 * @brief 获取高精度时间戳（纳秒）
 * @return 时间戳（纳秒），用于性能测量
 * @note 用于微秒级性能测量，封装clock_gettime(CLOCK_MONOTONIC)
 */
int64_t OSAL_get_highres_time(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_TIME_H */
