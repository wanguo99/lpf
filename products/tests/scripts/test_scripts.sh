#!/bin/bash
# Test script to verify all maintenance tools work correctly

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../../.."

echo "Testing EMS Test Maintenance Scripts"
echo "======================================"
echo ""

# Test 1: validate_test_config.py
echo "[1/3] Testing validate_test_config.py..."
python3 products/tests/scripts/validate_test_config.py --category unit > /dev/null
if [ $? -eq 0 ]; then
    echo "  ✅ validate_test_config.py works correctly"
else
    echo "  ❌ validate_test_config.py failed"
    exit 1
fi
echo ""

# Test 2: validate_tests.py
echo "[2/3] Testing validate_tests.py..."
python3 products/tests/scripts/validate_tests.py --category unit --module osal > /dev/null
if [ $? -eq 0 ]; then
    echo "  ✅ validate_tests.py works correctly"
else
    echo "  ❌ validate_tests.py failed"
    exit 1
fi
echo ""

# Test 3: sync_kconfig.py (dry-run only)
echo "[3/3] Testing sync_kconfig.py..."
python3 products/tests/scripts/sync_kconfig.py --category unit --module osal > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "  ✅ sync_kconfig.py works correctly"
else
    echo "  ❌ sync_kconfig.py failed"
    exit 1
fi
echo ""

echo "======================================"
echo "All tests passed! ✅"
echo ""
echo "Available scripts:"
echo "  - validate_test_config.py - Check Kconfig consistency"
echo "  - validate_tests.py       - Check test naming and structure"
echo "  - sync_kconfig.py         - Auto-generate Kconfig files"
echo ""
echo "See README.md for detailed usage instructions."
