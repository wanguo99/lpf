#!/bin/bash
#
# Generate include/generated/gen_version.h with build information
# Similar to Linux kernel version information
#

set -e

OUTPUT_FILE="include/generated/gen_version.h"
TEMP_FILE="${OUTPUT_FILE}.tmp"

# Ensure include/generated directory exists
mkdir -p include/generated

# Extract version from .config or use default
if [ -f .config ]; then
    VERSION=$(grep '^CONFIG_PDM_VERSION=' .config | cut -d'"' -f2)
else
    VERSION="unknown"
fi

# Collect build information
BUILD_USER="${USER:-unknown}"
BUILD_HOST="${HOSTNAME:-$(hostname 2>/dev/null || echo unknown)}"
BUILD_TIME=$(date -u '+%Y-%m-%d %H:%M:%S UTC')
BUILD_TIMESTAMP=$(date -u '+%s')

# Get GCC version
if command -v gcc >/dev/null 2>&1; then
    GCC_VERSION=$(gcc --version | head -1)
else
    GCC_VERSION="gcc version unknown"
fi

# Get architecture
ARCH=$(uname -m)

# Get kernel info
KERNEL_VERSION=$(uname -r)

# Get git commit if available
if [ -d .git ] && command -v git >/dev/null 2>&1; then
    GIT_COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
    GIT_DIRTY=$(git diff --quiet 2>/dev/null || echo "-dirty")
else
    GIT_COMMIT="unknown"
    GIT_DIRTY=""
fi

# Generate version.h
cat > "$TEMP_FILE" << EOF
/*
 * Automatically generated file - do not edit
 * PDM version: ${VERSION}
 * Generated on: ${BUILD_TIME}
 */

#ifndef __PDM_VERSION_H__
#define __PDM_VERSION_H__

/*
 * Version Information
 */
#define PDM_VERSION           "${VERSION}"
#define PDM_VERSION_CODE      ${BUILD_TIMESTAMP}

/*
 * Build Information
 */
#define PDM_COMPILE_BY        "${BUILD_USER}"
#define PDM_COMPILE_HOST      "${BUILD_HOST}"
#define PDM_COMPILER          "${GCC_VERSION}"

/*
 * Timestamps
 */
#define PDM_COMPILE_TIME      "${BUILD_TIME}"
#define PDM_COMPILE_TIMESTAMP ${BUILD_TIMESTAMP}

/*
 * Platform Information
 */
#define PDM_BUILD_ARCH        "${ARCH}"
#define PDM_BUILD_KERNEL      "${KERNEL_VERSION}"

/*
 * Git Information
 */
#define PDM_GIT_COMMIT        "${GIT_COMMIT}${GIT_DIRTY}"

/*
 * Version Banner (similar to Linux kernel banner)
 * Format: PDM version <version> (<user>@<host>) (<compiler>) <timestamp>
 */
#define PDM_BANNER \
    "PDM version " PDM_VERSION \
    " (" PDM_COMPILE_BY "@" PDM_COMPILE_HOST ")" \
    " (" PDM_COMPILER ")" \
    " " PDM_COMPILE_TIME

/*
 * Short version string (version + git commit)
 */
#define PDM_VERSION_STRING \
    PDM_VERSION "-" PDM_GIT_COMMIT

#endif /* __PDM_VERSION_H__ */
EOF

# Only update if changed (to avoid unnecessary rebuilds)
if [ ! -f "$OUTPUT_FILE" ] || ! cmp -s "$TEMP_FILE" "$OUTPUT_FILE"; then
    mv "$TEMP_FILE" "$OUTPUT_FILE"
    echo "  GEN     $OUTPUT_FILE"
else
    rm -f "$TEMP_FILE"
fi
