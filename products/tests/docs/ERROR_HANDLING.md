# Test Framework Error Handling and Validation

This document describes the enhanced error handling and validation features in the EMS test framework.

## Overview

The test framework now provides comprehensive validation and actionable error messages to help developers maintain consistency between test files and Kconfig options.

## Features

### 1. Automatic Test Discovery Validation

When CMake discovers test files, it validates that:
- Each test file has a corresponding Kconfig option
- Kconfig option names match test file names
- Test files follow naming conventions

### 2. Enhanced Error Messages

All error messages follow this pattern:
1. **What went wrong**: Clear description of the issue
2. **Where**: Exact file path and location
3. **Why**: Explanation of the requirement
4. **How to fix**: Step-by-step actionable guidance

### 3. Configuration Consistency Checks

The framework validates:
- Test file → Kconfig mapping
- Kconfig → Test file mapping
- Naming convention compliance
- Character validity in filenames

## Error Types and Solutions

### Missing Kconfig Option

**When it occurs**: You created a test file but didn't add the corresponding Kconfig option.

**Error message**:
```
WARNING: Test file 'test_osal_mutex.c' found but CONFIG_TEST_OSAL_MUTEX not defined in Kconfig.
  File: /home/user/EMS/products/tests/unit/osal/test_osal_mutex.c
  Expected config: CONFIG_TEST_OSAL_MUTEX
  Location: products/tests/unit/osal/Kconfig

  Actionable steps:
  1. Add the following to products/tests/unit/osal/Kconfig:

     config TEST_OSAL_MUTEX
         bool "Mutex tests"
         default y
         help
           Test OSAL mutex operations (create, lock, unlock, trylock, destroy).
           Runtime: <100ms
           Hardware: None
           Dependencies: CONFIG_OSAL

  2. Update the TEST_OSAL_ALL option to select TEST_OSAL_MUTEX
  3. Run: python3 build.py menuconfig
  4. Or remove the test file if it's obsolete or incorrectly named.
```

**Solution**:
1. Open `products/tests/unit/osal/Kconfig`
2. Add the config block as shown above
3. Update `TEST_OSAL_ALL` to include `select TEST_OSAL_MUTEX`
4. Run `python3 build.py menuconfig` to verify
5. Run `python3 build.py build` to compile

### Orphaned Kconfig Option

**When it occurs**: A Kconfig option exists but there's no corresponding test file.

**How to detect**: Run the validation script:
```bash
python3 products/tests/scripts/validate_test_config.py
```

**Example output**:
```
⚠️  Kconfig options without test files: 1
  - CONFIG_TEST_OSAL_OBSOLETE -> missing test_osal_obsolete.c
    Either:
      1. Create test file: unit/osal/test_osal_obsolete.c
      2. Remove from Kconfig: unit/osal/Kconfig
```

**Solution A** (Create missing test file):
```bash
cd products/tests/unit/osal
cp test_osal_mutex.c test_osal_obsolete.c
# Edit test_osal_obsolete.c to implement tests
```

**Solution B** (Remove orphaned config):
1. Open the Kconfig file
2. Remove the `config TEST_OSAL_OBSOLETE` block
3. Remove from `TEST_OSAL_ALL` selection list
4. Run `python3 build.py menuconfig`

### Naming Convention Violation

**When it occurs**: Test file name doesn't follow the `test_<module>_<name>.c` pattern.

**Error message**:
```
WARNING: Test file 'osal_mutex_test.c' does not follow naming convention.
  File: /home/user/EMS/products/tests/unit/osal/osal_mutex_test.c
  Expected: test_<module>_<name>.c
  Action: Rename the file to match convention, or move to non-test directory.
```

**Solution**:
```bash
cd products/tests/unit/osal
git mv osal_mutex_test.c test_osal_mutex.c
```

### Invalid Characters in Filename

**When it occurs**: Test filename contains spaces or special characters.

**Error message**:
```
WARNING: Test file 'test osal-mutex.c' contains invalid characters.
  Use only: a-z, A-Z, 0-9, underscore
  Location: /home/user/EMS/products/tests/unit/osal/test osal-mutex.c
```

**Solution**:
```bash
cd products/tests/unit/osal
git mv "test osal-mutex.c" test_osal_mutex.c
```

## Kconfig Help Text Format

All Kconfig options should include comprehensive help text following this template:

```kconfig
config TEST_<MODULE>_<NAME>
    bool "<Short description>"
    default y
    help
      Test <module> <functionality> operations (op1, op2, op3).

      Coverage:
        - Feature 1 description
        - Feature 2 description
        - Feature 3 description
        - Error handling scenarios

      Runtime: <estimated time in ms>
      Hardware: <hardware requirements or "None">
      Dependencies: CONFIG_<MODULE>
      Tags: <fast|slow|hardware|filesystem|network|stress>
      Test file: test_<module>_<name>.c
```

### Help Text Sections

1. **First line**: Brief description of what is being tested
2. **Coverage**: Detailed list of features and scenarios covered
3. **Runtime**: Estimated execution time (helps users choose which tests to run)
4. **Hardware**: Hardware dependencies (helps identify tests requiring real hardware)
5. **Dependencies**: Required Kconfig options (ensures proper dependency ordering)
6. **Tags**: Classification tags for filtering (fast/slow/hardware/etc.)
7. **Test file**: Explicit mapping to source file (documentation and validation)

### Example: Complete Kconfig Entry

```kconfig
config TEST_OSAL_MUTEX
    bool "Mutex tests"
    default y
    help
      Test OSAL mutex operations (create, lock, unlock, trylock, destroy).

      Coverage:
        - Mutex creation and destruction
        - Lock and unlock operations
        - Try-lock (non-blocking) operations
        - Recursive mutex support
        - Priority inheritance (if supported)
        - Deadlock detection
        - Thread-safety verification

      Runtime: <100ms
      Hardware: None
      Dependencies: CONFIG_OSAL
      Tags: fast
      Test file: test_osal_mutex.c
```

## Validation Workflow

### During Development

```bash
# 1. Create test file
touch products/tests/unit/osal/test_osal_newfeature.c

# 2. Validate configuration
python3 products/tests/scripts/validate_test_config.py

# 3. Add missing Kconfig entry (follow error message instructions)
vim products/tests/unit/osal/Kconfig

# 4. Validate again
python3 products/tests/scripts/validate_test_config.py

# 5. Configure and build
python3 build.py config tests_x86_full_defconfig
python3 build.py build
```

### Before Committing

```bash
# Run validation to catch any issues
python3 products/tests/scripts/validate_test_config.py

# If validation passes, commit
git add products/tests/unit/osal/test_osal_newfeature.c
git add products/tests/unit/osal/Kconfig
git commit -m "test(osal): add newfeature tests"
```

### In CI Pipeline

Add to `.github/workflows/test.yml` or CI configuration:

```yaml
- name: Validate test configuration
  run: |
    python3 products/tests/scripts/validate_test_config.py
    if [ $? -ne 0 ]; then
      echo "❌ Test configuration validation failed"
      exit 1
    fi
```

## CMake Build Statistics

When building, CMake now shows detailed statistics:

```
-- Discovering unit tests:
--   OSAL: 19/19 tests enabled
--     Enabled: test_osal_atomic, test_osal_clock, test_osal_cond, ...
--   HAL: 6/6 tests enabled
--     Enabled: test_hal_can, test_hal_gpio, test_hal_i2c, ...
--   PDL: 5/5 tests enabled
--   PRL: 2/2 tests enabled
--   PCONFIG: 1/1 tests enabled
--   ACONFIG: 1/1 tests enabled
```

This helps verify that:
- All expected tests are found
- Correct number of tests are enabled
- Configuration matches expectations

## Troubleshooting

### Issue: CMake doesn't find my new test

**Check**:
1. Is the file named `test_*.c`?
2. Is it in the correct directory?
3. Does the Kconfig option exist?
4. Is the Kconfig option enabled in `.config`?

**Debug**:
```bash
# Reconfigure from scratch
python3 build.py distclean
python3 build.py config tests_x86_full_defconfig

# Check if config is defined
grep TEST_OSAL_NEWFEATURE .config

# Build with verbose output
python3 build.py build --verbose 2>&1 | grep test_osal_newfeature
```

### Issue: Test file not compiling

**Check**:
1. Does it include required headers?
2. Are dependencies linked in CMakeLists.txt?
3. Is the module (OSAL/HAL/PDL) enabled?

**Debug**:
```bash
# Check compilation command
cd _build
make VERBOSE=1 2>&1 | grep test_osal_newfeature
```

### Issue: Validation script reports false positives

**Possible causes**:
1. Test file in wrong directory
2. Symlinks or duplicate files
3. Backup files (test_osal_mutex.c.bak)

**Solution**:
```bash
# List all test files
find products/tests -name "test_*.c" -type f

# Remove backup files
find products/tests -name "*.c.bak" -delete
find products/tests -name "*~" -delete
```

## Best Practices

1. **Add Kconfig before test file**: Create the Kconfig entry first, then the test file
2. **Use validation script**: Run validation before committing
3. **Keep help text updated**: Update help text when test coverage changes
4. **Follow naming convention**: Always use `test_<module>_<name>.c`
5. **Test configuration changes**: Run `python3 build.py menuconfig` after Kconfig changes
6. **Document runtime**: Keep runtime estimates accurate for test selection

## References

- [Test Discovery CMake Module](../cmake/TestDiscovery.cmake)
- [Kconfig Documentation](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
- [Test Framework Documentation](../README.md)
- [Validation Script](../scripts/validate_test_config.py)
