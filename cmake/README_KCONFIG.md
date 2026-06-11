# Kconfig Integration for ES-Middleware

This directory contains the CMake modules for integrating Kconfig configuration system into ES-Middleware.

## Overview

ES-Middleware uses **Kconfig** (from Buildroot/Linux kernel) for feature configuration and **CMake** for building. The integration provides:

1. **Native Kconfig tools** built at configure time (conf, mconf, nconf)
2. **Automatic header generation** (autoconf.h) from .config
3. **CMake variable export** for all CONFIG_* symbols
4. **Custom CMake targets** for interactive configuration (menuconfig, nconfig)
5. **Dependency tracking** - CMake reconfigures when .config changes

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ES-Middleware Build                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. KconfigTools.cmake                                       │
│     ├─ Builds conf, mconf, nconf from scripts/kconfig/      │
│     └─ Exports KCONFIG_*_EXECUTABLE variables               │
│                                                              │
│  2. Kconfig.cmake                                            │
│     ├─ Loads .config file                                   │
│     ├─ Generates autoconf.h (C header with #defines)        │
│     ├─ Generates kconfig.cmake (CMake variables)            │
│     ├─ Exports CONFIG_* to CMake cache                      │
│     └─ Creates custom targets (menuconfig, savedefconfig)   │
│                                                              │
│  3. CMakeLists.txt                                           │
│     ├─ include(KconfigTools.cmake)                          │
│     ├─ include(Kconfig.cmake)                               │
│     ├─ kconfig_config(...)                                  │
│     └─ Auto-includes autoconf.h for all C compilation       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Files

### Core Modules

- **`KconfigTools.cmake`** - Builds native Kconfig tools (conf, mconf, nconf)
- **`Kconfig.cmake`** - Main integration module for loading and processing Kconfig

### Build Products

Generated in `_build/config/`:
- **`autoconf.h`** - C header with all CONFIG_* as #defines
- **`kconfig.cmake`** - CMake script with all CONFIG_* as cache variables
- **`.config`** - Current Kconfig configuration (in project root)

### Tool Binaries

Built in `_build/kconfig-tools-build/`:
- **`conf`** - Command-line configuration tool
- **`mconf`** - Interactive menuconfig (ncurses TUI)
- **`nconf`** - Alternative ncurses interface

## Usage

### Basic Workflow

```bash
# 1. Load a predefined configuration
python3 build.py config ccm_h200_100p_am625_debug_defconfig

# 2. Customize configuration (optional)
python3 build.py menuconfig

# 3. Build the project
python3 build.py build

# 4. Save changes to defconfig (optional)
python3 build.py savedefconfig
```

### CMake Integration

In your root `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

# Build and load Kconfig tools
include(cmake/KconfigTools.cmake)
include(cmake/Kconfig.cmake)

# Configure Kconfig
kconfig_config(
    KCONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/Kconfig"
    CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/.config"
    AUTOCONF_H "${CMAKE_CURRENT_BINARY_DIR}/config/autoconf.h"
    CMAKE_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/config/kconfig.cmake"
    USE_NATIVE_TOOLS TRUE
)

project(my_project C)

# Auto-include autoconf.h for all C files
add_compile_options(-include ${CMAKE_CURRENT_BINARY_DIR}/config/autoconf.h)

# Use CONFIG_* variables in CMake
if(CONFIG_MY_FEATURE)
    add_subdirectory(my_feature)
endif()
```

### Available Custom Targets

```bash
# Interactive configuration (ncurses TUI)
make menuconfig

# Alternative ncurses interface
make nconfig

# Update configuration with new Kconfig options
make oldconfig

# Save minimal configuration to defconfig file
make savedefconfig

# List available defconfig files
make list_defconfigs
```

## Configuration Flow

### Loading a Configuration

1. User runs `python3 build.py config <name>_defconfig`
2. Build script copies `configs/<name>_defconfig` to `.config`
3. CMake runs and includes `Kconfig.cmake`
4. `Kconfig.cmake` reads `.config` and generates:
   - `autoconf.h` with C preprocessor defines
   - `kconfig.cmake` with CMake cache variables
5. All CONFIG_* symbols available in both C and CMake

### Generated autoconf.h Example

```c
/* Auto-generated from .config - DO NOT EDIT */
#ifndef AUTOCONF_H
#define AUTOCONF_H

#define CONFIG_OSAL 1
#define CONFIG_HAL 1
#define CONFIG_PDL 1
#define CONFIG_ARCH_X86_64 1
#define CONFIG_BUILD_TESTING 1
#define CONFIG_MAX_THREADS 32

#endif /* AUTOCONF_H */
```

### Generated kconfig.cmake Example

```cmake
# Auto-generated from .config - DO NOT EDIT

set(CONFIG_OSAL "ON" CACHE INTERNAL "Kconfig: OSAL")
set(CONFIG_HAL "ON" CACHE INTERNAL "Kconfig: HAL")
set(CONFIG_PDL "ON" CACHE INTERNAL "Kconfig: PDL")
set(CONFIG_ARCH_X86_64 "ON" CACHE INTERNAL "Kconfig: ARCH_X86_64")
set(CONFIG_BUILD_TESTING "ON" CACHE INTERNAL "Kconfig: BUILD_TESTING")
set(CONFIG_MAX_THREADS "32" CACHE INTERNAL "Kconfig: MAX_THREADS")
```

## Kconfig Value Types

| Kconfig Type | .config Format | autoconf.h | kconfig.cmake |
|--------------|----------------|------------|---------------|
| bool (yes)   | `CONFIG_FOO=y` | `#define CONFIG_FOO 1` | `set(CONFIG_FOO "ON")` |
| bool (no)    | `# CONFIG_FOO is not set` | (not defined) | `set(CONFIG_FOO OFF)` |
| string       | `CONFIG_NAME="value"` | `#define CONFIG_NAME "value"` | `set(CONFIG_NAME "value")` |
| int          | `CONFIG_COUNT=42` | `#define CONFIG_COUNT 42` | `set(CONFIG_COUNT "42")` |
| hex          | `CONFIG_ADDR=0x1000` | `#define CONFIG_ADDR 0x1000` | `set(CONFIG_ADDR "0x1000")` |

## Using CONFIG_* in Code

### In C Code

```c
#include <stdio.h>
/* autoconf.h is auto-included via -include compiler flag */

void example(void) {
#ifdef CONFIG_DEBUG_ENABLED
    printf("Debug mode enabled\n");
#endif

#if CONFIG_MAX_THREADS > 16
    printf("High thread count: %d\n", CONFIG_MAX_THREADS);
#endif

    printf("Platform: " CONFIG_PLATFORM_NAME "\n");
}
```

### In CMakeLists.txt

```cmake
# Conditional compilation
if(CONFIG_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Feature-based targets
if(CONFIG_HAL_CAN)
    target_sources(hal PRIVATE hal_can.c)
endif()

# Using string values
message(STATUS "Building for platform: ${CONFIG_PLATFORM_NAME}")

# Using numeric values
if(CONFIG_MAX_THREADS GREATER 32)
    target_compile_definitions(myapp PRIVATE HIGH_CONCURRENCY=1)
endif()
```

## Dependency Tracking

CMake automatically reconfigures when:
- `.config` file changes (modified by menuconfig/nconfig)
- Any `Kconfig` file changes
- Defconfig loaded via `python3 build.py config`

This ensures build is always in sync with configuration.

## Implementation Details

### Why Manual autoconf.h Generation?

The native `conf --syncconfig` command expects Buildroot's directory structure:
- `include/generated/autoconf.h`
- `include/config/*.h` (dependency tracking files)

Since ES-Middleware uses custom paths (`_build/config/autoconf.h`), `--syncconfig` causes segmentation faults. We generate `autoconf.h` manually by parsing `.config` in CMake, which is more reliable and portable.

### Parser Files (No flex/bison Required)

The Kconfig tools use pre-generated parser files (`zconf.tab.c_shipped`, `zconf.lex.c_shipped`) extracted from Buildroot. This means:
- **No flex/bison required** at build time
- Faster configuration (no parser regeneration)
- Identical behavior to Buildroot

If you want to regenerate parsers (advanced use):
```bash
cd scripts/kconfig/build
cmake -DKCONFIG_REGENERATE_PARSERS=ON ..
make
```

### Tool Chain

```
User Input → Tool → Output
─────────────────────────────────────────
menuconfig → mconf → .config (modified interactively)
defconfig  → conf  → .config (loaded from file)
.config    → CMake → autoconf.h + kconfig.cmake
```

## Troubleshooting

### menuconfig not available

**Symptom**: `make menuconfig` fails or target doesn't exist

**Cause**: ncurses development libraries not installed

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libncurses-dev

# Fedora/RHEL
sudo dnf install ncurses-devel

# Reconfigure
rm -rf _build
python3 build.py config <defconfig>
```

### autoconf.h not generated

**Symptom**: `Configuration header not found: autoconf.h`

**Cause**: .config doesn't exist yet

**Solution**:
```bash
python3 build.py config ccm_h200_100p_am625_debug_defconfig
```

### CONFIG_* variable not found in CMake

**Symptom**: `CONFIG_MY_FEATURE` is empty in CMakeLists.txt

**Cause**: Variable not set in .config, or CMake cache stale

**Solution**:
```bash
# Check if variable exists in .config
grep MY_FEATURE .config

# Force reconfigure
rm -rf _build
cmake -B _build
```

### Changes in menuconfig not taking effect

**Symptom**: Modified config in menuconfig but build unchanged

**Cause**: CMake cache not updated

**Solution**:
```bash
# After menuconfig, reconfigure CMake
make menuconfig
cmake -B _build  # Or: python3 build.py build (it auto-reconfigures)
```

## Best Practices

### 1. Always Use Defconfigs

Store minimal configurations in `configs/*_defconfig`:
```bash
# Save current config to defconfig
make savedefconfig
mv defconfig configs/my_new_defconfig
```

### 2. Don't Commit .config

The `.config` file is generated and should not be committed:
```gitignore
.config
.config.old
```

### 3. Use Namespaced Config Names

Prefix your symbols to avoid conflicts:
```kconfig
config MYMODULE_FEATURE
    bool "Enable my feature"

config MYMODULE_BUFFER_SIZE
    int "Buffer size"
    default 1024
```

### 4. Document Configuration Options

Add help text to all options:
```kconfig
config MY_FEATURE
    bool "Enable my feature"
    help
      This enables support for my feature.
      
      If unsure, say N.
```

### 5. Check Configuration in CI

```bash
#!/bin/bash
# Verify all defconfigs load successfully
for cfg in configs/*_defconfig; do
    echo "Testing $cfg..."
    python3 build.py config $(basename $cfg)
    if [ $? -ne 0 ]; then
        echo "Failed to load $cfg"
        exit 1
    fi
done
```

## Advanced Features

### Conditional Compilation

```kconfig
# In Kconfig
config FEATURE_A
    bool "Feature A"

config FEATURE_B
    bool "Feature B"
    depends on FEATURE_A
```

```cmake
# In CMakeLists.txt
if(CONFIG_FEATURE_B)
    # FEATURE_B implies FEATURE_A is also enabled
    add_executable(myapp main.c feature_a.c feature_b.c)
else()
    add_executable(myapp main.c)
endif()
```

### Range Validation

```kconfig
config THREAD_COUNT
    int "Number of threads"
    range 1 128
    default 4
```

### Choice Menus

```kconfig
choice
    prompt "Platform selection"
    default PLATFORM_AM625

config PLATFORM_AM625
    bool "TI AM625"

config PLATFORM_AM62L
    bool "TI AM62L"

endchoice

config PLATFORM_NAME
    string
    default "am625" if PLATFORM_AM625
    default "am62l" if PLATFORM_AM62L
```

## References

- [Kconfig Language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
- [Buildroot Manual - Kconfig](https://buildroot.org/downloads/manual/manual.html#_infrastructure_for_kconfig_based_configuration_files)
- [Linux Kernel Kconfig](https://www.kernel.org/doc/html/latest/kbuild/kconfig.html)

## Version History

- **2026-06-11**: Initial integration with native Buildroot tools
  - Built conf, mconf, nconf from extracted sources
  - Manual autoconf.h generation (--syncconfig workaround)
  - Full CMake integration with custom targets
  - Dependency tracking and auto-reconfiguration

---

**Maintained by**: ES-Middleware Team  
**Last Updated**: 2026-06-11
