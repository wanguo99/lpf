# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**EMS** (Embedded Middleware System) is a general-purpose embedded middleware framework providing hardware abstraction and peripheral management for embedded controllers. It uses a strict 5-layer architecture designed for cross-platform portability (Linux/RTOS).

**Typical Application**:
```
External System <--CAN--> EMS Controller <--Ethernet/UART/CAN--> Device Modules
```

## Quick Commands

### Build

**Standard CMake commands**
```bash
# Native build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Cross-compilation
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm32-linux-gnueabihf.cmake
cmake --build build -j$(nproc)

cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build -j$(nproc)

cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-linux-gnu.cmake
cmake --build build -j$(nproc)
```

**Alternative: build.sh wrapper**
```bash
./build.sh              # Release build
./build.sh -d           # Debug build
./build.sh -c           # Clean
./build.sh -a arm32     # ARM32 cross-compile
./build.sh -a arm64     # ARM64 cross-compile
./build.sh -a riscv64   # RISC-V 64 cross-compile
```

**Output locations**:
- Binaries: `build/bin/`
- Libraries: `build/lib/`

### Test

```bash
# Run tests
./build/bin/ems-test -i    # Interactive menu (recommended)
./build/bin/ems-test -a    # Run all tests
./build/bin/ems-test -L OSAL  # Run OSAL layer tests
./build/bin/ems-test -m test_osal_task  # Run specific module (without .c extension)

# Busybox-style shortcuts (via symlinks)
./build/bin/osal-test -a    # Run only OSAL tests
./build/bin/hal-test -a     # Run only HAL tests

# Fast iteration: rebuild single test layer
cd build && make osal_tests -j$(nproc) && cd ..
cd build && make hal_tests -j$(nproc) && cd ..
```

### Debug

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
gdb ./build/bin/sample_app
gdb --args ./build/bin/ems-test -m test_osal_task
```

## Architecture: 5-Layer Design

**Critical Dependency Chain**:
```
Apps → PDL → HAL → OSAL → Linux System Calls
         ↓
        PCL (configuration data)
```

### Layer Responsibilities

**OSAL** (Operating System Abstraction Layer)
- **Only layer allowed to include system headers** (`<unistd.h>`, `<pthread.h>`, `<stdlib.h>`)
- Wraps ALL system calls and standard library functions
- Provides: task management, queues, mutexes, logging, file I/O, networking, time services
- Platform-specific code in `osal/src/posix/` (future: `freertos/`, `vxworks/`)

**HAL** (Hardware Abstraction Layer)
- **Only layer allowed to include hardware-specific headers** (`<linux/can.h>`, `<net/if.h>`)
- **Must use OSAL wrappers for all system calls** (never direct `socket()`, `open()`, etc.)
- Provides: CAN, UART, I2C, SPI, GPIO, Watchdog drivers
- Platform-specific code in `hal/src/linux/` (future: `ti_am62/`, `nxp_imx8/`)

**PCL** (Peripheral Configuration Library)
- Device-tree-like hardware configuration (pure data structures)
- **Must be completely platform-independent**
- Configuration organized by peripheral: `platform/<vendor>/<chip>/<product>/`
- Only PDL layer accesses PCL

**PDL** (Peripheral Driver Layer)
- Unified management of satellite/BMC/MCU peripherals
- **Must be completely platform-independent**
- Provides application-facing peripheral interfaces
- Only Apps and Tests layers can access PDL APIs

**Apps** (Application Layer)
- **Must be completely platform-independent**
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

### Data Types (from osal/include/osal_types.h)

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

### Adding New OSAL Interface
1. Add interface declaration in `osal/include/` (e.g., `osal_timer.h`)
2. Implement POSIX version in `osal/src/posix/` (e.g., `osal_timer.c`)
3. Add unit tests in `tests/unit/osal/` (e.g., `test_osal_timer.c`)
4. Update `osal/CMakeLists.txt` to include new source file
5. Update `osal/README.md` if adding major functionality

### Adding New HAL Driver
1. Add driver interface in `hal/include/` (e.g., `hal_gpio.h`)
2. Add driver config in `hal/include/config/` (e.g., `hal_gpio_config.h`)
3. Implement Linux version in `hal/src/linux/` (using OSAL wrappers, e.g., `hal_gpio.c`)
4. Add unit tests in `tests/unit/hal/` (e.g., `test_hal_gpio.c`)
5. Update `hal/CMakeLists.txt` to include new driver source

### Adding New PDL Service
1. Add service interface in `pdl/include/` (e.g., `pdl_satellite.h`)
2. Implement service in `pdl/src/` (using HAL and PCL APIs)
3. Add unit tests in `tests/pdl/`
4. Update `pdl/README.md`

### Adding New Platform Configuration
1. Create platform directory: `pcl/platform/<vendor>/<chip>/<product>/`
2. Add hardware config file (defines peripheral interfaces)
3. Add application config file (maps apps to peripherals)
4. Register platform in PCL initialization
5. Add platform-specific tests if needed

### Adding New Test
1. Create test file in `tests/unit/<layer>/` (e.g., `test_osal_timer.c`)
2. Use `TEST_MODULE_BEGIN/END` and `TEST_CASE` macros
3. Add source file to `tests/CMakeLists.txt` in the appropriate section
4. Build and run: `cmake --build build && ./build/bin/ems-test -m test_osal_timer`
5. For fast iteration: `cd build && make osal_tests -j$(nproc) && cd ..`
6. Test module name should match filename without `.c` extension

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
# Install toolchains on Ubuntu/Debian:
sudo apt-get install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu gcc-riscv64-linux-gnu

# Cross-compile using toolchain files
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm32-linux-gnueabihf.cmake
cmake --build build -j$(nproc)

cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build -j$(nproc)

cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-linux-gnu.cmake
cmake --build build -j$(nproc)
```

**Architecture-Specific Compiler Flags**:
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
build/
├── bin/              # Executables (sample_app, watchdog_app, ems-test, osal-test, hal-test, etc.)
└── lib/              # Static libraries (libosal.a, libhal.a, libpdl.a, libpcl.a)
```

## Documentation

- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Coding Standards**: [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md)
- **Quick Build Guide**: [docs/QUICK_BUILD.md](docs/QUICK_BUILD.md)
- **Module READMEs**: Each layer has `README.md` and `docs/` directory
- **Test Framework**: [tests/README.md](tests/README.md)

## Debugging and Troubleshooting

### Common Build Issues

**Missing pthread library**:
```bash
# Error: undefined reference to `pthread_create`
# Solution: pthread is linked automatically via find_package(Threads)
```

**Cross-compilation toolchain not found**:
```bash
# Install missing toolchain
sudo apt-get install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu gcc-riscv64-linux-gnu
```

**Test failures without hardware**:
- CAN tests require `can0` or `vcan0` interface
- Serial tests require `/dev/ttyS0` or `/dev/ttyUSB0`
- Use `-i` interactive mode to skip hardware-dependent tests

### Debugging Tips

**Enable verbose logging**:
```c
OSAL_LogSetLevel(LOG_LEVEL_DEBUG);  // In your code
```

**GDB debugging**:
```bash
cmake --preset debug && cmake --build --preset debug
gdb --args ./build/debug/bin/ems-test -m test_osal_task
```

**Memory leak detection**:
```bash
valgrind --leak-check=full ./build/debug/bin/ems-test -m test_osal_queue
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
- **Layers**: 5 (OSAL/HAL/PCL/PDL/Apps)
- **Platforms**: TI AM6254, vendor_demo (extensible)
- **Build System**: CMake 3.16+, supports native and cross-compilation

## Important Files

- [CMakeLists.txt](CMakeLists.txt) - Main build config
- [build.sh](build.sh) - Build script wrapper
- [cmake/toolchains/](cmake/toolchains/) - Cross-compilation toolchain files
- [osal/include/osal.h](osal/include/osal.h) - OSAL main header
- [osal/include/osal_types.h](osal/include/osal_types.h) - Type definitions
- [tests/core/main.c](tests/core/main.c) - Test runner entry
- [apps/sample_app/src/main.c](apps/sample_app/src/main.c) - Sample application
- [apps/watchdog_app/src/main.c](apps/watchdog_app/src/main.c) - Watchdog application

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
