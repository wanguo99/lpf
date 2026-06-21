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
    config/        # runtime configuration backends and parsers
    hw/            # capability-grouped LPF HW implementations
    include/       # runtime-private helper headers
    runtime/       # integrated runtime entry and orchestration
    peripheral/    # framework-owned runtime services and service backends
  lpf-configs/
    configs/       # selected static board descriptions for lpf_configs.ko

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
  shared chrdev/sysfs/debugfs helpers. It also calls the integrated runtime
  entry linked into `lpf_core.ko`.
- `kernel/lpf-core/peripheral` owns framework runtime service
  implementations; current service paths are linked into `lpf_core.ko`.
- `kernel/lpf-core/protocol` provides kernel-side LPF protocol helpers through
  `lpf_core.ko` for services that need framed communication.
- `kernel/lpf-core/hw` provides capability-grouped LPF-owned hardware access
  implementations used by LPF peripheral services. The objects are linked into
  `lpf_core.ko`; runtime-private helper headers live under
  `kernel/lpf-core/include`.
- `kernel/lpf-core/config` provides LPF runtime config backend/parser objects
  linked into `lpf_core.ko`.
- `kernel/lpf-configs` builds `lpf_configs.ko`; its root holds module glue and
  section sentinels, while `kernel/lpf-configs/configs` holds concrete static
  board descriptions.
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
LPF Core device model + LPF HW APIs
    ↓
Linux kernel subsystem / hardware
```

Kernel-side device creation is driven by runtime config before userspace opens
an instance node:

```text
static config / Device Tree
    ↓
configured-device node table
    ↓
peripheral config driver
    ↓
lpf_device_register()
    ↓
LPF Core type match + service probe
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
- LPF UAPI rules are documented in `docs/lpf_uapi_abi.md`.
