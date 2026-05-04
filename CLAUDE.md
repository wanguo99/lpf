# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**EMS** (Embedded Middleware System) is a general-purpose embedded system board support package, providing hardware abstraction and peripheral management services for embedded controllers.

**Typical Application**:
```
External System <--CAN--> EMS Controller <--Ethernet/UART/CAN--> Device Modules
```

## Quick Commands

### Build

**Native Build (本地架构)**:
```bash
./build.sh              # Release build
./build.sh -d           # Debug build
./build.sh -c           # Clean build directory
```

**Cross-Compilation (交叉编译)**:
```bash
# ARM32 (ARMv7-A)
./build.sh -a arm32
./build.sh -a arm32 -d  # Debug mode

# ARM64 (ARMv8-A)
./build.sh -a arm64

# RISC-V 64
./build.sh -a riscv64

# x86_64 (explicit)
./build.sh -a x86_64
```

**Prerequisites for Cross-Compilation**:
```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-linux-gnueabihf      # ARM32
sudo apt-get install gcc-aarch64-linux-gnu        # ARM64
sudo apt-get install gcc-riscv64-linux-gnu        # RISC-V 64
```

### Test
```bash
./output/target/bin/unit-test -i    # Interactive menu (recommended)
./output/target/bin/unit-test -a    # Run all tests
./output/target/bin/unit-test -L OSAL  # Run OSAL layer tests
./output/target/bin/unit-test -m test_osal_task  # Run specific module

# Fast iteration: compile single test layer
cd output/build && make osal_tests -j$(nproc) && cd ../..
cd output/build && make hal_tests -j$(nproc) && cd ../..
cd output/build && make pdl_tests -j$(nproc) && cd ../..
cd output/build && make pcl_tests -j$(nproc) && cd ../..
```

### Run
```bash
./output/target/bin/sample_app      # Sample application
```

### Debug
```bash
./build.sh -d
gdb ./output/target/bin/sample_app
gdb --args ./output/target/bin/unit-test -m test_osal_task
```

## Architecture: 5-Layer Design

**Critical Dependency Chain**:
```
Apps → PDL → HAL → OSAL → Linux System Calls
         ↓
        PCL (configuration data)
```

### OSAL - Operating System Abstraction Layer
- **Only layer allowed to include system headers** (`<unistd.h>`, `<pthread.h>`, etc.)
- Provides two types of interfaces:
  1. High-level business APIs: `OSAL_TaskCreate()`, `OSAL_QueueCreate()`
  2. Raw system call wrappers: `OSAL_socket()`, `OSAL_open()`, `OSAL_close()`
- All standard library functions must be wrapped here
- Encapsulates operating system APIs for cross-platform portability
- Platform-specific code in `src/posix/` (future: `src/freertos/`, `src/vxworks/`)

### HAL - Hardware Abstraction Layer
- **Only layer allowed to include hardware-specific headers** (`<linux/can.h>`, `<net/if.h>`)
- **Must use OSAL wrappers for all system calls** (never direct `socket()`, `open()`, etc.)
- Encapsulates hardware drivers: CAN, UART, I2C, SPI
- Platform-specific code in `src/linux/` (future: `src/ti_am62/`, `src/nxp_imx8/`)

### PCL - Peripheral Configuration Library
- Device-tree-like hardware configuration
- **Must be completely platform-independent** - pure data structures
- Configuration organized by peripheral (MCU/BMC/Satellite/Sensor)
- Platform configs in nested structure: `platform/<vendor>/<chip>/<product>/`

### PDL - Peripheral Driver Layer
- Unified management of satellite/BMC/MCU peripherals
- **Must be completely platform-independent**
- Provides application-facing peripheral interfaces
- Only Apps and Tests layers can access PDL APIs

### Apps - Application Layer
- **Must be completely platform-independent**
- Currently contains only sample_app (reference implementation)

## Critical Rules

### Platform Independence (MOST IMPORTANT)

**System Call Encapsulation**:
- ❌ **FORBIDDEN** in HAL/PCL/PDL/Apps/Tests: Direct use of `socket()`, `bind()`, `open()`, `close()`, `memcpy()`, `strlen()`, `printf()`, etc.
- ✅ **REQUIRED**: Use OSAL wrappers: `OSAL_socket()`, `OSAL_open()`, `OSAL_Memcpy()`, `OSAL_Strlen()`, `LOG_INFO()`, etc.

**Header File Rules**:
- ✅ OSAL layer: Can include `<unistd.h>`, `<sys/socket.h>`, `<pthread.h>`, `<stdlib.h>`
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
char                           // For strings and text data ONLY (compatible with standard C library)

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

**Usage Rules**:
- `char` for strings and text data ONLY
- `uint8`/`int8` for binary data and bytes
- Fixed-size types for numeric data
- `osal_size_t`/`osal_ssize_t` for memory sizes and array indices

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

**Correct Examples**:
```c
/* ✅ CORRECT: Using OSAL types */
int32 HAL_CAN_Init(const char *device, hal_can_handle_t *handle)
{
    uint32 filter_id = 0x100;
    uint8 dlc = 8;
    osal_size_t buffer_size = 1024;
    bool is_initialized = false;
    return OSAL_SUCCESS;
}

/* ❌ WRONG: Using native types */
int HAL_CAN_Init(const char *device, void *handle)  // ❌ int, char
{
    unsigned int filter_id = 0x100;  // ❌ unsigned int
    size_t buffer_size = 1024;       // ❌ size_t
    return 0;
}
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

## Development Workflows

### Adding New OSAL Interface
1. Add interface declaration in `osal/include/`
2. Implement POSIX version in `osal/src/posix/`
3. Add unit tests in `tests/osal/`
4. Update `osal/README.md`

### Adding New HAL Driver
1. Add driver interface in `hal/include/`
2. Add driver config in `hal/include/config/`
3. Implement Linux version in `hal/src/linux/` (using OSAL wrappers)
4. Add unit tests in `tests/hal/`

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
1. Create test file in `tests/<layer>/` (e.g., `test_osal_timer.c`)
2. Use `TEST_MODULE_BEGIN/END` macros
3. Add source file to `tests/CMakeLists.txt`
4. Build and run: `./build.sh -d && ./output/target/bin/unit-test -m test_osal_timer`
5. For fast iteration: `cd output/build && make osal_tests -j$(nproc) && cd ../..`

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
- **Type System**: Uses fixed-size types (`uint32_t`, `int64_t`) for portability
- **Endianness**: Automatic detection with `OSAL_HTONS/HTONL` macros for byte order conversion
- **Atomic Operations**: Uses C11 `_Atomic` with fixed-size types (`_Atomic uint32_t`)
- **Pointer Casting**: Uses `uintptr_t` for safe pointer-to-integer conversions

### Cross-Compilation
```bash
# ARM32 (requires arm-linux-gnueabihf-gcc)
./build.sh -a arm32

# ARM64 (requires aarch64-linux-gnu-gcc)
./build.sh -a arm64

# RISC-V 64 (requires riscv64-linux-gnu-gcc)
./build.sh -a riscv64

# Install toolchains on Ubuntu/Debian:
sudo apt-get install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu gcc-riscv64-linux-gnu
```

**Architecture-Specific Compiler Flags**:
- ARM32: `-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard`
- ARM64: `-march=armv8-a`
- RISC-V 64: `-march=rv64imafdc -mabi=lp64d`

### Hardware-Related Test Failures
- CAN tests require `can0` device
- Serial tests require `/dev/ttyS0`
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

## Build Output Structure

```
output/
├── build.log        # Build log
├── build/           # Intermediate files
└── target/
    ├── bin/         # Executables (sample_app, unit-test)
    └── lib/         # Static libraries (libosal.a, libhal.a, libpdl.a, libpcl.a)
```

## Documentation

- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Coding Standards**: [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md)
- **Module READMEs**: Each layer has `README.md` and `docs/` directory
- **Test Framework**: [tests/README.md](tests/README.md)

## Project Stats

- **Code Size**: ~50,000 lines of C/H source code
- **Files**: 128 C/H files
- **Test Coverage**: 142+ test cases across all layers
  - OSAL: 50+ tests (10 modules)
  - HAL: 72 tests (CAN, UART, I2C, SPI)
  - PCL: 5+ tests
  - PDL: 15+ tests (Satellite, BMC, MCU)
- **Layers**: 5 (OSAL/HAL/PCL/PDL/Apps)
- **Platforms**: TI AM6254, vendor_demo (extensible)
- **Build System**: CMake 3.10+, supports native and cross-compilation

## Important Files

- [CMakeLists.txt](CMakeLists.txt) - Main build config
- [build.sh](build.sh) - Build script
- [osal/include/osal.h](osal/include/osal.h) - OSAL main header
- [tests/core/main.c](tests/core/main.c) - Test runner entry
- [apps/sample_app/src/main.c](apps/sample_app/src/main.c) - Sample application

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
