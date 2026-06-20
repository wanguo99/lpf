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
    pdm/           # kernel-side cross-module PDM headers
  osal/
    src/           # builds osal.ko
  hal/
    src/           # kernel-only hardware access implementation
  pconfig/
    src/           # kernel-side configuration implementation
  lpf/
    protocol/      # LPF protocol helpers linked into lpf_core.ko
  pdm/
    src/           # builds pdm.ko and owns ioctl dispatch

user/
  osal/            # userspace OSAL library
  aconfig/         # userspace application configuration
  pdi/             # userspace API library for PDM

uapi/
  lpf/             # ioctl ABI shared by PDM and PDI
```

## Responsibilities

- `kernel/osal` wraps Linux kernel APIs and builds `osal.ko`.
- `kernel/pdm` owns the kernel module entry, device node, ioctl boundary,
  and links current built-in LPF peripheral services into `pdm.ko`.
- `kernel/lpf/protocol` provides kernel-side LPF protocol helpers through
  `lpf_core.ko` for services that need framed communication.
- `kernel/hal` provides kernel-only hardware access used by PDM.
  It builds as `hal.ko` and exports HAL API symbols for PDM.
- `kernel/pconfig` provides kernel-side platform/product configuration used
  by PDM. It builds as `pconfig.ko`.
- `user/pdi` provides the application-facing C API and wraps open/ioctl.
- `uapi/lpf` is the stable ABI shared by `kernel/pdm` and `user/pdi`.

## Boundary Rules

- Kernel code may include `kernel/include/<module>/` and generated headers.
- Userspace code may include `user/<module>/include/` and `uapi/` headers.
- Userspace code must not include kernel-internal HAL, PCONFIG, or PDM headers.
- UAPI headers must not depend on kernel-only types or private framework
  structures.
- PDI should marshal data and call ioctl; it should not duplicate PDM or HAL
  behavior in userspace.
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
PDM kernel driver
    ↓
PCONFIG + HAL
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
