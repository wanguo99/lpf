# Test Maintenance Scripts

This directory contains maintenance scripts for the ES-Middleware test framework.

## Overview

These scripts help maintain consistency and quality of the test infrastructure:

- **validate_test_config.py** - Validates Kconfig consistency with test files
- **sync_kconfig.py** - Auto-generates Kconfig files from test directory structure
- **validate_tests.py** - Validates test file naming and code consistency

## Scripts

### validate_test_config.py

Validates consistency between test files and Kconfig options. This script helps detect configuration mismatches and ensures the test discovery system works correctly.

#### Features

- **Test file validation**: Checks that all test_*.c files have corresponding Kconfig options
- **Kconfig validation**: Detects orphaned Kconfig options without test files
- **Naming convention checks**: Validates test file naming (test_<module>_<name>.c)
- **Character validation**: Ensures filenames contain only valid characters
- **Multi-category support**: Can validate unit, performance, stress, and system tests

#### Usage

```bash
# Validate all unit tests (default)
python3 validate_test_config.py

# Validate specific category
python3 validate_test_config.py --category unit

# Verbose output with fix suggestions
python3 validate_test_config.py --verbose
```

#### Example Output

```
================================================================================
Module: unit/osal
================================================================================
Test files: 19
Kconfig options: 19
✅ All checks passed!

================================================================================
Module: unit/hal
================================================================================
Test files: 6
Kconfig options: 6
✅ All checks passed!

================================================================================
Summary
================================================================================
Total issues found:
  - Missing Kconfig options: 0
  - Orphaned Kconfig options: 0
  - Naming violations: 0

✅ All validations passed!
```

#### Error Detection

The script detects and reports:

1. **Missing Kconfig options**
   ```
   ⚠️  Test files without Kconfig options: 1
     - test_osal_new_feature.c -> missing CONFIG_TEST_OSAL_NEW_FEATURE
       Add to unit/osal/Kconfig:
       config TEST_OSAL_NEW_FEATURE
           bool "New feature tests"
           default y
           help
             Test <functionality>.
             Runtime: <time>
             Hardware: <requirements>
             Dependencies: CONFIG_OSAL
   ```

2. **Orphaned Kconfig options**
   ```
   ⚠️  Kconfig options without test files: 1
     - CONFIG_TEST_OSAL_OLD_FEATURE -> missing test_osal_old_feature.c
       Either:
         1. Create test file: unit/osal/test_osal_old_feature.c
         2. Remove from Kconfig: unit/osal/Kconfig
   ```

3. **Naming convention violations**
   ```
   ⚠️  Naming convention violations: 1
     - File 'osal_test.c' does not start with 'test_'
   ```

#### Integration with CI

Add to your CI pipeline:

```bash
# Run validation before build
python3 products/tests/scripts/validate_test_config.py
if [ $? -ne 0 ]; then
    echo "Test configuration validation failed!"
    exit 1
fi
```

#### When to Run

- **Before adding new test files**: Ensure naming follows convention
- **After removing test files**: Check for orphaned Kconfig options
- **During code review**: Validate configuration consistency
- **In CI pipeline**: Catch configuration errors early

### sync_kconfig.py

Auto-generates Kconfig files from test directory structure. This script scans test files and creates corresponding Kconfig entries automatically.

#### Features

- **Auto-discovery**: Scans test_*.c files and generates Kconfig entries
- **Metadata extraction**: Parses test file comments for metadata (@test_description, @test_runtime, etc.)
- **Preserve existing help**: Preserves manually written help text from existing Kconfig files
- **TEST_*_ALL generation**: Automatically creates selection configs
- **Multi-category support**: Works with unit, performance, stress, and system tests

#### Usage

```bash
# Dry-run (preview changes without writing)
python3 sync_kconfig.py --dry-run

# Generate Kconfig for all modules
python3 sync_kconfig.py --write

# Generate for specific category/module
python3 sync_kconfig.py --category unit --module osal --write

# Interactive mode (review and confirm each change)
python3 sync_kconfig.py --interactive

# Preserve existing help text (default)
python3 sync_kconfig.py --preserve --write
```

#### Test Metadata Format

Add metadata comments to your test files for better Kconfig generation:

```c
// @test_description: Test OSAL mutex operations
// @test_runtime: <50ms
// @test_hardware: None
// @test_tags: fast

#include "test_framework.h"
#include "osal.h"

TEST_CASE(test_mutex_create_success) {
    // Test implementation
}
```

#### Example Output

```
================================================================================
Category: unit
================================================================================

Processing: unit/osal

[MODIFIED] products/tests/unit/osal/Kconfig
================================================================================
- config TEST_OSAL_MUTEX
-     bool "Mutex tests"
+ config TEST_OSAL_MUTEX
+     bool "Test OSAL mutex operations"
      default y
      help
+       Test OSAL mutex operations.
+
+       Runtime: <50ms
+       Hardware: None
+       Dependencies: CONFIG_OSAL
+       Tags: fast
+       Test file: test_osal_mutex.c
================================================================================
✅ Written: products/tests/unit/osal/Kconfig

Summary
================================================================================
Files generated: 6
Files unchanged: 0

💡 Run validate_test_config.py to verify changes
```

#### When to Use

- **After adding new test files**: Auto-generate missing Kconfig entries
- **During refactoring**: Regenerate all Kconfig files consistently
- **Updating documentation**: Sync help text from test file comments
- **Initial setup**: Bootstrap Kconfig structure for new modules

#### Integration with Workflow

```bash
# 1. Add new test file
touch products/tests/unit/osal/test_osal_new_feature.c

# 2. Add metadata comments to test file
# @test_description: Test new feature
# @test_runtime: <100ms

# 3. Generate Kconfig entry
python3 products/tests/scripts/sync_kconfig.py --category unit --module osal --write

# 4. Verify
python3 products/tests/scripts/validate_test_config.py --category unit --module osal

# 5. Build
make menuconfig
make
```

---

### validate_tests.py

Validates test file naming consistency and checks for common issues. This script performs comprehensive checks beyond Kconfig consistency.

#### Features

- **Naming convention validation**: Ensures test_<module>_<name>.c format
- **Character validation**: Checks for invalid characters and patterns
- **Kconfig consistency**: Checks test files match Kconfig options
- **Duplicate detection**: Finds duplicate test names across modules
- **Registration checks**: Verifies tests use proper registration macros
- **CMake integration**: Checks test discovery configuration

#### Usage

```bash
# Validate all tests
python3 validate_tests.py

# Validate specific category
python3 validate_tests.py --category unit

# Check specific module
python3 validate_tests.py --category unit --module osal

# Show detailed suggestions
python3 validate_tests.py --verbose

# Check for missing test registration
python3 validate_tests.py --check-registration

# Strict mode (warnings become errors)
python3 validate_tests.py --strict
```

#### Example Output

```
================================================================================
Module: unit/osal
================================================================================
Test files: 19
Kconfig options: 19
✅ All checks passed!

================================================================================
Module: unit/hal
================================================================================
Test files: 6
Kconfig options: 6

❌ Naming convention violations: 1
  • hal_test.c
    - Does not start with 'test_' prefix
    Fix: Rename to follow test_<module>_<name>.c convention

❌ Test files without Kconfig: 1
  • test_hal_new.c -> missing CONFIG_TEST_HAL_NEW
    Fix: Add to unit/hal/Kconfig or run sync_kconfig.py

================================================================================
Summary
================================================================================
Total test files: 25
Total issues: 2
  - Naming violations: 1
  - Registration issues: 0
  - Missing Kconfig: 1
  - Orphaned Kconfig: 0
  - CMake issues: 0

⚠️  Please fix the issues above.

Next steps:
  1. Fix naming convention violations
  2. Add missing test registration macros
  3. Run: python3 sync_kconfig.py --write (to auto-generate Kconfig)
  4. Run: python3 validate_test_config.py (to verify)
```

#### Validation Checks

1. **Naming Convention**
   - Must start with `test_`
   - Must follow format: `test_<module>_<name>.c`
   - Only lowercase letters, numbers, and underscores
   - No double underscores or trailing underscores
   - Reasonable length (< 50 characters)

2. **Code Registration**
   - Has `TEST_SUITE_AUTO_REGISTER()` or `TEST_MODULE_BEGIN/END`
   - Contains at least one `TEST_CASE()` definition
   - No mixed registration styles

3. **Kconfig Consistency**
   - Each test file has corresponding Kconfig option
   - No orphaned Kconfig options without test files
   - Config names match file names (test_osal_mutex.c → CONFIG_TEST_OSAL_MUTEX)

4. **CMake Integration**
   - Test files referenced in CMakeLists.txt
   - Uses test_discover_and_add() or manual listing

5. **Duplicate Detection**
   - No duplicate test file names across modules

#### When to Use

- **Before committing**: Ensure all tests follow conventions
- **After adding tests**: Verify proper setup
- **During code review**: Catch issues early
- **In CI pipeline**: Automated validation
- **Periodic audits**: Maintain code quality

#### Integration with CI

Add to `.github/workflows/test.yml` or CI config:

```yaml
- name: Validate test consistency
  run: |
    python3 products/tests/scripts/validate_tests.py --check-registration --strict
    if [ $? -ne 0 ]; then
      echo "Test validation failed!"
      exit 1
    fi
```

---

## Workflow: Adding a New Test

Complete workflow using all three scripts:

```bash
# 1. Create test file with proper naming
cat > products/tests/unit/osal/test_osal_new_feature.c << 'EOF'
// @test_description: Test new OSAL feature
// @test_runtime: <100ms
// @test_hardware: None
// @test_tags: fast

#include "test_framework.h"
#include "osal.h"

TEST_CASE(test_feature_basic) {
    TEST_ASSERT_EQUAL(0, osal_new_feature());
}

TEST_SUITE_AUTO_REGISTER();
EOF

# 2. Validate naming and structure
python3 products/tests/scripts/validate_tests.py \
    --category unit --module osal --check-registration --verbose

# 3. Auto-generate Kconfig entry
python3 products/tests/scripts/sync_kconfig.py \
    --category unit --module osal --interactive

# 4. Verify Kconfig consistency
python3 products/tests/scripts/validate_test_config.py \
    --category unit --verbose

# 5. Build and test
make menuconfig  # Enable CONFIG_TEST_OSAL_NEW_FEATURE
make
./_build/bin/es-middleware-test
```

## Workflow: Refactoring Test Structure

```bash
# 1. Validate current state
python3 products/tests/scripts/validate_tests.py --verbose

# 2. Make changes (rename files, move tests, etc.)
mv products/tests/unit/osal/old_test.c products/tests/unit/osal/test_osal_new.c

# 3. Regenerate all Kconfig files
python3 products/tests/scripts/sync_kconfig.py --write

# 4. Validate consistency
python3 products/tests/scripts/validate_test_config.py

# 5. Update defconfig files if needed
make tests_x86_full_defconfig_defconfig
make
```

## Future Enhancements

### generate_test_report.py (planned)

Generate test coverage and statistics reports:
- Number of tests per module/category
- Test execution time statistics
- Pass/fail rates over time
- Coverage gaps identification

### auto_update_defconfig.py (planned)

Automatically update defconfig files when new tests are added:
- Scan for new CONFIG_TEST_* options
- Update relevant defconfig files
- Maintain consistent defaults

## Contributing

When adding new maintenance scripts:
1. Follow Python PEP 8 style guidelines
2. Add comprehensive docstrings
3. Include usage examples
4. Update this README
5. Add error handling and validation
