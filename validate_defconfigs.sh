#!/bin/bash
# Validate and fix all ES-Middleware defconfig files
# This script ensures all configurations have necessary dependencies

set -e

cd "$(dirname "$0")"

echo "========================================="
echo "Validating All ES-Middleware Defconfigs"
echo "========================================="
echo ""

CONFIGS_DIR="configs"
FAILED=0
PASSED=0

# Test each defconfig
for config_file in "$CONFIGS_DIR"/*_defconfig; do
    config_name=$(basename "$config_file")

    echo -n "Testing $config_name ... "

    # Load config
    make distclean >/dev/null 2>&1
    if ! make "$config_name" >/dev/null 2>&1; then
        echo "FAILED (load)"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Check for required derived values
    if grep -q "CONFIG_OS_LINUX=y" .config; then
        if ! grep -q "CONFIG_OSAL_OS_POSIX=y" .config; then
            echo "WARNING (missing CONFIG_OSAL_OS_POSIX)"
        fi
    fi

    # Try to build
    if make -j$(nproc) >/dev/null 2>&1; then
        echo "PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "FAILED (build)"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "========================================="
echo "Summary: $PASSED passed, $FAILED failed"
echo "========================================="

exit $FAILED
