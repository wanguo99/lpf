# PDM - Linux Peripheral Framework

PDM (Linux Peripheral Framework) is a Linux-focused peripheral access framework.
It provides the shared layers needed to describe platform devices, access
hardware from kernel modules, expose stable kernel/userspace ABIs, and offer
application-facing C APIs.

PDM is not limited to embedded products. The current modules are useful for
embedded Linux boards, industrial controllers, development machines, and other
Linux systems that need a reusable peripheral driver and access stack.

The build system combines Kconfig feature selection, CMake userspace builds,
and Linux external-module Kbuild support.

## Current Scope

The repository currently contains the framework core modules. Product-specific
satellite/PMC business code, legacy configuration/runtime layers, and the
previous test product have been removed.

Current kernel modules:

- OSAL (`osal.ko`)
- PDM Core (`pdm.ko`), implemented as a Linux `bus_type` named `pdm`

PDM Core currently provides the standard Linux bus integration, Device Tree
backed PDM device creation through the PDM bus controller, native UART/I2C/SPI
MCU device creation, `/dev/pdm_ctl` discovery snapshots, lightweight
proc/debugfs/sysfs helpers, MCU transport backends, and LED GPIO/PWM backends.

PDM currently targets Linux only. OSAL still separates operating-system-facing
APIs from higher layers, but non-Linux ports are outside the current direction.

## Core Layers

- OSAL: operating-system abstraction
- PDM Core: Linux `bus_type`, PDM device lifecycle, Device Tree bus controller,
  `/dev/pdm_ctl` discovery, and shared kernel helper infrastructure
  (`pdm.ko`)
- UAPI: stable userspace/kernel ABI headers
- PDI: userspace peripheral driver interface library; discovery and MCU/LED
  wrappers remain available, but require matching kernel drivers/device nodes to
  be implemented on the new bus
- ACONFIG: application configuration layer

## What PDM Provides

- A clear split between kernel modules, UAPI headers, and userspace libraries.
- A standard Linux driver-model entry point for PDM kernel drivers.
- Device Tree based creation of PDM devices under `/sys/bus/pdm/`.
- Userspace PDI libraries that hide ioctl details from applications; discovery
  works through `/dev/pdm_ctl`, and MCU/LED bring-up nodes are available through
  `/dev/pdm/mcuN` and `/dev/pdm/ledN`.
- Kconfig-controlled feature selection for core modules.
- CMake/Kbuild integration for userspace libraries and Linux kernel modules.

## What PDM Does Not Own

- Product or business logic.
- Product-specific init scripts, services, or deployment policy.
- Generic Linux application framework behavior unrelated to peripheral access.
- Generated build artifacts under `_build/`.

The previous userspace test product has been removed. Tests under `tests/` still
include historical ABI/PDI coverage, but kernel-side MCU/LED runtime behavior
needs new bus-based test coverage as those drivers are reintroduced.

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

Kernel module load order is `osal.ko`, then `pdm.ko`.

For x86 smoke testing without Device Tree child nodes, use
`ubuntu_x86_mock_modules_defconfig`; it enables synthetic MCU/LED PDM devices so
`scripts/pdm_mock_module_smoke.sh` can verify `/dev/pdm/mcu0` and
`/dev/pdm/led0` in addition to `/dev/pdm_ctl`.

## Configuration

Defconfigs live under `configs/`, for example:

- `ubuntu_x86_modules_defconfig`
- `ubuntu_x86_mock_modules_defconfig`

Use `make menuconfig` for interactive configuration.

## Project Layout

```text
PDM/
├── kernel/        # Kernel modules and kernel-side headers
├── user/          # Userspace libraries
├── apps/          # Common userspace applications
├── uapi/          # Shared userspace/kernel ABI headers
├── configs/       # Development defconfigs
├── docs/          # Architecture and integration documentation
├── tests/         # ABI, mock, and integration test targets
└── scripts/       # Kconfig/CMake build support
```

## Documentation

- `docs/architecture.md`: current module architecture and extension flow.
- `docs/pdm_module_boundaries.md`: allowed dependencies and forbidden shortcuts.
- `docs/pdm_kernel_compat_policy.md`: supported kernel baseline and compat
  feature-gate rules.
- `docs/kernel_user_split.md`: kernel/userspace boundary and include rules.
- `docs/pdm_uapi_abi.md`: UAPI ABI and PDI wrapper rules for new peripherals.
- `docs/pdm_bus_devicetree.md`: current PDM bus child-node compatibles and backend properties.
- `docs/pdm_can_transport.md`: CAN transport frame format, DTS properties, and runtime setup.
