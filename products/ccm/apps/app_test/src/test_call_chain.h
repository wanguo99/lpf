/**
 * @file test_call_chain.h
 * @brief 完整调用链测试用例头文件
 */

#ifndef TEST_CALL_CHAIN_H
#define TEST_CALL_CHAIN_H

#include <stdint.h>

/* 测试功能 ID 定义 */
#define TEST_FUNCTION_MCU_CAN       0x9001  /* 测试 MCU CAN 接口 */
#define TEST_FUNCTION_MCU_SERIAL    0x9002  /* 测试 MCU Serial 接口 */
#define TEST_FUNCTION_BMC           0x9003  /* 测试 BMC */

/**
 * @brief 测试 MCU CAN 调用链
 * @return 0 成功，负值失败
 */
int32_t APP_TestCallChain_MCU_CAN(void);

/**
 * @brief 测试 MCU Serial 调用链
 * @return 0 成功，负值失败
 */
int32_t APP_TestCallChain_MCU_Serial(void);

/**
 * @brief 测试 BMC 调用链
 * @return 0 成功，负值失败
 */
int32_t APP_TestCallChain_BMC(void);

/**
 * @brief 运行所有测试
 * @return 0 全部通过，负值有失败
 */
int32_t APP_RunAllTests(void);

#endif /* TEST_CALL_CHAIN_H */
