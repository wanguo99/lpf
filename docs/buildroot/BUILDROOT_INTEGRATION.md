# ES-Middleware Buildroot Integration

This directory contains all necessary files to integrate ES-Middleware into Buildroot build system.

## Overview

ES-Middleware is packaged as a Buildroot external package that can be integrated into the CSPD project's `buildroot-external` tree. It uses Kconfig + CMake build system with zero Python dependency.

## Files

- **es-middleware.mk** - Buildroot package makefile (build rules)
- **Config.in** - Kconfig configuration options for menuconfig
- **S90es-middleware** - SysV init script for PMC services
- **local.mk.example** - Example configuration for local development
- **README.md** - This file

## Build Directory Structure

ES-Middleware uses different build directories depending on the build context:

### Standalone Build (Development)
```bash
ES-Middleware/
├── _build/              # Default build output (not committed to git)
│   ├── bin/            # Executables
│   ├── lib/            # Libraries
│   └── ...
├── .config             # Kconfig configuration
└── include/
    └── generated/
        ├── autoconf.h  # Generated configuration header
        └── version.h   # Generated version header
```

### Buildroot Integration (Production)
```bash
output/build/es-middleware-v1.0.0/
├── _build/             # Build output (in-tree build)
│   ├── bin/
│   ├── lib/
│   └── ...
├── .config
├── core/
├── products/
└── ...
```

**Key Points:**
- Standalone: Uses `_build/` in project root
- Buildroot: Uses `_build/` in Buildroot's build directory (`output/build/es-middleware-*/_build`)
- Both use in-tree builds for simplicity
- No separate `build-output` directory needed

## Source Configuration

### Default: Git Repository

By default, ES-Middleware is fetched from the git repository:

```makefile
ES_MIDDLEWARE_VERSION = v1.0.0
ES_MIDDLEWARE_SITE = ssh://gitea@192.168.18.254:4022/CSPD/ES-Middleware.git
ES_MIDDLEWARE_SITE_METHOD = git
```

This is suitable for:
- Production builds
- CI/CD pipelines
- Release builds

### Local Development: local.mk Override

For local development and testing, use `local.mk` to override the source:

```bash
# 1. Copy the example file
cd buildroot-external/package/es-middleware/
cp local.mk.example local.mk

# 2. Edit local.mk to point to your local source
vim local.mk

# Update this line:
ES_MIDDLEWARE_OVERRIDE_SRCDIR = $(TOPDIR)/../ES-Middleware

# 3. Rebuild the package
cd /path/to/buildroot
make es-middleware-rebuild
```

**Benefits of local.mk:**
- Rapid iteration without git commits
- Test changes immediately
- Work offline
- Debug integration issues

**Note:** `local.mk` is git-ignored to prevent accidental commits.

## Integration Steps

### 1. Register Package in Buildroot

Edit `buildroot-external/Config.in` and add:

```kconfig
menu "CSPD Packages"
    source "$BR2_EXTERNAL_CSPD_PATH/package/es-middleware/Config.in"
endmenu
```

### 2. Configure Buildroot

```bash
cd /path/to/buildroot
make menuconfig

# Navigate to: Target packages -> CSPD Packages -> ES-Middleware
# Enable: BR2_PACKAGE_ES_MIDDLEWARE=y
# Select defconfig: pmc_h200_100p_am625_release_defconfig
# Set build type: Release
```

### 3. Build

```bash
# Full build
make es-middleware

# Clean and rebuild
make es-middleware-rebuild

# Clean only
make es-middleware-dirclean
```

## Configuration Options

Available in `make menuconfig` under "ES-Middleware":

### Package Options
- **BR2_PACKAGE_ES_MIDDLEWARE** - Enable ES-Middleware package
- **BR2_PACKAGE_ES_MIDDLEWARE_DEFCONFIG** - Select Kconfig defconfig (21 available)
- **BR2_PACKAGE_ES_MIDDLEWARE_BUILD_TYPE** - Build type (Debug/Release/RelWithDebInfo)
- **BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_HEADERS** - Install development headers to staging (default: n)
- **BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_INIT** - Install SysV init scripts (default: n)
- **BR2_PACKAGE_ES_MIDDLEWARE_MENUCONFIG** - Enable Kconfig menuconfig support

### Default Configuration

The default configuration is `pmc_h200_100p_am625_release_defconfig`, which provides:
- All PMC applications (collector, comm, health, logger, supervisor)
- Complete middleware stack (OSAL, HAL, PCONFIG, ACONFIG, PDL, PRL)
- Optimized for production deployment
- Release build with minimal debugging overhead

For development and testing, use `pmc_h200_100p_am625_debug_defconfig` or one of the test configurations.

### Available Defconfigs (21 total)

#### PMC Product Configurations (2)
Production configurations for PMC board:
- `pmc_h200_100p_am625_release_defconfig` - **Recommended for production** (optimized)
- `pmc_h200_100p_am625_debug_defconfig` - Debug build for development

#### Test Configurations - ARM64 (10)
For ARM64 cross-compilation testing:
- `tests_arm64_full_defconfig` - All modules, all tests
- `tests_arm64_minimal_defconfig` - Minimal configuration
- `tests_arm64_osal_defconfig` - OSAL only
- `tests_arm64_hal_defconfig` - HAL layer testing
- `tests_arm64_pconfig_defconfig` - Platform configuration
- `tests_arm64_aconfig_defconfig` - Application configuration
- `tests_arm64_pdl_defconfig` - Peripheral driver layer
- `tests_arm64_prl_defconfig` - Protocol layer
- `tests_arm64_system_defconfig` - System integration tests
- `tests_arm64_stress_defconfig` - Stress tests

#### Test Configurations - x86_64 (9)
For x86_64 development and testing:
- `tests_x86_full_defconfig` - All modules, all tests
- `tests_x86_minimal_defconfig` - Minimal configuration
- `tests_x86_osal_defconfig` - OSAL only
- `tests_x86_pconfig_defconfig` - Platform configuration
- `tests_x86_aconfig_defconfig` - Application configuration
- `tests_x86_pdl_defconfig` - Peripheral driver layer
- `tests_x86_prl_defconfig` - Protocol layer
- `tests_x86_system_defconfig` - System integration tests
- `tests_x86_stress_defconfig` - Stress tests

**Note:** Defconfig names are used directly without directory prefix (e.g., `pmc_h200_100p_am625_release_defconfig`, not `pmc/pmc_h200_100p_am625_release_defconfig`).

## Build Process

The build follows Buildroot's standard workflow:

1. **Extract/Clone**: 
   - Git: Clones from repository to `output/build/es-middleware-v1.0.0/`
   - Local: Uses `ES_MIDDLEWARE_OVERRIDE_SRCDIR` path

2. **Configure**: 
   ```bash
   make <defconfig>           # e.g., make pmc_h200_100p_am625_release_defconfig
   # Generates: .config, include/generated/autoconf.h, include/generated/version.h
   ```

3. **Build**: 
   ```bash
   make all                   # Builds in _build/ directory
   # Uses CMAKE_TOOLCHAIN_FILE for cross-compilation
   ```

4. **Install**: 
   ```bash
   make install DESTDIR=$(TARGET_DIR)
   # Installs to target filesystem under /usr
   ```

**Cross-Compilation:**
- Automatically uses Buildroot's toolchain via `CMAKE_TOOLCHAIN_FILE`
- Respects `CMAKE_BUILD_TYPE` from Buildroot configuration
- Supports all Buildroot target architectures

## Installation Layout

After `make es-middleware`, files are installed to target filesystem:

### Target Directory (Runtime)
```
$(TARGET_DIR)/
├── usr/
│   ├── bin/
│   │   ├── pmc_collector          # PMC applications
│   │   ├── pmc_comm
│   │   ├── pmc_health
│   │   ├── pmc_logger
│   │   ├── pmc_supervisor
│   │   └── es-middleware-test     # Test runner (if enabled)
│   └── lib/
│       ├── libosal.a              # Static libraries
│       ├── libhal.a
│       ├── libpconfig.a
│       ├── libaconfig.a
│       ├── libpdl.a
│       └── libprl.a
└── etc/
    └── init.d/
        └── S90es-middleware       # Init script (if enabled)
```

### Staging Directory (Development, optional)

When `BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_HEADERS=y`:

```
$(STAGING_DIR)/
└── usr/
    ├── include/
    │   ├── osal.h                 # OSAL headers
    │   ├── hal_can.h              # HAL headers
    │   ├── hal_uart.h
    │   ├── pconfig.h              # PCONFIG headers
    │   ├── aconfig.h              # ACONFIG headers
    │   ├── pdl.h                  # PDL headers
    │   └── prl.h                  # PRL headers
    └── lib/
        └── (same as target)       # Static libraries
```

**Note**: 
- ES-Middleware uses static libraries by default for embedded systems.
- Headers are only installed to staging when explicitly enabled.
- Staging installation is needed if other packages depend on ES-Middleware APIs.

## Init Scripts

When `BR2_PACKAGE_ES_MIDDLEWARE_INSTALL_INIT=y`:

- Installs `/etc/init.d/S90es-middleware`
- Auto-starts PMC applications on boot
- Manages service lifecycle

## Troubleshooting

### Build fails with "No rule to make target"

Check that ES-Middleware source is available:
- For git: Verify network connectivity and repository URL
- For local.mk: Verify `ES_MIDDLEWARE_OVERRIDE_SRCDIR` path

### Configuration not applied

Rebuild with clean:
```bash
make es-middleware-dirclean
make es-middleware-rebuild
```

### Cross-compilation errors

Verify toolchain is configured:
```bash
# Check toolchain file
ls $(HOST_DIR)/share/buildroot/toolchainfile.cmake

# Verify target architecture
cat $(TARGET_DIR)/etc/os-release
```

### Wrong defconfig loaded

Verify the defconfig name in Buildroot menuconfig:
```bash
make menuconfig
# Navigate to: Target packages -> CSPD Packages -> ES-Middleware
# Check: "Kconfig defconfig to use" field
```

The defconfig name should NOT include the directory prefix:
- ✓ Correct: `pmc_h200_100p_am625_release_defconfig`
- ✗ Wrong: `pmc/pmc_h200_100p_am625_release_defconfig`

## Development Workflow

### Quick Iteration with local.mk

```bash
# 1. Make changes in ES-Middleware source
vim /path/to/ES-Middleware/core/osal/src/osal.c

# 2. Rebuild (uses local source via local.mk)
cd /path/to/buildroot
make es-middleware-rebuild

# 3. Test on target
# Files are automatically installed to target filesystem
```

### Switching Back to Git

```bash
# Remove local.mk override
rm buildroot-external/package/es-middleware/local.mk

# Clean rebuild from git
make es-middleware-dirclean
make es-middleware
```

## Dependencies

- **host-pkgconf** - Required for pkg-config during build
- **host-cmake** - CMake build system
- **host-kconfig-frontends** - Optional, for menuconfig support

**Zero Python dependency** - Uses native Kconfig + CMake tools only.

## Version Management

### Git Builds
Version is determined by `ES_MIDDLEWARE_VERSION` in `es-middleware.mk`:
```makefile
ES_MIDDLEWARE_VERSION = v1.0.0  # Git tag, branch, or commit
```

### Local Builds
Version can be overridden in `local.mk`:
```makefile
ES_MIDDLEWARE_VERSION = 1.0.0-local-$(shell date +%Y%m%d)
```

## License

Proprietary - See ES-Middleware LICENSE file.
