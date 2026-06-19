# Kernel And Userspace Split

The Linux-only direction separates kernel modules from userspace API libraries.

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
  pdm/
    src/           # builds pdm.ko, owns ioctl dispatch and protocol helpers

user/
  osal/            # userspace OSAL library
  aconfig/         # userspace application configuration
  pdi/             # userspace API library for PDM

uapi/
  pdi/             # ioctl ABI shared by PDM and PDI
```

## Responsibilities

- `kernel/osal` wraps Linux kernel APIs and builds `osal.ko`.
- `kernel/pdm` owns the kernel module entry, device node, ioctl boundary,
  and links kernel-side PCONFIG/PDM protocol objects into `pdm.ko` when enabled.
- `kernel/hal` provides kernel-only hardware access used by PDM.
  It builds as `hal.ko` and exports HAL API symbols for PDM.
- `kernel/pconfig` provides kernel-side platform/product configuration used
  by PDM. It builds as `pconfig.ko`.
- `user/pdi` provides the application-facing C API and wraps open/ioctl.
- `uapi/pdi` is the stable ABI shared by `kernel/pdm` and `user/pdi`.

The previous userspace test framework was removed with the old ctest product.
New tests should be added under a new test layout with explicit Kconfig/CMake
integration.

## Include Rules

- Kernel cross-module headers live under `kernel/include/<module>/`.
- UAPI headers live under `uapi/pdi/` and must be valid for both kernel and
  userspace builds.
- Userspace code must not include non-UAPI kernel headers.
- PDI/UAPI ABI rules are documented in `docs/PDI_UAPI_ABI.md`.
