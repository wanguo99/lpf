# Test Reporting and CI Integration Guide

This document describes the advanced reporting features added to the EMS test framework.

## Features

### 1. JUnit XML Export

Export test results in JUnit XML format for CI integration:

```bash
# Run all tests and export to JUnit XML
es-middleware-test -a --format junit --output test-results.xml

# Run specific layer tests with JUnit output
es-middleware-test -L OSAL --format junit --output osal-results.xml

# Run only fast tests with JUnit output
es-middleware-test -a --fast --format junit --output fast-results.xml
```

**Supported CI Systems:**
- Jenkins
- GitLab CI
- GitHub Actions
- Azure DevOps
- CircleCI
- Travis CI

### 2. JSON Export

Export test results in JSON format for automation and custom tooling:

```bash
# Run all tests and export to JSON
es-middleware-test -a --format json --output test-results.json

# JSON format includes:
# - Summary statistics (total, passed, failed, timing)
# - List of passed tests with elapsed time
# - List of failed tests with elapsed time
```

**JSON Structure:**
```json
{
  "summary": {
    "total": 150,
    "passed": 148,
    "failed": 2,
    "total_time_ms": 5432,
    "avg_time_ms": 36
  },
  "passed": [
    {"suite": "osal_mutex", "test": "test_mutex_create", "elapsed_ms": 5},
    ...
  ],
  "failed": [
    {"suite": "hal_can", "test": "test_can_send_timeout", "elapsed_ms": 120}
  ]
}
```

### 3. Per-Suite Statistics

Automatically displayed after test execution:

```
[==========]
 Per-Suite Statistics
[==========]
  Suite                          Total   Pass   Fail   Time(ms)
  -----------------------------------------------------------------------
  osal_mutex                        15     15      0         85
  osal_thread                       12     12      0        145
  hal_can                            8      7      1        320
  pdl_mcu                           10     10      0        180
```

### 4. Slowest Tests Report

Automatically shows the top 10 slowest tests after execution:

```
[==========]
 Top 10 Slowest Tests
[==========]
   320 ms  [hal_can] test_can_send_timeout
   180 ms  [pdl_mcu] test_mcu_firmware_update
   145 ms  [osal_thread] test_thread_stress
    85 ms  [osal_mutex] test_mutex_contention
    ...
```

### 5. Command-Line Options

```bash
# Output format (text, junit, json)
--format <fmt>         # Default: text

# Output file (for junit/json formats)
--output <file>        # Example: test-results.xml

# Disable statistics (for minimal output)
--no-stats             # Skip per-suite stats and slowest tests
```

## CI Integration Examples

### GitHub Actions

See `.github/workflows/test.yml` for complete configuration.

```yaml
- name: Run tests
  run: |
    ./_build/bin/es-middleware-test -a --format junit --output test-results.xml

- name: Publish test results
  uses: EnricoMi/publish-unit-test-result-action@v2
  with:
    files: test-results.xml
```

### GitLab CI

See `.gitlab-ci.yml` for complete configuration.

```yaml
test:unit:
  script:
    - _build/bin/es-middleware-test -a --format junit --output test-results.xml
  artifacts:
    reports:
      junit: test-results.xml
```

### Jenkins

```groovy
pipeline {
    stages {
        stage('Test') {
            steps {
                sh './_build/bin/es-middleware-test -a --format junit --output test-results.xml'
            }
        }
    }
    post {
        always {
            junit 'test-results.xml'
        }
    }
}
```

## Usage Examples

### Run all tests with full reporting
```bash
es-middleware-test -a --format junit --output results.xml
```

### Run fast tests for quick feedback
```bash
es-middleware-test -a --fast --format junit --output fast-results.xml
```

### Run specific layer with JSON export
```bash
es-middleware-test -L OSAL --format json --output osal-results.json
```

### Run tests without hardware requirements (CI-friendly)
```bash
es-middleware-test -a --exclude-hardware --format junit --output ci-results.xml
```

### Run unit tests only
```bash
es-middleware-test -a --category unit --format junit --output unit-results.xml
```

### Multiple output formats
```bash
# Text output to console, XML for CI
es-middleware-test -a --format junit --output results.xml

# JSON for analysis, text for debugging
es-middleware-test -a --format json --output results.json
```

## API Reference

### Export Functions

```c
/**
 * Export test results in JUnit XML format
 * @param output_path Path to output XML file
 * @return OSAL_SUCCESS on success, error code otherwise
 */
int32_t libutest_export_junit_xml(const char *output_path);

/**
 * Export test results in JSON format
 * @param output_path Path to output JSON file
 * @return OSAL_SUCCESS on success, error code otherwise
 */
int32_t libutest_export_json(const char *output_path);
```

### Statistics Functions

```c
/**
 * Print the N slowest tests
 * @param top_n Number of tests to display
 */
void libutest_print_slowest_tests(uint32_t top_n);

/**
 * Print per-suite statistics
 */
void libutest_print_suite_stats(void);
```

## Implementation Details

### File Organization

```
products/tests/
├── core/
│   ├── test_reporter.c          # New: Export and reporting implementation
│   ├── test_runner.c            # Modified: Added statistics calls
│   └── main.c                   # Modified: Added --format, --output options
├── include/
│   └── test_core.h              # Modified: Added export API declarations
└── CMakeLists.txt               # Modified: Added test_reporter.c to build
```

### XML Escaping

The JUnit XML exporter properly escapes special characters:
- `&` → `&amp;`
- `<` → `&lt;`
- `>` → `&gt;`
- `"` → `&quot;`
- `'` → `&apos;`

### Performance

- Statistics collection has minimal overhead (< 1% runtime impact)
- XML/JSON export happens after test execution (no impact on test timing)
- Slowest tests report uses qsort for O(n log n) sorting

## Troubleshooting

### XML file not created
- Check write permissions in output directory
- Verify output path is absolute or relative to execution directory

### CI system not picking up results
- Ensure XML file path matches CI configuration
- Verify JUnit XML format is valid (use XML validator)
- Check CI logs for file path mismatches

### Statistics not showing
- Verify tests are actually running
- Check if `--no-stats` flag was accidentally set
- Ensure test framework is linked correctly

## Future Enhancements

Potential additions:
- HTML report generation
- Test trend analysis
- Performance regression detection
- Code coverage integration
- Custom report templates
