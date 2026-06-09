# Quick Start Guide for Test Maintenance Scripts

## Installation

No installation required. Scripts are located in `products/tests/scripts/`.

## Prerequisites

- Python 3.6+
- ES-Middleware project environment

## Quick Examples

### 1. Validate Existing Tests

Check if all test files have corresponding Kconfig options:

```bash
# Check all unit tests
python3 products/tests/scripts/validate_test_config.py

# Check specific module
python3 products/tests/scripts/validate_test_config.py --category unit --verbose
```

### 2. Validate Test Naming and Structure

Check test file naming conventions and code structure:

```bash
# Basic validation
python3 products/tests/scripts/validate_tests.py --category unit

# With registration checks
python3 products/tests/scripts/validate_tests.py --check-registration --verbose

# Strict mode (for CI)
python3 products/tests/scripts/validate_tests.py --strict
```

### 3. Auto-Generate Kconfig Files

Generate or update Kconfig files from test directory structure:

```bash
# Dry-run (preview changes)
python3 products/tests/scripts/sync_kconfig.py --category unit --module osal

# Interactive mode (review each change)
python3 products/tests/scripts/sync_kconfig.py --interactive

# Actually write files
python3 products/tests/scripts/sync_kconfig.py --category unit --write
```

## Common Workflows

### Adding a New Test File

```bash
# 1. Create test file with metadata
cat > products/tests/unit/osal/test_osal_new.c << 'EOF'
// @test_description: Test new OSAL feature
// @test_runtime: <100ms
// @test_hardware: None
// @test_tags: fast

#include "test_framework.h"
#include "osal.h"

TEST_CASE(test_new_feature) {
    TEST_ASSERT_EQUAL(0, 0);
}

TEST_SUITE_REGISTER(osal_new, "OSAL", osal_new_cases, 
                    TEST_CATEGORY_UNIT, TEST_TAG_FAST, 100, 
                    "New feature tests")
EOF

# 2. Validate naming
python3 products/tests/scripts/validate_tests.py --category unit --module osal

# 3. Generate Kconfig
python3 products/tests/scripts/sync_kconfig.py --category unit --module osal --write

# 4. Verify
python3 products/tests/scripts/validate_test_config.py --category unit --module osal
```

### Before Committing Changes

```bash
# Run all validations
python3 products/tests/scripts/validate_test_config.py
python3 products/tests/scripts/validate_tests.py --check-registration
```

### CI Integration

Add to your CI pipeline:

```yaml
- name: Validate tests
  run: |
    python3 products/tests/scripts/validate_test_config.py
    python3 products/tests/scripts/validate_tests.py --strict
```

## Troubleshooting

### "Missing Kconfig option" Error

Run `sync_kconfig.py` to auto-generate missing entries:

```bash
python3 products/tests/scripts/sync_kconfig.py --category unit --write
```

### "Naming violation" Error

Rename test file to follow convention: `test_<module>_<name>.c`

### "Missing registration" Error

Add one of these macros to your test file:
- `TEST_SUITE_REGISTER(...)` - Full registration with metadata
- `TEST_SUITE_AUTO_REGISTER()` - Auto registration (planned)
- `TEST_MODULE_BEGIN/END` - Legacy style

## Getting Help

```bash
# Show help for any script
python3 products/tests/scripts/validate_test_config.py --help
python3 products/tests/scripts/validate_tests.py --help
python3 products/tests/scripts/sync_kconfig.py --help
```

## More Information

See `README.md` in the same directory for detailed documentation.
