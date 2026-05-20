# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**EMS** (Embedded Middleware System) is a general-purpose embedded middleware framework providing hardware abstraction and peripheral management for embedded controllers. It uses a 3-layer call chain (Apps→PDL→HAL) running on OSAL runtime environment, with 2 configuration libraries (ACL/PCL), designed for cross-platform portability (Linux/RTOS).

**Build System**: Linux kernel-style Makefile + Kconfig configuration system for flexible, modular builds.

**Typical Application**:
```
External System <--CAN--> EMS Controller <--Ethernet/UART/CAN--> Device Modules
```

**Key Products**:
- **CCM** (Communication Controller Management) - formerly PMC, manages communication between external systems and device modules

## Quick Commands

### Build

**EMS uses Linux kernel-style Kconfig + Makefile build system**

```bash
# First-time setup: Configure the build
make menuconfig                              # Interactive configuration (ncurses)
# OR use a preset configuration
make ccm_h200_am625_release_defconfig       # Release preset
make ccm_h200_am625_debug_defconfig         # Debug preset

# Build everything
make -j$(nproc)                             # Build all modules

# Build specific targets
make core -j$(nproc)                        # Build core modules only
make products -j$(nproc)                    # Build product modules only

# Out-of-tree build (keeps source clean)
make O=/tmp/build defconfig                 # Configure in separate directory
make O=/tmp/build -j$(nproc)                # Build in separate directory

# Install
make install INSTALL_PREFIX=/opt/ems        # Install libs and binaries
make headers_install INSTALL_PREFIX=/opt/ems # Install headers only
make install-all INSTALL_PREFIX=/opt/ems    # Install everything

# Clean targets
make clean                                  # Remove build artifacts
make distclean                              # Remove build and config
make mrproper                               # Full clean including tools
```

**Configuration commands**:
```bash
make menuconfig          # Interactive menu-based configuration
make defconfig           # Load default configuration
make savedefconfig       # Save minimal config to defconfig
make oldconfig           # Update config for new options
make syncconfig          # Synchronize and generate headers
```

**Available presets** (in `configs/`):
- `ccm_h200_am625_release_defconfig` - CCM H200 AM625 release build
- `ccm_h200_am625_debug_defconfig` - CCM H200 AM625 debug build

**Output locations**:
- Binaries: `bin/` (or `$OUTPUT_DIR/bin/` for out-of-tree builds)
- Libraries: `lib/` (or `$OUTPUT_DIR/lib/` for out-of-tree builds)
- Configuration: `.config` (Kconfig configuration file)
- Generated headers: `include/generated/autoconf.h`
- Exported API headers: `include/{osal,hal,pdl,pcl,acl}/`

### Test

```bash
# Run tests
./bin/ems-test -i    # Interactive menu (recommended)
./bin/ems-test -a    # Run all tests
./bin/ems-test -L OSAL  # Run OSAL layer tests
./bin/ems-test -m test_osal_task  # Run specific module (without .c extension)

# Busybox-style shortcuts (via symlinks)
./bin/osal-test -a    # Run only OSAL tests
./bin/hal-test -a     # Run only HAL tests
```

### Debug

```bash
# Build with debug configuration
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# Run with GDB
gdb ./bin/sample_app
gdb --args ./bin/ems-test -m test_osal_task
```

## Architecture Design

```
┌─────────────────────────────────────────────────────────────────┐
│                      OSAL Runtime Environment                    │
│  • Shields platform differences (Linux, 32/64-bit, syscalls)    │
│  • Provides unified interfaces (thread/IPC/memory/time/file/net) │
│  • Platform implementation (POSIX)                               │
│  • ALL layers use OSAL interfaces (no direct syscalls/stdlib)   │
└─────────────────────────────────────────────────────────────────┘
         ↑                    ↑                    ↑
         │ Use OSAL          │ Use OSAL           │ Use OSAL
         │                    │                    │
  ┌──────┴──────┐      ┌─────┴──────┐      ┌─────┴──────┐
  │    Apps     │─────>│    PDL     │─────>│    HAL     │
  │             │      │            │      │            │
  │  Business   │      │ Peripheral │      │  Hardware  │
  │   Logic     │      │   Driver   │      │   Driver   │
  └──────┬──────┘      └─────┬──────┘      └────────────┘
         │                    │
         │ Read Config        │ Read Config
         ↓                    ↓
  ┌─────────────┐      ┌─────────────┐
  │     ACL     │      │     PCL     │
  │  (Config)   │      │  (Config)   │
  │  Business   │      │  Hardware   │
  │   Mapping   │      │   Config    │
  └─────────────┘      └─────────────┘
```

**Call Chain** (3 layers):
```
Apps → PDL → HAL
```

**OSAL Runtime Environment**:
```
All layers (Apps/PDL/HAL) use OSAL interfaces
OSAL → Linux System Calls
```

**Configuration Libraries** (not in call chain):
```
ACL - Read by Apps layer (business function to device mapping)
PCL - Read by PDL layer (hardware configuration)
```

**CRITICAL RULE**: All layers (Apps/PDL/HAL) MUST use OSAL wrappers. Only OSAL can call system APIs and standard library.

### Component Responsibilities

**OSAL** (Operating System Abstraction Layer) - **Runtime Environment for All Layers**
- **Only layer allowed to include system headers** (`<unistd.h>`, `<pthread.h>`, `<stdlib.h>`)
- **Only layer allowed to call system APIs and standard library functions**
- Wraps ALL system calls and standard library for upper layers
- Provides: task management, queues, mutexes, logging, file I/O, networking, time services, memory operations (`OSAL_Memcpy`, `OSAL_Strlen`), string operations
- Platform-specific code in `core/osal/src/posix/` (future: `freertos/`, `vxworks/`)
- **All other layers (Apps/PDL/HAL) MUST use OSAL wrappers**

**HAL** (Hardware Abstraction Layer)
- **Only layer allowed to include hardware-specific headers** (`<linux/can.h>`, `<net/if.h>`)
- **MUST use OSAL wrappers for ALL operations** (never direct `socket()`, `open()`, `memcpy()`, `strlen()`, etc.)
- Provides: CAN, UART, I2C, SPI, GPIO, Watchdog drivers
- Platform-specific code in `core/hal/src/linux/` (future: `ti_am62/`, `nxp_imx8/`)

**PDL** (Peripheral Driver Layer)
- Unified management of satellite/BMC/MCU peripherals
- **Must be completely platform-independent**
- **MUST use OSAL wrappers for ALL operations**
- Reads hardware configuration from PCL
- Provides application-facing peripheral interfaces
- Only Apps and Tests layers can access PDL APIs

**ACL** (Application Configuration Layer) - **Configuration Library**
- **Not in call chain, read by Apps layer**
- Maps business functions (e.g., "server power on") to device types and indexes
- **O(1) lookup performance** using enum-indexed arrays
- Configuration files in `core/acl/config/<product>/`
- Pure data structures, no business logic

**PCL** (Peripheral Configuration Library) - **Configuration Library**
- **Not in call chain, read by PDL layer**
- Device-tree-like hardware configuration (pure data structures)
- **Must be completely platform-independent**
- Configuration organized by peripheral: `core/pcl/platform/<vendor>/<chip>/<product>/`
- Pure data structures, no business logic

**Apps** (Application Layer)
- **Must be completely platform-independent**
- **MUST use OSAL wrappers for ALL operations**
- Reads business configuration from ACL
- Currently contains sample_app (reference implementation) and watchdog_app (watchdog demonstration)

## Critical Rules

### Platform Independence (MOST IMPORTANT)

**System Call Encapsulation**:
- ❌ **FORBIDDEN** in HAL/PCL/PDL/Apps/Tests: Direct use of `socket()`, `bind()`, `open()`, `close()`, `memcpy()`, `strlen()`, `printf()`, etc.
- ✅ **REQUIRED**: Use OSAL wrappers: `OSAL_socket()`, `OSAL_open()`, `OSAL_Memcpy()`, `OSAL_Strlen()`, `LOG_INFO()`, etc.

**Header File Rules**:
- ✅ OSAL layer: Can include system headers (`<unistd.h>`, `<sys/socket.h>`, `<pthread.h>`, `<stdlib.h>`)
- ✅ HAL layer: Can include hardware headers, but must use OSAL system call wrappers
- ❌ PCL/PDL/Apps/Tests: **Strictly forbidden** to include any system headers

**Example**:
```c
/* ❌ WRONG: HAL layer using direct system calls */
int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);  // FORBIDDEN
bind(sockfd, ...);  // FORBIDDEN

/* ✅ CORRECT: HAL layer using OSAL wrappers */
int32 sockfd = OSAL_socket(PF_CAN, SOCK_RAW, CAN_RAW);  // CORRECT
OSAL_bind(sockfd, ...);  // CORRECT
```

### Data Types (from core/osal/include/osal_types.h)

**CRITICAL: This project strictly prohibits using C native data types. You MUST use OSAL-wrapped types.**

```c
/* String type - ALLOWED */
char                           // For strings and text data ONLY

/* Fixed-size integers */
int8, int16, int32, int64       // Signed integers
uint8, uint16, uint32, uint64   // Unsigned integers

/* Boolean */
bool                            // true/false

/* Platform-dependent size types */
osal_size_t                     // Unsigned size (uint32 on 32-bit, uint64 on 64-bit)
osal_ssize_t                    // Signed size (int32 on 32-bit, int64 on 64-bit)

/* OSAL types */
osal_id_t                       // Object ID (uint32)
```

**Prohibited Native Types**:
```c
/* ❌ NEVER USE THESE */
int                 // Use int32 or int16
unsigned int        // Use uint32 or uint16
short               // Use int16
unsigned short      // Use uint16
long                // Use int32 or int64
unsigned long       // Use uint32 or uint64
long long           // Use int64
unsigned long long  // Use uint64
size_t              // Use osal_size_t
ssize_t             // Use osal_ssize_t
```

**Exceptions** (only in these cases):
1. **OSAL layer internal implementation** - when wrapping system calls
2. **Third-party library interaction** - when calling external library APIs
3. **Must document the reason in comments**

### Naming Conventions

**Public APIs** (exported to upper layers):
```c
OSAL_TaskCreate()           // OSAL layer
HAL_CANInit()               // HAL layer
PCL_Init()                  // PCL layer
PDL_SatelliteInit()         // PDL layer
```

**Internal functions** (static, module-private):
```c
static int32 osal_task_find_by_id(osal_id_t id);
static void hal_can_set_filter(int fd, uint32 filter_id);
static int32 pcl_find_config(const char *name);
```

**Test cases** (all lowercase with underscores):
```c
TEST_CASE(test_osal_task_create_success)
TEST_CASE(test_hal_can_init_null_handle)
TEST_CASE(test_pcl_api_register_success)
```

### Error Handling

- All functions return `int32` status codes
- Success: `OS_SUCCESS` (0)
- Failure: `OS_ERROR` (-1) or specific error code
- **All return values must be checked**

```c
int32 ret = OSAL_TaskCreate(&task_id, "task", entry, NULL);
if (ret != OS_SUCCESS) {
    LOG_ERROR("MODULE", "Task creation failed");
    return OS_ERROR;
}
```

### Logging

**Use OSAL logging interfaces** (never `printf`/`fprintf`):
```c
LOG_INFO("MODULE", "message");      // Recommended
LOG_ERROR("MODULE", "error: %d", errno);
OSAL_Printf("simple output\n");     // Only when necessary
```

### Task Programming

**Standard task template**:
```c
static void task_entry(void *arg)
{
    osal_id_t task_id = OSAL_TaskGetId();
    
    while (!OSAL_TaskShouldShutdown())  // Check shutdown flag
    {
        do_work();
        OSAL_TaskDelay(100);
    }
    
    cleanup();  // Clean up resources before exit
}
```

**Critical**: Never use `while(1)` - tasks must check `OSAL_TaskShouldShutdown()` for graceful shutdown.

## Kconfig Configuration System

EMS uses Linux kernel-style Kconfig for build configuration:

**Configuration workflow**:
1. Run `make menuconfig` to configure build options interactively
2. Configuration is saved to `.config` file
3. Build system generates `include/generated/autoconf.h` with C macros
4. Code uses `#ifdef CONFIG_*` to conditionally compile features

**Module dependencies** (enforced by Kconfig):
```
OSAL (foundation)
  ↓
HAL (depends on OSAL)
  ↓
PCL (depends on OSAL, HAL)
  ↓
PDL (depends on OSAL, HAL, PCL)
  ↓
ACL (depends on OSAL, PDL)
```

**Key configuration options**:
- Core modules: OSAL, HAL, PDL, PCL, ACL (with dependency checking)
- HAL drivers: CAN, UART, I2C, SPI, GPIO, Watchdog (conditional compilation)
- PDL services: Satellite, BMC, MCU, Watchdog (conditional compilation)
- Product selection: CCM (Communication Controller Management), samples
- Debug options: logging levels, assertions, debug symbols

**Configuration files**:
- `.config` - Current configuration (generated, do not commit)
- `defconfig` - Minimal default configuration
- `configs/*_defconfig` - Preset configurations for specific products
- `Kconfig` - Configuration menu definitions (hierarchical)

## Header File Structure

EMS uses a three-tier header file structure similar to Linux kernel:

**1. Module internal headers** (`core/*/include/`):
- Used during module development
- Contains internal implementation details
- Not exported to other modules

**2. Top-level exported headers** (`include/*/`):
- Used for cross-module references
- Contains only public APIs
- Organized by module: `include/{osal,hal,pdl,pcl,acl}/`
- Generated by `make headers_install` in each module

**3. System installed headers** (`/usr/local/include/`):
- Used by external applications
- Installed via `make headers_install INSTALL_PREFIX=/usr/local`

**Header inclusion rules**:
```c
// Within module (internal headers)
#include "osal_internal.h"

// Cross-module (use top-level include/)
#include <osal/osal.h>
#include <hal/hal_can.h>

// Use generated config
#include <generated/autoconf.h>
#ifdef CONFIG_HAL_CAN
    // CAN-specific code
#endif
```

## Buildroot Integration

EMS supports Buildroot integration for embedded Linux systems:

```bash
# 1. Copy configuration files to Buildroot
cp -r docs/buildroot/* <buildroot>/package/ems/

# 2. Enable EMS in menuconfig
make menuconfig
# Navigate to: Target packages -> Libraries -> ems

# 3. Build
make ems
```

Detailed integration guide: [docs/buildroot/README.md](docs/buildroot/README.md)

## Development Workflows

### First-Time Setup
1. Configure the build: `make menuconfig` or `make ccm_h200_am625_release_defconfig`
2. Build: `make -j$(nproc)`
3. Run tests: `./bin/ems-test -i`

### Daily Development Workflow
1. Edit source files
2. Run `make -j$(nproc)` (incremental build, only rebuilds changed files)
3. Test changes: `./bin/ems-test -m <module_name>`
4. Commit changes with Chinese commit message format

### Changing Build Configuration
1. Run `make menuconfig` to change options
2. Configuration saved to `.config`
3. Run `make -j$(nproc)` to rebuild with new config
4. To save as preset: `make savedefconfig` (saves to `defconfig`)

### Adding New OSAL Interface
1. Add interface declaration in `core/osal/include/` (e.g., `osal_timer.h`)
2. Implement POSIX version in `core/osal/src/posix/` (e.g., `osal_timer.c`)
3. Add to `SRCS` in `core/osal/Makefile`
4. Add to `EXPORT_HEADERS` in `core/osal/Makefile` if it's a public API
5. Run `make -C core/osal headers_install` to export headers
6. Add unit tests in `tests/unit/osal/` (e.g., `test_osal_timer.c`)
7. Update `core/osal/README.md` if adding major functionality

### Adding New HAL Driver
1. Add driver interface in `core/hal/include/` (e.g., `hal_gpio.h`)
2. Add driver config in `core/hal/include/config/` (e.g., `hal_gpio_config.h`)
3. Add Kconfig option in `core/hal/Kconfig`:
   ```kconfig
   config HAL_GPIO
       bool "GPIO driver"
       depends on HAL
       default y
   ```
4. Implement Linux version in `core/hal/src/linux/` (using OSAL wrappers)
5. Add conditional compilation in `core/hal/Makefile`:
   ```makefile
   SRCS-$(CONFIG_HAL_GPIO) += hal_gpio_linux.c
   ```
6. Add to `EXPORT_HEADERS` in `core/hal/Makefile`
7. Add unit tests in `tests/unit/hal/`

### Adding New PDL Service
1. Add service interface in `core/pdl/include/` (e.g., `pdl_satellite.h`)
2. Add Kconfig option in `core/pdl/Kconfig` with proper dependencies
3. Implement service in `core/pdl/src/` (using HAL and PCL APIs)
4. Add conditional compilation in `core/pdl/Makefile`
5. Add to `EXPORT_HEADERS` in `core/pdl/Makefile`
6. Add unit tests in `tests/pdl/`
7. Update `core/pdl/README.md`

### Adding New Module
1. Create module directory: `core/newmodule/{include,src}/`
2. Create `core/newmodule/Kconfig` with dependencies:
   ```kconfig
   config NEWMODULE
       bool "New Module"
       depends on OSAL
       default y
   ```
3. Create `core/newmodule/Makefile` using generic framework:
   ```makefile
   MODULE_NAME := newmodule
   MODULE_TYPE := shared_lib
   SRCS := newmodule.c
   EXPORT_HEADERS := newmodule.h
   LDLIBS := -L$(objtree)/lib -losal
   include $(srctree)/scripts/Makefile.build
   ```
4. Add to `core/Makefile`: `MODULES := osal hal pcl pdl acl newmodule`
5. Add dependency: `newmodule: osal`
6. Source Kconfig in top-level `Kconfig`: `source "core/newmodule/Kconfig"`

### Adding New Platform Configuration
1. Create platform directory: `core/pcl/platform/<vendor>/<chip>/<product>/`
2. Add hardware config file (defines peripheral interfaces)
3. Add application config file (maps apps to peripherals)
4. Register platform in PCL initialization
5. Add platform-specific tests if needed

### Adding New Test
1. Create test file in `tests/unit/<layer>/` (e.g., `test_osal_timer.c`)
2. Use `TEST_MODULE_BEGIN/END` and `TEST_CASE` macros
3. Add source file to `tests/Makefile` in the appropriate section
4. Build and run: `make -j$(nproc) && ./bin/ems-test -m test_osal_timer`
5. Test module name should match filename without `.c` extension

**Test template**:
```c
#include "tests_core.h"
#include <osal.h>

TEST_CASE(test_osal_timer_create_success)
{
    osal_id_t timer_id;
    int32 ret = OSAL_TimerCreate(&timer_id, "test_timer", NULL, NULL);
    TEST_ASSERT_EQUAL(OS_SUCCESS, ret);
    OSAL_TimerDelete(timer_id);
}

TEST_MODULE_BEGIN(test_osal_timer, "Timer Tests")
    TEST_CASE_REGISTER(test_osal_timer_create_success, "Timer creation")
TEST_MODULE_END()
```

## Key Design Patterns

### Linux Kernel-Style Build System
- **Kconfig**: Menu-driven configuration with dependency checking (`depends on`)
- **Silent build**: Clean output with `CC`, `LD`, `AR` prefixes (use `V=1` for verbose)
- **Dependency tracking**: Automatic header dependency generation (`.d` files)
- **Out-of-tree builds**: Keep source tree clean with `O=<dir>` option
- **Incremental builds**: Only rebuild changed files and their dependents
- **Generic build framework**: `scripts/Makefile.build` - modules only define variables

### Generic Makefile Framework (Buildroot-style)
- **Unified build rules**: All modules use `scripts/Makefile.build`
- **Variable-based configuration**: Modules define `MODULE_NAME`, `SRCS`, `EXPORT_HEADERS`
- **Automatic processing**: Compilation, linking, dependency tracking, header export
- **Conditional compilation**: `SRCS-$(CONFIG_XXX) += file.c` pattern
- **Three module types**: `shared_lib`, `static_lib`, `executable`

### Three-Tier Header Structure
- **Module internal**: `core/*/include/` - development headers
- **Top-level export**: `include/*/` - cross-module API headers
- **System install**: `/usr/local/include/` - external application headers
- **Automatic export**: `make headers_install` copies from module to top-level

### OSAL User-Space Library Design
- No explicit initialization required (static initialization)
- No idle loop (OS-scheduled)
- Graceful shutdown via `OSAL_TaskShouldShutdown()`
- Thread-safe with deadlock detection

### PCL Configuration Architecture
- Two-layer config: Hardware config (defines peripheral interfaces) + App config (defines app-to-peripheral mapping)
- Peripheral-centric (like Linux device tree)
- Multi-platform support: `platform/<vendor>/<chip>/<product>/`
- Selection via environment variable / compile option / default

### PDL Peripheral Framework
- Unified peripheral interface (`peripheral_device.h`)
- Adapter pattern for legacy interfaces (100% backward compatible)
- Multi-channel redundancy with automatic failover
- Heartbeat mechanism for health monitoring

## Common Issues

### Compilation Warnings
- Project uses `-Werror` - all warnings are errors
- Must fix all warnings to compile
- C Standard: C99/C11 with `-D_POSIX_C_SOURCE=200809L`

### Multi-Architecture Support
- **Supported Architectures**: x86_64, ARM32 (ARMv7-A), ARM64 (ARMv8-A), RISC-V 64
- **Type System**: Uses fixed-size types (`uint32`, `int64`) for portability
- **Endianness**: Automatic detection with `OSAL_HTONS/HTONL` macros for byte order conversion
- **Atomic Operations**: Uses C11 `_Atomic` with fixed-size types (`_Atomic uint32`)
- **Pointer Casting**: Uses `uintptr_t` for safe pointer-to-integer conversions

### Cross-Compilation
```bash
# Cross-compilation support is configured via Kconfig
# Edit .config or use menuconfig to set target architecture

make menuconfig
# Navigate to: Build Options -> Target Architecture
# Select: ARM32, ARM64, RISC-V 64, or x86_64

# Then build normally
make -j$(nproc)
```

**Architecture-Specific Compiler Flags** (configured in Kconfig):
- ARM32: `-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard`
- ARM64: `-march=armv8-a`
- RISC-V 64: `-march=rv64imafdc -mabi=lp64d`

### Hardware-Related Test Failures
- CAN tests require `can0` or `vcan0` device
- Serial tests require `/dev/ttyS0` or `/dev/ttyUSB0`
- GPIO tests require `/sys/class/gpio` access
- Watchdog tests require `/dev/watchdog` device
- These failures are expected without hardware
- Use `-i` interactive mode to skip hardware-dependent tests

### CAN Interface Setup
```bash
# Check if CAN modules are loaded
lsmod | grep can

# Load CAN modules
sudo modprobe can can_raw vcan

# Setup virtual CAN (for testing without hardware)
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Setup real CAN interface
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

### Serial Port Permissions
```bash
# Add user to dialout group for serial port access
sudo usermod -a -G dialout $USER
# Log out and log back in for changes to take effect

# Or use sudo for testing
sudo ./build/release/bin/ems-test -m test_hal_serial
```

## Build Output Structure

```
# In-tree build (default)
.
├── bin/              # Executables (sample_app, watchdog_app, ems-test, osal-test, hal-test, etc.)
├── lib/              # Static libraries (libosal.a, libhal.a, libpdl.a, libpcl.a, libacl.a)
├── .config           # Kconfig configuration
└── include/
    ├── config/       # Kconfig internal files
    └── generated/
        └── autoconf.h  # Generated configuration header

# Out-of-tree build (with O=/path/to/build)
/path/to/build/
├── bin/              # Executables
├── lib/              # Libraries
├── .config           # Configuration
├── include/generated/autoconf.h
└── core/             # Object files
    └── products/
```

## Documentation

- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Build System**: [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) - Detailed build system documentation
- **Coding Standards**: [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md)
- **Quick Build Guide**: [docs/QUICK_BUILD.md](docs/QUICK_BUILD.md)
- **Module READMEs**: Each layer has `README.md` and `docs/` directory
- **Test Framework**: [tests/README.md](tests/README.md)

## Debugging and Troubleshooting

### Common Build Issues

**Kconfig tools not built**:
```bash
# Error: scripts/kconfig/conf: No such file or directory
# Solution: Kconfig tools are built automatically on first make
make menuconfig  # This will build the tools first
```

**Missing configuration**:
```bash
# Error: .config not found
# Solution: Run configuration first
make defconfig  # Or use a preset: make ccm_h200_am625_release_defconfig
```

**Out-of-sync configuration**:
```bash
# After pulling new code with new Kconfig options
make oldconfig  # Update config for new options
# Or regenerate from preset
make ccm_h200_am625_release_defconfig
```

### Debugging Tips

**Enable verbose logging**:
```c
OSAL_LogSetLevel(LOG_LEVEL_DEBUG);  // In your code
```

**GDB debugging**:
```bash
# Build with debug configuration
make ccm_h200_am625_debug_defconfig
make -j$(nproc)
gdb --args ./bin/ems-test -m test_osal_task
```

**Memory leak detection**:
```bash
valgrind --leak-check=full ./bin/ems-test -m test_osal_queue
```

**Check task status**:
```bash
# Monitor running tasks
ps -eLf | grep ems-test
```

## Project Stats

- **Code Size**: ~23,000 lines (13,000 production + 10,000 test)
- **Files**: 151 C/H files
- **Test Coverage**: 406 test cases across all layers
  - OSAL: 200 tests (19 modules)
  - HAL: 89 tests (6 modules: CAN, UART, I2C, SPI, GPIO, Watchdog)
  - PCL: 22 tests
  - PDL: 95 tests (Satellite, BMC, MCU, Watchdog)
  - ACL: Configuration validation and statistics
- **Architecture**: 3-layer call chain (Apps→PDL→HAL) + OSAL runtime + 2 config libraries (ACL/PCL)
- **Platforms**: TI AM6254 (CCM H200), vendor_demo (extensible)
- **Build System**: Linux kernel-style Makefile + Kconfig, supports native and cross-compilation

## Important Files

- [Makefile](Makefile) - Top-level build configuration (Linux kernel style)
- [Kconfig](Kconfig) - Configuration menu definitions
- [defconfig](defconfig) - Default configuration
- [configs/](configs/) - Preset configurations for products
- [scripts/kconfig/](scripts/kconfig/) - Kconfig tools (conf, mconf)
- [scripts/Makefile.lib](scripts/Makefile.lib) - Build system library (compiler flags, silent build)
- [scripts/Makefile.build](scripts/Makefile.build) - Generic build framework for modules
- [scripts/install.mk](scripts/install.mk) - Installation rules
- [include/](include/) - Top-level exported API headers (organized by module)
- [core/osal/include/osal.h](core/osal/include/osal.h) - OSAL main header
- [core/osal/include/osal_types.h](core/osal/include/osal_types.h) - Type definitions
- [tests/core/main.c](tests/core/main.c) - Test runner entry
- [products/ccm/](products/ccm/) - CCM (Communication Controller Management) product
- [products/samples/](products/samples/) - Sample applications
- [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) - Build system detailed documentation

## Performance Considerations

### Memory Management
- OSAL uses static allocation for task/queue/mutex tables (64 entries each)
- Queue buffers are dynamically allocated but pre-sized at creation
- No runtime memory allocation in critical paths (interrupt handlers, tight loops)

### Real-time Considerations
- Task priorities: 1-255 (lower number = higher priority)
- OSAL_TaskDelay() uses nanosleep() for precise timing
- Mutex deadlock detection threshold: 5000ms (configurable)
- Queue operations support timeout for bounded waiting

### Thread Safety
- All OSAL APIs are thread-safe
- HAL drivers use OSAL mutexes for protection
- Atomic operations use C11 `_Atomic` types
- Reference counting prevents use-after-free in queues

## Git Commit Message Convention

This project uses Chinese commit message format:

```
<类型>：<描述>

类型 (Types):
- 重构 (refactor): Code restructuring without changing functionality
- 文档 (docs): Documentation changes
- 修复 (fix): Bug fixes
- 新增 (feat): New features
- 测试 (test): Adding or updating tests
- 构建 (build): Build system changes
- 优化 (optimize): Performance or code quality improvements
```

**Examples**:
```
重构：移除 osal_task 模块，简化为基础线程接口
文档：完成 Buildroot 集成文件的 BSP→EMS 重命名
修复：替换不存在的OSAL_Strchr函数并修复测试编译
新增：实现Redfish协议支持
```
