# ES-Middleware Architecture

ES-Middleware is a Linux-focused middleware framework. It separates kernel
modules from userspace API libraries while keeping Kconfig-driven feature
selection and CMake-driven source builds.

## Layer Order

```text
Application/Product code
        ↓
ACONFIG
        ↓
PDI (userspace API)
        ↓
ioctl / UAPI
        ↓
PDM + PDM protocol
        ↓
PCONFIG
        ↓
HAL
        ↓
OSAL
        ↓
Linux kernel / hardware
```

## Core Modules

- User OSAL provides libc/POSIX wrappers for userspace libraries and tests.
- Kernel OSAL wraps Linux kernel APIs used by kernel modules.
- HAL provides kernel-side hardware access helpers.
- PCONFIG stores kernel-side platform hardware configuration tables.
- PDM provides kernel-side peripheral driver modules.
- PDM protocol helpers provide kernel-side packet framing owned by PDM.
- PDI provides userspace APIs over the PDM ioctl ABI.
- ACONFIG stores userspace application-facing configuration mappings.

## Current Peripheral Scope

The current framework keeps one concrete peripheral/device family:

- MCU configuration in PCONFIG
- MCU protocol in PDM
- MCU driver in PDM
- Userspace access through PDI
- LED configuration in PCONFIG
- LED driver in PDM
- Userspace access through PDI

Other peripheral families can be added later by introducing matching PCONFIG
types, PDM protocol definitions, PDM kernel implementation, PDI userspace API
coverage, UAPI definitions when needed, and Kconfig entries.

## Kernel Module Load Order

Load kernel modules in dependency order:

```text
osal.ko
pconfig.ko
hal.ko
pdm.ko
```

`pdm.ko` consumes PConfig entries and HAL symbols, then probes linked PDM
drivers for each configured peripheral.

## Adding A Peripheral

Add the following pieces together:

- PCONFIG type and platform config array entry.
- PDM driver object registered with `pdm_driver_register`.
- Optional PDM character device `/dev/pdm_<peripheral>` when userspace access
  is needed.
- UAPI header `uapi/pdi/pdi_<peripheral>.h` following
  `docs/PDI_UAPI_ABI.md`.
- Userspace wrapper `user/pdi/src/pdi_<peripheral>.c`.
- Kbuild object selection for the new PDM driver.

Feature selection stays at object/list registration boundaries. Kconfig may
select which objects are linked, but business logic should not branch on
feature macros.

## Dependency Rules

- Core modules do not depend on product/application code.
- Dependencies point downward through the layer stack.
- Product-specific behavior belongs outside shared middleware module directories.
- Kernel hardware tables are compiled through PCONFIG and consumed by PDM
  through typed accessors.
- Userspace code must use PDI/UAPI rather than including kernel-internal HAL,
  PCONFIG, or PDM headers.
