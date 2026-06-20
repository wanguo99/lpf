# LPF - Linux Peripheral Framework

LPF (Linux Peripheral Framework) is a Linux-focused peripheral access framework.
It provides the shared layers needed to describe platform devices, access
hardware from kernel modules, expose stable kernel/userspace ABIs, and offer
application-facing C APIs.

LPF is not limited to embedded products. The current modules are useful for
embedded Linux boards, industrial controllers, development machines, and other
Linux systems that need a reusable peripheral driver and access stack.

The build system combines Kconfig feature selection, CMake userspace builds,
and Linux external-module Kbuild support.

## Current Scope

The repository currently contains framework core modules. Product-specific
satellite/PMC business code and the previous test product have been removed.

Current concrete peripheral/device types:

- MCU peripheral type in LPF runtime config, exposed to
  userspace through PDI
- LED peripheral type in LPF runtime config, exposed to
  userspace through PDI

The framework keeps layered extension points so additional peripheral types can
be added later without changing the core architecture.

LPF currently targets Linux only. OSAL still separates operating-system-facing
APIs from higher layers, but non-Linux ports are outside the current direction.

## Core Layers

- OSAL: operating-system abstraction
- HAL: kernel hardware abstraction module (`hal.ko`)
- LPF Runtime Config: platform hardware configuration registry linked into
  `lpf_peripheral_runtime.ko`
- LPF Core: framework device model and shared kernel infrastructure
- LPF Peripheral Runtime: integrated kernel module for reusable peripheral
  services (`lpf_peripheral_runtime.ko`)
- PDI: userspace peripheral driver interface library
- ACONFIG: application configuration layer

## What LPF Provides

- A clear split between kernel modules, UAPI headers, and userspace libraries.
- Kernel-side peripheral drivers built on LPF runtime config, HAL, and OSAL.
- Userspace PDI libraries that hide ioctl details from applications.
- Kconfig-controlled feature selection for modules and peripheral families.
- CMake/Kbuild integration for userspace libraries and Linux kernel modules.
- A repeatable extension path for adding new peripheral types.

## What LPF Does Not Own

- Product or business logic.
- Product-specific init scripts, services, or deployment policy.
- Generic Linux application framework behavior unrelated to peripheral access.
- Generated build artifacts under `_build/`.

The previous userspace test product has been removed. New tests now live under
`tests/` with CMake/CTest integration. Current coverage includes UAPI ABI
layout checks, PDI userspace API validation/error-path smoke tests, and
syscall-mocked PDI ioctl operation-path tests.

## Quick Start

```bash
make list
make kernel_x86_modules_defconfig
make all
```

To build only the kernel modules:

```bash
make modules
```

To build and run the current test targets:

```bash
make tests
```

To build the no-hardware kernel mock preset:

```bash
make kernel_x86_mock_modules_defconfig
make modules
```

That preset also builds `hal_mock_selftest.ko`; loading it after `osal.ko`,
`lpf_core.ko`, and `hal.ko` runs HAL GPIO/PWM/CAN/Serial/I2C/SPI checks through
the mock SoC adapter.

Kernel module load order is `osal.ko`, `lpf_core.ko`, `hal.ko`, then
`lpf_peripheral_runtime.ko`.

Generated libraries are written under `_build/lib/`. Kernel modules are written
under `_build/modules/`.

## Configuration

Defconfigs live under `configs/`, for example:

- `kernel_x86_modules_defconfig`
- `kernel_x86_mock_modules_defconfig`

Use `make menuconfig` for interactive configuration.

## Project Layout

```text
LPF/
├── kernel/        # Kernel modules and kernel-side headers
├── user/          # Userspace libraries
├── uapi/          # Shared userspace/kernel ABI headers
├── configs/       # Development defconfigs
├── docs/          # Architecture and integration documentation
├── tests/         # ABI, mock, and integration test targets
└── scripts/       # Kconfig/CMake build support
```

## Documentation

- `docs/ARCHITECTURE.md`: current module architecture and extension flow.
- `docs/KERNEL_USER_SPLIT.md`: kernel/userspace boundary and include rules.
- `docs/LPF_LONG_TERM_OPTIMIZATION_PLAN.md`: long-term optimization plan for
  multi-kernel and multi-SoC deployments.
- `docs/LPF_UAPI_ABI.md`: PDI ioctl ABI rules for new peripherals.
