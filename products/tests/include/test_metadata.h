/**
 * @file test_metadata.h
 * @brief Test metadata definitions for categorization and filtering
 *
 * Provides structures and enums for test categorization, tagging, and filtering.
 * Supports runtime filtering by category, tags, timeout, and other attributes.
 */

#ifndef TEST_METADATA_H
#define TEST_METADATA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Test category enumeration
 * Defines the primary classification of test suites
 */
typedef enum {
	TEST_CATEGORY_UNIT = 0,      /**< Unit tests - isolated module testing */
	TEST_CATEGORY_PERFORMANCE,   /**< Performance tests - timing, throughput, latency */
	TEST_CATEGORY_STRESS,        /**< Stress tests - load, concurrency, resource limits */
	TEST_CATEGORY_SYSTEM,        /**< System tests - integration, end-to-end scenarios */
	TEST_CATEGORY_MAX
} test_category_t;

/**
 * Test tag bit flags
 * Multiple tags can be combined using bitwise OR
 */
typedef enum {
	TEST_TAG_NONE = 0,           /**< No special tags */
	TEST_TAG_FAST = (1 << 0),    /**< Fast test (<100ms) */
	TEST_TAG_SLOW = (1 << 1),    /**< Slow test (>1s) */
	TEST_TAG_HARDWARE = (1 << 2), /**< Requires real hardware */
	TEST_TAG_NETWORK = (1 << 3),  /**< Requires network connectivity */
	TEST_TAG_FILESYSTEM = (1 << 4), /**< Requires filesystem access */
	TEST_TAG_MEMORY_INTENSIVE = (1 << 5), /**< High memory usage */
	TEST_TAG_CPU_INTENSIVE = (1 << 6),    /**< High CPU usage */
	TEST_TAG_PRIVILEGED = (1 << 7),       /**< Requires elevated privileges */
	TEST_TAG_UNSTABLE = (1 << 8),         /**< Known to be flaky/unstable */
	TEST_TAG_DEPRECATED = (1 << 9)        /**< Deprecated, to be removed */
} test_tag_t;

/**
 * Test metadata structure
 * Extends test suite with categorization and filtering attributes
 */
typedef struct {
	test_category_t category;    /**< Test category */
	uint32_t tags;               /**< Tag bitmask (combination of test_tag_t) */
	uint32_t timeout_ms;         /**< Expected maximum execution time (0 = no limit) */
	const char *description;     /**< Human-readable description */
} test_metadata_t;

/**
 * Test filter structure
 * Used to filter test suites at runtime
 */
typedef struct {
	uint32_t category_mask;      /**< Category bitmask (1 << category) */
	uint32_t include_tags;       /**< Tags that must be present (bitwise AND) */
	uint32_t exclude_tags;       /**< Tags that must not be present (bitwise AND) */
	uint32_t max_timeout_ms;     /**< Maximum allowed timeout (0 = no limit) */
	bool enabled;                /**< Filter is active */
} test_filter_t;

/**
 * Helper macros for category mask operations
 */
#define TEST_CATEGORY_MASK(cat)  (1u << (cat))
#define TEST_CATEGORY_ALL        ((1u << TEST_CATEGORY_MAX) - 1)

/**
 * Helper macro to create metadata initializer
 * Usage: TEST_METADATA(TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, "Quick unit test")
 */
#define TEST_METADATA(cat, tags, timeout, desc) \
	{ .category = (cat), .tags = (tags), .timeout_ms = (timeout), .description = (desc) }

/**
 * Default metadata (no special attributes)
 */
#define TEST_METADATA_DEFAULT \
	{ .category = TEST_CATEGORY_UNIT, .tags = TEST_TAG_NONE, .timeout_ms = 0, .description = "" }

/**
 * Check if metadata matches a filter
 * @param metadata Test metadata to check
 * @param filter Filter criteria
 * @return true if test passes filter, false otherwise
 */
static inline bool test_metadata_matches_filter(const test_metadata_t *metadata, const test_filter_t *filter)
{
	if (!filter || !filter->enabled) {
		return true;  /* No filter or disabled filter = pass all */
	}

	/* Check category */
	if (filter->category_mask != 0) {
		uint32_t cat_bit = (1u << metadata->category);
		if ((filter->category_mask & cat_bit) == 0) {
			return false;
		}
	}

	/* Check include tags (all must be present) */
	if (filter->include_tags != 0) {
		if ((metadata->tags & filter->include_tags) != filter->include_tags) {
			return false;
		}
	}

	/* Check exclude tags (none must be present) */
	if (filter->exclude_tags != 0) {
		if ((metadata->tags & filter->exclude_tags) != 0) {
			return false;
		}
	}

	/* Check timeout */
	if (filter->max_timeout_ms > 0 && metadata->timeout_ms > 0) {
		if (metadata->timeout_ms > filter->max_timeout_ms) {
			return false;
		}
	}

	return true;
}

/**
 * Get category name as string
 * @param category Category enum value
 * @return Static string representing category name
 */
static inline const char* test_category_name(test_category_t category)
{
	switch (category) {
	case TEST_CATEGORY_UNIT:
		return "unit";
	case TEST_CATEGORY_PERFORMANCE:
		return "performance";
	case TEST_CATEGORY_STRESS:
		return "stress";
	case TEST_CATEGORY_SYSTEM:
		return "system";
	default:
		return "unknown";
	}
}

/**
 * Get tag names as a formatted string (comma-separated)
 * @param tags Tag bitmask
 * @param buf Output buffer
 * @param buf_size Buffer size
 * @return Number of characters written (excluding null terminator)
 *
 * Note: Uses static buffer internally, not thread-safe if buf is NULL
 */
const char* test_tags_to_string(uint32_t tags, char *buf, uint32_t buf_size);

#endif /* TEST_METADATA_H */
