/**
 * @file test_metadata.c
 * @brief Test metadata utility functions
 */

#include "test_metadata.h"
#include "osal.h"

/**
 * Get tag names as a formatted string (comma-separated)
 * @param tags Tag bitmask
 * @param buf Output buffer (if NULL, uses static buffer - not thread-safe)
 * @param buf_size Buffer size
 * @return Pointer to formatted string
 */
const char* test_tags_to_string(uint32_t tags, char *buf, uint32_t buf_size)
{
    static char static_buf[256];
    char *output = buf ? buf : static_buf;
    uint32_t size = buf ? buf_size : sizeof(static_buf);
    uint32_t pos = 0;
    bool first = true;

    if (size == 0) {
        return "";
    }

    if (tags == TEST_TAG_NONE) {
        OSAL_strncpy(output, "none", size);
        output[size - 1] = '\0';
        return output;
    }

    output[0] = '\0';

#define APPEND_TAG(flag, name) \
    if ((tags & (flag)) && pos < size - 1) { \
        if (!first && pos < size - 2) { \
            output[pos++] = ','; \
            output[pos++] = ' '; \
        } \
        uint32_t len = OSAL_strlen(name); \
        if (pos + len < size - 1) { \
            OSAL_strncpy(output + pos, name, size - pos); \
            pos += len; \
            first = false; \
        } \
    }

    APPEND_TAG(TEST_TAG_FAST, "fast");
    APPEND_TAG(TEST_TAG_SLOW, "slow");
    APPEND_TAG(TEST_TAG_HARDWARE, "hardware");
    APPEND_TAG(TEST_TAG_NETWORK, "network");
    APPEND_TAG(TEST_TAG_FILESYSTEM, "filesystem");
    APPEND_TAG(TEST_TAG_MEMORY_INTENSIVE, "memory");
    APPEND_TAG(TEST_TAG_CPU_INTENSIVE, "cpu");
    APPEND_TAG(TEST_TAG_PRIVILEGED, "privileged");
    APPEND_TAG(TEST_TAG_UNSTABLE, "unstable");
    APPEND_TAG(TEST_TAG_DEPRECATED, "deprecated");

#undef APPEND_TAG

    output[pos] = '\0';
    return output;
}
