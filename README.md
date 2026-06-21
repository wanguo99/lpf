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
- LPF Configs: DTS-like static board description provider
  (`lpf_configs.ko`)
- LPF Core: framework device model, shared kernel infrastructure, integrated
  runtime orchestration, LPF HW, runtime config backends, and reusable
  peripheral services (`lpf_core.ko`)
- LPF HW: framework-owned hardware access APIs linked into `lpf_core.ko`
- LPF Runtime Config: platform hardware configuration registry linked into
  `lpf_core.ko`
- PDI: userspace peripheral driver interface library
- ACONFIG: application configuration layer

## What LPF Provides

- A clear split between kernel modules, UAPI headers, and userspace libraries.
- Kernel-side peripheral drivers built on LPF runtime config, LPF HW APIs,
  and OSAL.
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
layout checks, runtime config normalization checks, PDI userspace API
validation/error-path smoke tests, and syscall-mocked PDI ioctl operation-path
tests.

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

That preset also builds `lpf_configs.ko` and `lpf_hw_mock_selftest.ko`;
loading the modules in order runs LPF HW GPIO/PWM/CAN/UART/I2C/SPI checks
through the mock SoC adapter.

Kernel module load order is `osal.ko`, `lpf_configs.ko`, then `lpf_core.ko`.

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

- `docs/architecture.md`: current module architecture and extension flow.
- `docs/lpf_target_architecture.md`: target layered architecture.
- `docs/lpf_module_boundaries.md`: allowed dependencies and forbidden shortcuts.
- `docs/lpf_refactor_roadmap.md`: staged cleanup roadmap.
- `docs/lpf_kernel_compat_policy.md`: supported kernel baseline and compat
  feature-gate rules.
- `docs/kernel_user_split.md`: kernel/userspace boundary and include rules.
- `docs/lpf_long_term_optimization_plan.md`: long-term optimization plan for
  multi-kernel and multi-SoC deployments.
- `docs/lpf_uapi_abi.md`: UAPI ABI and PDI wrapper rules for new peripherals.
