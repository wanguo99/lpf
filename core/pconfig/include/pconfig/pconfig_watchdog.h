/************************************************************************
 * PCONFIG Watchdog 配置类型定义
 *
 * 功能：
 * - Watchdog 外设配置（支持多种通信接口）
 ************************************************************************/

#ifndef PCONFIG_WATCHDOG_H
#define PCONFIG_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Watchdog 接口类型
 *===========================================================================*/

/**
 * @brief Watchdog 通信接口类型
 */
typedef enum {
	PCONFIG_WATCHDOG_INTERFACE_GPIO = 0,     /* GPIO 引脚 */
	PCONFIG_WATCHDOG_INTERFACE_JTAG = 1,     /* JTAG 调试接口 */
	PCONFIG_WATCHDOG_INTERFACE_I2C = 2,      /* I2C 总线 */
	PCONFIG_WATCHDOG_INTERFACE_SPI = 3,      /* SPI 总线 */
	PCONFIG_WATCHDOG_INTERFACE_DEVFILE = 4,  /* 设备文件（如 /dev/watchdog）*/
} pconfig_watchdog_interface_t;

/*===========================================================================
 * Watchdog 接口配置（union 方式）
 *===========================================================================*/

/**
 * @brief Watchdog GPIO 接口配置
 */
typedef struct {
	uint32_t gpio_pin;           /* GPIO 引脚号 */
	uint32_t toggle_interval_ms; /* 翻转间隔 */
} pconfig_watchdog_gpio_config_t;

/**
 * @brief Watchdog JTAG 接口配置
 */
typedef struct {
	const char *jtag_device;     /* JTAG 设备路径 */
	uint32_t timeout_ms;         /* 超时时间 */
} pconfig_watchdog_jtag_config_t;

/**
 * @brief Watchdog I2C 接口配置
 */
typedef struct {
	const char *device;          /* I2C 设备名（如 "i2c-0"） */
	uint8_t slave_address;       /* 从设备地址 */
	uint32_t timeout_ms;         /* 超时时间 */
} pconfig_watchdog_i2c_config_t;

/**
 * @brief Watchdog SPI 接口配置
 */
typedef struct {
	const char *device;          /* SPI 设备名（如 "spi0.0"） */
	uint32_t speed_hz;           /* SPI 速度 */
	uint8_t mode;                /* SPI 模式 */
	uint32_t timeout_ms;         /* 超时时间 */
} pconfig_watchdog_spi_config_t;

/**
 * @brief Watchdog 设备文件接口配置
 */
typedef struct {
	const char *device_path;     /* 设备文件路径（如 "/dev/watchdog"） */
	uint32_t timeout_sec;        /* 超时时间（秒） */
} pconfig_watchdog_devfile_config_t;

/*===========================================================================
 * Watchdog 完整配置
 *===========================================================================*/

/**
 * @brief Watchdog 外设配置
 */
typedef struct {
	pconfig_watchdog_interface_t interface;  /* 接口类型 */

	/* 接口配置（union）*/
	union {
		pconfig_watchdog_gpio_config_t gpio;
		pconfig_watchdog_jtag_config_t jtag;
		pconfig_watchdog_i2c_config_t i2c;
		pconfig_watchdog_spi_config_t spi;
		pconfig_watchdog_devfile_config_t devfile;
	} hw;

	/* 业务配置 */
	uint32_t feed_interval_ms;   /* 喂狗间隔 */
	bool auto_feed;              /* 是否自动喂狗 */
} pconfig_watchdog_config_t;

/**
 * @brief Watchdog 配置条目（带启用标志）
 */
typedef struct {
	bool enabled;                        /* 是否启用 */
	const char *description;             /* 描述信息 */
	pconfig_watchdog_config_t config;    /* 配置数据 */
} pconfig_watchdog_entry_t;

#endif /* PCONFIG_WATCHDOG_H */
