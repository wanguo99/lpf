#!/bin/bash
# ============================================================================
# Configuration System Validation Script
# ============================================================================
# Tests all defconfig files to ensure they load correctly after refactoring
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "========================================================================"
echo "ES-Middleware Configuration System Validation"
echo "========================================================================"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
TOTAL=0

# Function to test a defconfig
test_defconfig() {
    local defconfig="$1"
    local name="$(basename "$defconfig")"

    echo -n "Testing $name ... "
    TOTAL=$((TOTAL + 1))

    # Clean previous config
    make distclean > /dev/null 2>&1

    # Try to load the defconfig - use the full basename as target
    if make "$name" > /tmp/config_test.log 2>&1; then
        # Check for warnings about unmet dependencies
        if grep -q "WARNING: unmet direct dependencies" /tmp/config_test.log; then
            echo -e "${YELLOW}WARN${NC} (unmet dependencies)"
            grep "WARNING: unmet direct dependencies" -A 3 /tmp/config_test.log | head -5
            PASSED=$((PASSED + 1))
            return 0
        fi

        # Check for errors in output
        if grep -qi "ERROR:" /tmp/config_test.log; then
            echo -e "${RED}FAIL${NC} (configuration errors)"
            grep -i "ERROR:" /tmp/config_test.log | head -5
            FAILED=$((FAILED + 1))
            return 1
        fi

        # Check if .config was generated
        if [ -f .config ]; then
            echo -e "${GREEN}PASS${NC}"
            PASSED=$((PASSED + 1))
            return 0
        else
            echo -e "${RED}FAIL${NC} (.config not generated)"
            FAILED=$((FAILED + 1))
            return 1
        fi
    else
        echo -e "${RED}FAIL${NC} (failed to load)"
        tail -10 /tmp/config_test.log
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# Test all CCM configs
echo "Testing CCM Configurations:"
echo "----------------------------------------"
while IFS= read -r config; do
    test_defconfig "$config"
done < <(find configs/ccm -name "*_defconfig" -type f | sort)
echo ""

# Test all x86 test configs
echo "Testing x86 Test Configurations:"
echo "----------------------------------------"
while IFS= read -r config; do
    test_defconfig "$config"
done < <(find configs/tests -name "tests_x86_*_defconfig" -type f | sort)
echo ""

# Test all ARM64 test configs
echo "Testing ARM64 Test Configurations:"
echo "----------------------------------------"
while IFS= read -r config; do
    test_defconfig "$config"
done < <(find configs/tests -name "tests_arm64_*_defconfig" -type f | sort)
echo ""

# Summary
echo "========================================================================"
echo "Validation Summary:"
echo "========================================================================"
echo "Total configurations tested: $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All configurations validated successfully!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some configurations failed validation.${NC}"
    echo "Please review the errors above and fix the configuration issues."
    exit 1
fi
