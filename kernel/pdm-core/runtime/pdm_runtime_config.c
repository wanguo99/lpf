// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_runtime_config.c
 * @brief Runtime 配置驱动系统（已禁用）
 *
 * 注意：此文件已禁用，因为：
 * 1. pdm_configs 模块已删除
 * 2. 设备现在由 Device Tree + pdm_bus 创建和管理
 * 3. Runtime config driver 系统已被新总线架构替代
 *
 * 如果需要恢复设备探测功能，应该：
 * - 使用新总线的 pdm_driver_register() API
 * - 通过 Device Tree 定义设备节点
 * - 让内核自动匹配和 probe
 */

#include "pdm/runtime/pdm_runtime.h"
#include "pdm/core/pdm_core.h"
#include "pdm_runtime_internal.h"

/*
 * 已禁用的旧实现：
 * - pdm_runtime_config_driver_first/last
 * - pdm_runtime_config_find_driver
 * - pdm_runtime_probe_devices
 *
 * 这些函数依赖已删除的 pdm_configs 模块。
 * 在新架构中，设备由 pdm_bus_controller 从 Device Tree 创建。
 */
