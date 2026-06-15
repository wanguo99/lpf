/**
 * @file test_framework.h
 * @brief 统一测试框架头文件
 *
 * 这是测试用例唯一需要包含的头文件，整合了所有测试框架功能。
 * 使用方式：
 *   #include "test_framework.h"
 *   #include <osal.h>  // 或其他被测试模块的头文件
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* 包含所有测试框架组件 */
#include "test_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "test_parameterized.h"
#include "test_mock.h"
#include "test_metadata.h"

#endif /* TEST_FRAMEWORK_H */
