# Kernel And Userspace Split

LPF is Linux-only in its current direction. It separates kernel modules from
userspace API libraries so hardware access, ioctl ABI definitions, and
application-facing APIs remain in the right layer.

## Target Layout

```text
kernel/
  Makefile
  include/
    osal/          # kernel-side cross-module OSAL headers
    hal/           # kernel-side cross-module HAL headers
    pconfig/       # kernel-side cross-module PConfig headers
    lpf/           # kernel-side cross-module LPF headers
  osal/
    src/           # builds osal.ko
  hal/
    src/           # kernel-only hardware access implementation
  pconfig/
    src/           # kernel-side configuration implementation
  lpf/
    core/          # LPF device model and shared node infrastructure
    protocol/      # LPF protocol helpers linked into lpf_core.ko
    peripheral/    # framework-owned peripheral runtime and services

user/
  osal/            # userspace OSAL library
  aconfig/         # userspace application configuration
  pdi/             # userspace API library for LPF

uapi/
  lpf/             # ioctl ABI shared by LPF kernel nodes and PDI
```

## Responsibilities

- `kernel/osal` wraps Linux kernel APIs and builds `osal.ko`.
- `kernel/lpf/core` owns the LPF device model, control/discovery node, and
  shared chrdev/sysfs/debugfs helpers.
- `kernel/lpf/peripheral` owns the framework peripheral runtime, integrated
  module entry, and service implementations; current service paths are linked
  into `lpf_peripheral_runtime.ko`.
- `kernel/lpf/protocol` provides kernel-side LPF protocol helpers through
  `lpf_core.ko` for services that need framed communication.
- `kernel/hal` provides kernel-only hardware access used by LPF peripheral
  services. It builds as `hal.ko` and exports HAL API symbols.
- `kernel/pconfig` provides kernel-side platform/product configuration used
  by LPF peripheral configuration. It builds as `pconfig.ko`.
- `user/pdi` provides the application-facing C API and wraps open/ioctl.
- `uapi/lpf` is the stable ABI shared by LPF kernel nodes and `user/pdi`.

## Boundary Rules

- Kernel code may include `kernel/include/<module>/` and generated headers.
- Userspace code may include `user/<module>/include/` and `uapi/` headers.
- Userspace code must not include kernel-internal HAL, PCONFIG, LPF Core, or
  LPF peripheral headers.
- UAPI headers must not depend on kernel-only types or private framework
  structures.
- PDI should marshal data and call ioctl; it should not duplicate LPF
  peripheral service or HAL behavior in userspace.
- Product code should call PDI and ACONFIG rather than reaching into kernel
  framework internals.

## Device Access Flow

```text
Application
    ↓
PDI userspace API
    ↓
/dev/lpf/<peripheral><index> ioctl
    ↓
LPF peripheral service
    ↓
PCONFIG + LPF Core + HAL
    ↓
Linux kernel subsystem / hardware
```

Each userspace-visible peripheral should have a matching UAPI header and PDI
wrapper. For example, MCU uses `/dev/lpf/mcu0`, `uapi/lpf/lpf_mcu.h`, and
`user/pdi/src/pdi_mcu.c`.

The previous userspace test product was removed. New tests live under
`tests/` with explicit CMake/CTest integration; UAPI layout checks and PDI
userspace smoke tests currently run through `make tests`. PDI operation-path
tests use an internal syscall mock boundary so ioctl marshaling can be
validated without requiring live LPF device nodes.

## Include Rules

- Kernel cross-module headers live under `kernel/include/<module>/`.
- UAPI headers live under `uapi/lpf/` and must be valid for both kernel and
  userspace builds.
- Userspace code must not include non-UAPI kernel headers.
- LPF UAPI rules are documented in `docs/LPF_UAPI_ABI.md`.
