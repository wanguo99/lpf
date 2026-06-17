/**
 * @file pmc_runtime.h
 * @brief PMC product runtime initialization entry.
 */

#ifndef PMC_RUNTIME_H
#define PMC_RUNTIME_H

#include <stdint.h>

/**
 * @brief Initialize PMC product runtime configuration.
 *
 * This initializes and registers product-owned PCONFIG and ACONFIG data so
 * applications do not need to know board-specific symbols.
 *
 * @return OSAL_SUCCESS on success, OSAL_ERR_* on failure.
 */
int32_t PMC_Runtime_Init(void);

/**
 * @brief Cleanup PMC product runtime configuration.
 */
void PMC_Runtime_Cleanup(void);

#endif /* PMC_RUNTIME_H */
