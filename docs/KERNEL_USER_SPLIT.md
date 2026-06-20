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
    lpf/           # kernel-side cross-module LPF headers and layer subdirs
      compat/      # Linux kernel compatibility headers
      config/      # runtime config type headers
      core/        # LPF Core model and shared node headers
      hw/          # LPF HW API headers
      peripheral/  # LPF runtime and service headers
      protocol/    # LPF protocol headers
      soc/         # LPF SoC adapter headers
  osal/
    src/           # builds osal.ko
  lpf-core/
    core/          # LPF device model and shared node infrastructure
    protocol/      # LPF protocol helpers linked into lpf_core.ko
    compat/        # Linux kernel compatibility wrappers
    soc/           # SoC adapter backends
  lpf-runtime/
    config/        # runtime configuration backends and tables
    hw/            # capability-grouped LPF HW implementations
    include/       # runtime-private helper headers
    runtime/       # lpf_runtime.ko entry and orchestration
    peripheral/    # framework-owned runtime services and service backends

user/
  osal/            # userspace OSAL library
  aconfig/         # userspace application configuration
  pdi/             # userspace API library for LPF

uapi/
  lpf/             # ioctl ABI shared by LPF kernel nodes and PDI
```

## Responsibilities

- `kernel/osal` wraps Linux kernel APIs and builds `osal.ko`.
- `kernel/lpf-core/core` owns the LPF device model, control/discovery node, and
  shared chrdev/sysfs/debugfs helpers.
- `kernel/lpf-runtime/peripheral` owns the framework runtime, integrated
  module entry, runtime configuration loading, and service implementations;
  current service paths are linked into `lpf_runtime.ko`.
- `kernel/lpf-core/protocol` provides kernel-side LPF protocol helpers through
  `lpf_core.ko` for services that need framed communication.
- `kernel/lpf-runtime/hw` provides capability-grouped LPF-owned hardware access
  implementations used by LPF peripheral services. The objects are linked into
  `lpf_runtime.ko`; runtime-private helper headers live under
  `kernel/lpf-runtime/include`.
- `kernel/lpf-runtime/config` provides LPF runtime config source files and type headers.
  The objects are linked into `lpf_runtime.ko` rather than a
  standalone module.
- `user/pdi` provides the application-facing C API and wraps open/ioctl.
- `uapi/lpf` is the stable ABI shared by LPF kernel nodes and `user/pdi`.

## Boundary Rules

- Kernel code may include `kernel/include/<module>/` and generated headers.
- Userspace code may include `user/<module>/include/` and `uapi/` headers.
- Userspace code must not include kernel-internal LPF HW, LPF runtime config,
  LPF Core, or LPF peripheral headers.
- UAPI headers must not depend on kernel-only types or private framework
  structures.
- PDI should marshal data and call ioctl; it should not duplicate LPF
  peripheral service or LPF HW behavior in userspace.
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
LPF runtime config + LPF Core + LPF HW APIs
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
- LPF layer-specific kernel headers live under `kernel/include/lpf/<layer>/`.
- UAPI headers live under `uapi/lpf/` and must be valid for both kernel and
  userspace builds.
- Userspace code must not include non-UAPI kernel headers.
- LPF UAPI rules are documented in `docs/LPF_UAPI_ABI.md`.
