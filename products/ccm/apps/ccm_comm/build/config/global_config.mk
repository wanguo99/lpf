
#
# Toolchain configuration
#
CONFIG_TOOLCHAIN_PATH=""
CONFIG_TOOLCHAIN_PREFIX=""
# end of Toolchain configuration

#
# SDK Components Configuration
#
CONFIG_ACL=y

#
# ACL Build Configuration
#
# CONFIG_ACL_BUILD_STATIC is not set
CONFIG_ACL_BUILD_SHARED=y
# end of ACL Build Configuration

#
# ACL Configuration
#
CONFIG_ACL_MAX_DEVICES=64
# end of ACL Configuration

CONFIG_HAL=y

#
# Platform Configuration
#
CONFIG_PLATFORM_GENERIC_LINUX=y
# end of Platform Configuration

#
# Build Configuration
#
CONFIG_HAL_BUILD_STATIC=y
CONFIG_HAL_BUILD_SHARED=y
# end of Build Configuration

#
# Driver Configuration
#
CONFIG_HAL_CAN=y
CONFIG_HAL_UART=y
CONFIG_HAL_I2C=y
CONFIG_HAL_SPI=y
CONFIG_HAL_GPIO=y
CONFIG_HAL_WATCHDOG=y
# end of Driver Configuration

#
# Advanced Features
#
# CONFIG_HAL_DEBUG is not set
# CONFIG_HAL_STATISTICS is not set
# CONFIG_HAL_POWER_MANAGEMENT is not set
# end of Advanced Features

CONFIG_OSAL=y

#
# Platform Configuration
#
CONFIG_OS_LINUX=y
# CONFIG_OS_WINDOWS is not set
# CONFIG_OS_RTOS is not set
# CONFIG_OS_MACOS is not set
# CONFIG_OS_BARE is not set
CONFIG_OS="linux"
# CONFIG_ARCH_X86_64 is not set
# CONFIG_ARCH_ARM32 is not set
CONFIG_ARCH_ARM64=y
# CONFIG_ARCH_RISCV64 is not set
CONFIG_ARCH="arm64"
CONFIG_OSAL_OS_POSIX=y
CONFIG_OSAL_ARCH_64BIT=y
# end of Platform Configuration

#
# Build Configuration
#
CONFIG_OSAL_BUILD_STATIC=y
CONFIG_OSAL_BUILD_SHARED=y
# end of Build Configuration

#
# Feature Configuration
#
CONFIG_OSAL_IPC=y
CONFIG_OSAL_FILE=y
CONFIG_OSAL_THREAD=y
CONFIG_OSAL_NETWORK=y
CONFIG_OSAL_SIGNAL=y
# end of Feature Configuration

#
# Resource Limits
#
CONFIG_OSAL_MAX_TASKS=256
CONFIG_OSAL_MAX_QUEUES=128
CONFIG_OSAL_MAX_MUTEXES=128
# end of Resource Limits

#
# Debug Options
#
# CONFIG_OSAL_DEBUG_LOGGING is not set
# end of Debug Options

CONFIG_PCL=y
CONFIG_PCL_PLATFORM_TI_AM625=y
# CONFIG_PCL_PLATFORM_VENDOR_DEMO is not set
CONFIG_PCL_PRODUCT_H200_100P=y
# CONFIG_PCL_PRODUCT_H200_200P is not set

#
# PCL Build Configuration
#
# CONFIG_PCL_BUILD_STATIC is not set
CONFIG_PCL_BUILD_SHARED=y
# end of PCL Build Configuration

CONFIG_PDL=y

#
# PDL Build Configuration
#
# CONFIG_PDL_BUILD_STATIC is not set
CONFIG_PDL_BUILD_SHARED=y
# end of PDL Build Configuration

CONFIG_PDL_SATELLITE_SUPPORT=y
CONFIG_PDL_BMC_SUPPORT=y
CONFIG_PDL_MCU_SUPPORT=y
CONFIG_PDL_WATCHDOG_SUPPORT=y
CONFIG_H200_AM625=y
# CONFIG_H200_AM625_BUILD_SHARED is not set
CONFIG_LIBCCM=y
# CONFIG_LIBCCM_BUILD_SHARED is not set
# end of SDK Components Configuration

#
# Project Components Configuration
#
# end of Project Components Configuration
