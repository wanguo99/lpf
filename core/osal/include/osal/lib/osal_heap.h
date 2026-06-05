/************************************************************************
 * OSAL Public API - Memory Allocation and Heap Management
 *
 * This file provides the public API for dynamic memory allocation,
 * heap statistics, and memory usage monitoring.
 ************************************************************************/

#ifndef OSAL_HEAP_H
#define OSAL_HEAP_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * Memory Allocation
 ************************************************************************/

/**
 * @brief Allocate memory from the heap
 *
 * Allocates a block of memory of the specified size. The memory is
 * not initialized and may contain garbage values.
 *
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 *
 * @note The caller must free the memory using OSAL_Free()
 * @note Thread-safe
 */
void *OSAL_Malloc(uint32_t size);

/**
 * @brief Free previously allocated memory
 *
 * Frees a memory block previously allocated by OSAL_Malloc().
 *
 * @param ptr Pointer to memory block to free (NULL is safe)
 *
 * @note Passing NULL is safe and does nothing
 * @note Thread-safe
 * @warning Passing an invalid pointer causes undefined behavior
 */
void OSAL_Free(void *ptr);

/**
 * @brief Reallocate memory block
 *
 * Changes the size of the memory block pointed to by ptr. The contents
 * will be unchanged in the range from the start of the region up to the
 * minimum of the old and new sizes.
 *
 * @param ptr Pointer to previously allocated memory (NULL allowed)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 *
 * @note If ptr is NULL, behaves like OSAL_Malloc()
 * @note If new_size is 0 and ptr is not NULL, behaves like OSAL_Free()
 * @note The returned pointer may differ from ptr
 * @note Thread-safe
 */
void *OSAL_Realloc(void *ptr, uint32_t new_size);

/************************************************************************
 * Heap Monitoring
 ************************************************************************/

/**
 * @brief Get heap memory information
 *
 * Returns current and total heap memory usage. On Linux, reads from
 * /proc/self/status. On other platforms, uses internal tracking.
 *
 * @param[out] free_bytes Available memory in bytes
 * @param[out] total_bytes Total allocated memory in bytes
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Thread-safe
 */
int32_t OSAL_HeapGetInfo(uint32_t *free_bytes, uint32_t *total_bytes);

/**
 * @brief Set heap usage threshold
 *
 * Sets the memory usage threshold as a percentage. When usage exceeds
 * this threshold, OSAL_HeapCheckThreshold() will report it.
 *
 * @param percent Threshold percentage (0-100)
 * @return OSAL_SUCCESS on success, OSAL_ERR_INVALID_SIZE if percent > 100
 *
 * @note Default threshold is OSAL_HEAP_THRESHOLD_DEFAULT
 * @note Thread-safe
 */
int32_t OSAL_HeapSetThreshold(uint32_t percent);

/**
 * @brief Check if heap usage exceeds threshold
 *
 * Checks current memory usage against the configured threshold.
 * Logs a warning on the first threshold exceedance.
 *
 * @param[out] exceeded true if threshold exceeded, false otherwise
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Thread-safe
 * @note Only logs warning once until usage drops below threshold
 */
int32_t OSAL_HeapCheckThreshold(bool *exceeded);

/**
 * @brief Get heap usage statistics
 *
 * Returns current and peak heap memory usage.
 *
 * @param[out] current Current memory usage in bytes
 * @param[out] peak Peak memory usage in bytes
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Thread-safe
 */
int32_t OSAL_HeapGetStats(uint32_t *current, uint32_t *peak);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_HEAP_H */
