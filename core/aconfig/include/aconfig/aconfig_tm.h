/**
 * @file aconfig_tm.h
 * @brief ACONFIG 遥测功能定义 - 兼容性包装（已废弃）
 * @deprecated 此文件已废弃，业务功能枚举已移到产品层
 *             请直接在产品代码中包含 products/ccm/include/ccm/ccm_tm_functions.h
 *
 * @note 核心层不应该包含产品特定头文件
 *       此文件保留仅用于文档说明，新代码应直接使用产品层定义
 */

#ifndef ACONFIG_TM_H
#define ACONFIG_TM_H

#warning "aconfig_tm.h is deprecated. Include product-specific headers (e.g., ccm_tm_functions.h) directly in your source files."

/* 此头文件不再包含任何产品特定定义
 * 请在您的源文件（.c）中直接包含产品层头文件：
 *
 * 示例：
 *   #include "osal.h"
 *   #include "aconfig.h"
 *   #include "ccm_tm_functions.h"  // 产品特定
 */

#endif /* ACONFIG_TM_H */
