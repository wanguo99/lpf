#!/bin/bash
# Script to update all defconfig files with new configuration options
# Each config takes ~30-60 seconds to process with yes ""

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Find all defconfig files
DEFCONFIGS=$(find configs -name "*_defconfig" -type f | sort)

TOTAL=$(echo "$DEFCONFIGS" | wc -l)
echo "Found $TOTAL defconfig files to update"
echo "This may take 10-20 minutes for all configs..."
echo ""

UPDATED=0
FAILED=0
CURRENT=0

for defconfig_path in $DEFCONFIGS; do
    CURRENT=$((CURRENT + 1))
    defconfig_name=$(basename "$defconfig_path")
    echo "[$CURRENT/$TOTAL] Processing: $defconfig_name"

    # Clean previous configuration
    make distclean >/dev/null 2>&1 || true

    # Load the defconfig with extended timeout (90 seconds per config)
    # Use yes "" to auto-answer all prompts with defaults
    if timeout 90 bash -c "yes \"\" | scripts/kconfig/conf -D \"$defconfig_path\" Config.in >/dev/null 2>&1"; then
        # Check if .config was generated
        if [ -f .config ]; then
            # Save the updated configuration back
            cp .config "$defconfig_path"
            echo "  ✓ Updated: $defconfig_path"
            UPDATED=$((UPDATED + 1))
        else
            echo "  ✗ Failed: No .config generated"
            FAILED=$((FAILED + 1))
        fi
    else
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "  ✗ Failed: Timeout (>90s)"
        else
            echo "  ✗ Failed: Configuration error (exit code: $EXIT_CODE)"
        fi
        FAILED=$((FAILED + 1))
    fi
    echo ""
done

# Clean up
make distclean >/dev/null 2>&1 || true

echo "=========================================="
echo "Defconfig Update Summary"
echo "=========================================="
echo "Updated: $UPDATED"
echo "Failed:  $FAILED"
echo "Total:   $TOTAL"
echo ""

if [ $FAILED -gt 0 ]; then
    echo "⚠ Some defconfigs failed to update."
    echo "You may need to update them manually with:"
    echo "  make distclean"
    echo "  yes \"\" | make <config>_defconfig"
    echo "  cp .config configs/<subdir>/<config>_defconfig"
    exit 1
else
    echo "✓ All defconfigs updated successfully!"
    exit 0
fi
