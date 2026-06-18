# Kernel And Userspace Split

The Linux-only direction separates kernel modules from userspace API libraries.

## Target Layout

```text
core/
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
      src/           # builds pdm.ko, owns ioctl dispatch

  user/
    osal/            # userspace OSAL library
    aconfig/         # userspace application configuration
    pdi/             # userspace API library for PDM

  uapi/
    pdi/             # ioctl ABI shared by PDM and PDI
```

## Responsibilities

- `core/kernel/osal` wraps Linux kernel APIs and builds `osal.ko`.
- `core/kernel/pdm` owns the kernel module entry, device node, and ioctl
  boundary, and builds `pdm.ko`.
- `core/kernel/hal` provides kernel-only hardware access used by PDM.
- `core/kernel/pconfig` provides kernel-side platform/product configuration used
  by PDM and HAL.
- `core/user/pdi` provides the application-facing C API and wraps open/ioctl.
- `core/uapi/pdi` is the stable ABI shared by `core/kernel/pdm` and
  `core/user/pdi`.

## Include Rules

- Kernel cross-module headers live under `core/kernel/include/<module>/`.
- UAPI headers live under `core/uapi/pdi/` and must be valid for both kernel and
  userspace builds.
- Userspace code must not include non-UAPI kernel headers.
