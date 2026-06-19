# ES-Middleware SDK

ES-Middleware is a Linux-focused embedded middleware framework built with
Kconfig, CMake, and Linux external-module Kbuild support.

## Current Scope

The repository currently contains framework core modules. Product-specific
satellite/PMC business code and the previous test product have been removed.

Current concrete peripheral/device type:

- MCU peripheral type in PCONFIG/PDM, exposed to userspace through PDI
- LED peripheral type in PCONFIG/PDM, exposed to userspace through PDI

The framework keeps the layered extension points so additional peripheral types can be added later without changing the core architecture.

## Core Layers

- OSAL: operating-system abstraction
- HAL: kernel hardware abstraction module (`hal.ko`)
- PCONFIG: platform hardware configuration registry
- PDM: kernel peripheral driver module
- PDI: userspace peripheral driver interface library
- ACONFIG: application configuration layer

The previous userspace test framework has been removed. New tests should be
introduced with a fresh layout and matching Kconfig/CMake integration when the
new test plan is ready.

## Build

```bash
make list
make kernel_x86_modules_defconfig
make modules
```

Kernel module load order is `osal.ko`, `pconfig.ko`, `hal.ko`, then `pdm.ko`.

## Project Layout

```text
ES-Middleware/
├── kernel/        # Kernel modules and kernel-side headers
├── user/          # Userspace libraries
├── uapi/          # Shared userspace/kernel ABI headers
├── configs/       # Development defconfigs
├── docs/          # Architecture and integration documentation
└── scripts/       # Kconfig/CMake build support
```

## Configuration

Defconfigs live under `configs/`, for example:

- `kernel_x86_modules_defconfig`

Use `make menuconfig` for interactive configuration.

## Documentation

- `docs/ARCHITECTURE.md`: current module architecture and extension flow.
- `docs/KERNEL_USER_SPLIT.md`: kernel/userspace boundary and include rules.
- `docs/PDI_UAPI_ABI.md`: PDI ioctl ABI rules for new peripherals.
