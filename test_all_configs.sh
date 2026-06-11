#!/bin/bash
# Test all ES-Middleware configurations
# This script validates clean, configure, and build for each defconfig

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

CONFIGS_DIR="configs"
LOG_DIR="test_logs"
PASSED=0
FAILED=0
TOTAL=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create log directory
mkdir -p "$LOG_DIR"

echo "========================================"
echo "ES-Middleware Configuration Test Suite"
echo "========================================"
echo ""

# Get all defconfig files
DEFCONFIGS=($(ls -1 "$CONFIGS_DIR"/*_defconfig 2>/dev/null | xargs -n1 basename))

if [ ${#DEFCONFIGS[@]} -eq 0 ]; then
    echo -e "${RED}ERROR: No defconfig files found in $CONFIGS_DIR${NC}"
    exit 1
fi

echo "Found ${#DEFCONFIGS[@]} configurations to test"
echo ""

# Function to test one configuration
test_config() {
    local config="$1"
    local log_file="$LOG_DIR/${config}.log"

    echo -n "Testing $config ... "
    TOTAL=$((TOTAL + 1))

    # Clear log file
    > "$log_file"

    # Step 1: Clean
    echo "=== Step 1: Clean ===" >> "$log_file"
    if ! make distclean >> "$log_file" 2>&1; then
        echo -e "${RED}FAILED${NC} (distclean error)"
        echo "ERROR: distclean failed" >> "$log_file"
        FAILED=$((FAILED + 1))
        return 1
    fi

    # Step 2: Configure
    echo "=== Step 2: Configure ===" >> "$log_file"
    if ! make "$config" >> "$log_file" 2>&1; then
        echo -e "${RED}FAILED${NC} (configure error)"
        echo "ERROR: Configuration loading failed" >> "$log_file"
        FAILED=$((FAILED + 1))
        return 1
    fi

    # Verify .config was created
    if [ ! -f ".config" ]; then
        echo -e "${RED}FAILED${NC} (.config not created)"
        echo "ERROR: .config file not created" >> "$log_file"
        FAILED=$((FAILED + 1))
        return 1
    fi

    # Step 3: Build
    echo "=== Step 3: Build ===" >> "$log_file"
    if ! make -j$(nproc) >> "$log_file" 2>&1; then
        echo -e "${RED}FAILED${NC} (build error)"
        echo "ERROR: Build failed" >> "$log_file"
        FAILED=$((FAILED + 1))
        return 1
    fi

    # Verify build outputs exist
    if [ ! -d "_build" ]; then
        echo -e "${RED}FAILED${NC} (_build not created)"
        echo "ERROR: _build directory not created" >> "$log_file"
        FAILED=$((FAILED + 1))
        return 1
    fi

    # Step 4: Test clean (without removing config)
    echo "=== Step 4: Test clean ===" >> "$log_file"
    if ! make clean >> "$log_file" 2>&1; then
        echo -e "${YELLOW}WARNING${NC} (clean failed)"
        echo "WARNING: make clean failed" >> "$log_file"
        # Don't fail the test for this
    fi

    # Success
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
    return 0
}

# Test each configuration
for config in "${DEFCONFIGS[@]}"; do
    test_config "$config"
done

# Final cleanup
echo ""
echo "Cleaning up..."
make distclean > /dev/null 2>&1 || true

# Summary
echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo -e "Total:  $TOTAL"
echo -e "${GREEN}Passed: $PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED${NC}"
else
    echo -e "Failed: 0"
fi
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All configurations passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some configurations failed.${NC}"
    echo "Check logs in $LOG_DIR/ for details"
    exit 1
fi
