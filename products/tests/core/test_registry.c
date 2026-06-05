/**
 * @file test_registry.c
 * @brief Test suite registry implementation
 *
 * Maintains a global registry of all test suites registered via
 * constructor attributes. Provides lookup and enumeration functions.
 */

#include "test_core.h"
#include "osal.h"

#define MAX_SUITES 128

/* Global registry */
static const test_suite_t *g_registered_suites[MAX_SUITES];
static uint32_t g_suite_count = 0;

/**
 * Register a test suite
 * Called automatically by constructor attributes
 */
void libutest_register_suite(const test_suite_t *suite)
{
    if (NULL == suite) {
        return;
    }

    if (g_suite_count < MAX_SUITES) {
        g_registered_suites[g_suite_count++] = suite;
    } else {
        OSAL_Printf("ERROR: Maximum number of test suites (%d) exceeded\n", MAX_SUITES);
    }
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
 * Find suite by name
 */
const test_suite_t* test_find_suite(const char *name)
{
    if (NULL == name) {
        return NULL;
    }

    uint32_t i;


    for (i = 0; i < g_suite_count; i++) {
        if (0 == OSAL_Strcmp(g_registered_suites[i]->suite_name, name)) {
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
        if (0 == OSAL_Strcmp(g_registered_suites[i]->layer_name, layer_name)) {
            suites[count++] = g_registered_suites[i];
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
        if (0 == OSAL_Strcmp(g_registered_suites[i]->module_name, module_name)) {
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
            if (0 == OSAL_Strcmp(layers[j], layer)) {
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
            if (0 == OSAL_Strcmp(modules[j], module)) {
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
