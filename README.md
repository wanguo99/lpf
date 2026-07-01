# PAF - Peripheral Access Framework

PAF (Peripheral Access Framework) is a Linux-focused peripheral access
framework. It provides the shared layers needed to describe platform devices,
access hardware from kernel modules, expose stable kernel/userspace ABIs, and
offer application-facing C APIs.

PAF is not limited to embedded products. The current modules are useful for
embedded Linux boards, industrial controllers, development machines, and other
Linux systems that need a reusable peripheral driver and access stack.

The build system combines Kconfig feature selection, CMake userspace builds,
and Linux external-module Kbuild support.

## Current Scope

The repository currently contains the framework core modules. Product-specific
business code, legacy configuration/runtime layers, standalone portability
layers, and the previous examples/tests products have been removed.

Current kernel modules:

- PDM (`pdm.ko`), the Peripheral Driver Module implemented as a Linux
  `bus_type` named `pdm`

PDM currently provides the standard Linux bus integration, Device Tree backed
PDM device creation through the PDM bus controller, native UART/I2C/SPI MCU
device creation, `/dev/pdm_manager` discovery snapshots, read-only sysfs
diagnostics, optional development proc/debugfs controls, MCU transport backends,
and LED GPIO/PWM backends.

PAF currently targets Linux only. Kernel code uses native Linux APIs, and
userspace code uses libc/POSIX APIs directly. Logging is local to PDM in kernel
space and PDI in userspace through the existing `LOG_*` interface.

## Core Layers

- PDM: Peripheral Driver Module; Linux `bus_type`, PDM device lifecycle, Device
  Tree bus controller, `/dev/pdm_manager` discovery, and shared kernel helper
  infrastructure (`pdm.ko`)
- UAPI: stable userspace/kernel ABI headers
- PDI: Peripheral Driver Interface userspace library; discovery and MCU/LED
  wrappers remain available, but require matching kernel drivers/device nodes to
  be implemented on the PDM bus

## What PAF Provides

- A clear split between kernel modules, UAPI headers, and userspace libraries.
- A standard Linux driver-model entry point for PDM kernel drivers.
- Device Tree based creation of PDM devices under `/sys/bus/pdm/`.
- Userspace PDI libraries that hide ioctl details from applications; discovery
  works through `/dev/pdm_manager`, and MCU/LED bring-up nodes are available through
  `/dev/pdm/mcuN` and `/dev/pdm/ledN`.
- Kconfig-controlled feature selection for PDM and PDI modules.
- CMake/Kbuild integration for userspace libraries and Linux kernel modules.

## What PAF Does Not Own

- Product or business logic.
- Product-specific init scripts, services, or deployment policy.
- Generic Linux application framework behavior unrelated to peripheral access.
- Generated build artifacts under `_build/`.

The previous userspace examples and historical top-level test tree have been
removed. New coverage should use a fresh layout when the lower layers
stabilize.

## Quick Start

```bash
make list
make ubuntu_x86_modules_defconfig
make all
```

To build only the kernel modules:

```bash
make modules
```

Generated libraries are written under `_build/lib/`. Kernel modules are written
under `_build/modules/`.

`make modules` builds `pdm.ko`. PDM kernel logging is linked into `pdm.ko`;
there is no separate kernel portability or logging module.

For x86 smoke testing without Device Tree child nodes, use
`ubuntu_x86_mock_modules_defconfig`; it enables synthetic MCU/LED PDM devices so
`scripts/pdm_mock_module_smoke.sh` can verify `/dev/pdm/mcu0` and
`/dev/pdm/led0` in addition to `/dev/pdm_manager`.

## Configuration

Defconfigs live under `configs/`, for example:

- `ubuntu_x86_modules_defconfig`
- `ubuntu_x86_mock_modules_defconfig`

Use `make menuconfig` for interactive configuration.

## Project Layout

```text
PAF/
├── kernel/pdm/         # PDM kernel module implementation
├── kernel/include/pdm/ # Kernel-side PDM public headers
├── user/pdi/           # Userspace PDI library
├── apps/               # Common userspace applications
├── uapi/pdm/           # Shared userspace/kernel ABI headers
├── configs/            # Development defconfigs
├── docs/               # Architecture and integration documentation
└── scripts/            # Kconfig/CMake build support
```

## Documentation

- `docs/architecture.md`: current module architecture and extension flow.
- `docs/PDM_架构分析.md`: detailed Chinese PDM architecture analysis and design notes.
- `docs/pdm_can_transport.md`: CAN transport frame format, DTS properties, and runtime setup.
- `docs/buildroot/buildroot_integration.md`: Buildroot integration notes.
