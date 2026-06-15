/**
 * @file test_registry.c
 * @brief Test suite registry implementation
 *
 * Maintains a global registry of all test suites registered via
 * constructor attributes. Provides lookup, enumeration, and filtering functions.
 * Supports dynamic allocation to avoid hardcoded limits.
 */

#include "test_core.h"
#include "osal.h"

#define INITIAL_CAPACITY 32
#define GROWTH_FACTOR 2

/* Global registry with dynamic allocation */
static const test_suite_t **g_registered_suites = NULL;
static uint32_t g_suite_count = 0;
static uint32_t g_suite_capacity = 0;

/**
 * Initialize registry with initial capacity
 */
static void init_registry(void)
{
    if (g_registered_suites == NULL) {
        g_suite_capacity = INITIAL_CAPACITY;
        g_registered_suites = (const test_suite_t **)OSAL_malloc(
            g_suite_capacity * OSAL_sizeof(test_suite_t *));
        if (g_registered_suites == NULL) {
            OSAL_printf("FATAL: Failed to allocate test suite registry\n");
            g_suite_capacity = 0;
        }
    }
}

/**
 * Expand registry capacity
 */
static bool expand_registry(void)
{
    uint32_t new_capacity = g_suite_capacity * GROWTH_FACTOR;
    const test_suite_t **new_suites = (const test_suite_t **)OSAL_malloc(
        new_capacity * OSAL_sizeof(test_suite_t *));

    if (new_suites == NULL) {
        OSAL_printf("ERROR: Failed to expand test suite registry\n");
        return false;
    }

    /* Copy old data to new buffer */
    OSAL_memcpy(new_suites, g_registered_suites,
                g_suite_count * OSAL_sizeof(test_suite_t *));

    /* Free old buffer and update registry */
    OSAL_free(g_registered_suites);
    g_registered_suites = new_suites;
    g_suite_capacity = new_capacity;
    return true;
}

/**
 * Register a test suite
 * Called automatically by constructor attributes
 */
void libutest_register_suite(const test_suite_t *suite)
{
    if (NULL == suite) {
        return;
    }

    init_registry();

    if (g_suite_count >= g_suite_capacity) {
        if (!expand_registry()) {
            OSAL_printf("ERROR: Cannot register test suite '%s' - registry full\n",
                        suite->suite_name);
            return;
        }
    }

    g_registered_suites[g_suite_count++] = suite;
}

/**
 * Get all registered suites
 */
const test_suite_t** test_get_all_suites(uint32_t *count)
{
    if (NULL != count) {
        *count = g_suite_count;
    }
    return g_registered_suites;
}

/**
 * Get all suites matching filter
 */
uint32_t test_get_filtered_suites(const test_filter_t *filter,
                                   const test_suite_t **suites,
                                   uint32_t max_suites)
{
    if (NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (test_metadata_matches_filter(&g_registered_suites[i]->metadata, filter)) {
            suites[count++] = g_registered_suites[i];
        }
    }

    return count;
}

/**
 * Find suite by name
 */
const test_suite_t* test_find_suite(const char *name)
{
    if (NULL == name) {
        return NULL;
    }

    uint32_t i;


    for (i = 0; i < g_suite_count; i++) {
        if (0 == OSAL_strcmp(g_registered_suites[i]->suite_name, name)) {
            return g_registered_suites[i];
        }
    }
    return NULL;
}

/**
 * Get suites by layer
 */
uint32_t test_get_suites_by_layer(const char *layer_name, const test_suite_t **suites, uint32_t max_suites)
{
    if (NULL == layer_name || NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (0 == OSAL_strcmp(g_registered_suites[i]->layer_name, layer_name)) {
            suites[count++] = g_registered_suites[i];
        }
    }
    return count;
}

/**
 * Get suites by layer with filter
 */
uint32_t test_get_suites_by_layer_filtered(const char *layer_name,
                                            const test_filter_t *filter,
                                            const test_suite_t **suites,
                                            uint32_t max_suites)
{
    if (NULL == layer_name || NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (0 == OSAL_strcmp(g_registered_suites[i]->layer_name, layer_name)) {
            if (test_metadata_matches_filter(&g_registered_suites[i]->metadata, filter)) {
                suites[count++] = g_registered_suites[i];
            }
        }
    }
    return count;
}

/**
 * Get suites by module
 */
uint32_t test_get_suites_by_module(const char *module_name, const test_suite_t **suites, uint32_t max_suites)
{
    if (NULL == module_name || NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (0 == OSAL_strcmp(g_registered_suites[i]->module_name, module_name)) {
            suites[count++] = g_registered_suites[i];
        }
    }
    return count;
}

/**
 * Get suites by module with filter
 */
uint32_t test_get_suites_by_module_filtered(const char *module_name,
                                             const test_filter_t *filter,
                                             const test_suite_t **suites,
                                             uint32_t max_suites)
{
    if (NULL == module_name || NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (0 == OSAL_strcmp(g_registered_suites[i]->module_name, module_name)) {
            if (test_metadata_matches_filter(&g_registered_suites[i]->metadata, filter)) {
                suites[count++] = g_registered_suites[i];
            }
        }
    }
    return count;
}

/**
 * Get suites by category
 */
uint32_t test_get_suites_by_category(test_category_t category,
                                      const test_suite_t **suites,
                                      uint32_t max_suites)
{
    if (NULL == suites) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if (g_registered_suites[i]->metadata.category == category) {
            suites[count++] = g_registered_suites[i];
        }
    }
    return count;
}

/**
 * Get suites by tag (any of the specified tags must match)
 */
uint32_t test_get_suites_by_tags(uint32_t tags,
                                  const test_suite_t **suites,
                                  uint32_t max_suites)
{
    if (NULL == suites || tags == 0) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_suites; i++) {
        if ((g_registered_suites[i]->metadata.tags & tags) != 0) {
            suites[count++] = g_registered_suites[i];
        }
    }
    return count;
}

/**
 * Get unique layer names
 */
uint32_t test_get_layers(const char **layers, uint32_t max_layers)
{
    if (NULL == layers) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_layers; i++) {
        const char *layer = g_registered_suites[i]->layer_name;

        /* Check if already in list */
        bool found = false;
        uint32_t j;

        for (j = 0; j < count; j++) {
            if (0 == OSAL_strcmp(layers[j], layer)) {
                found = true;
                break;
            }
        }

        if (!found) {
            layers[count++] = layer;
        }
    }
    return count;
}

/**
 * Get unique module names
 */
uint32_t test_get_modules(const char **modules, uint32_t max_modules)
{
    if (NULL == modules) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t i;

    for (i = 0; i < g_suite_count && count < max_modules; i++) {
        const char *module = g_registered_suites[i]->module_name;

        /* Check if already in list */
        bool found = false;
        uint32_t j;

        for (j = 0; j < count; j++) {
            if (0 == OSAL_strcmp(modules[j], module)) {
                found = true;
                break;
            }
        }

        if (!found) {
            modules[count++] = module;
        }
    }
    return count;
}
