# LPF Architecture

LPF (Linux Peripheral Framework) separates kernel modules from userspace API
libraries while keeping Kconfig-driven feature selection and CMake-driven source
builds. Its main job is to provide a reusable Linux peripheral access stack:
platform configuration, kernel hardware access, peripheral drivers, stable UAPI
headers, and userspace API wrappers.

LPF is intentionally narrower than a general Linux software framework. Product
logic, service lifecycle, deployment policy, and application behavior stay
outside the shared framework layers.

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
PDM peripheral services + PDM protocol
        ↓
LPF Core + PCONFIG mapping
        ↓
HAL
        ↓
LPF SoC Adapter
        ↓
LPF Kernel Compat
        ↓
OSAL
        ↓
Linux kernel / hardware
```

## Core Modules

- User OSAL provides libc/POSIX wrappers for userspace libraries and tests.
- Kernel OSAL wraps Linux kernel APIs used by kernel modules.
- LPF Core owns the framework device and driver registry, lifecycle, device
  names, capability metadata, and the active SoC adapter.
- HAL provides kernel-side hardware access helpers.
- LPF SoC Adapter isolates SoC or vendor BSP differences below HAL.
- LPF Kernel Compat isolates Linux kernel API differences below the generic
  Linux SoC adapter.
- PCONFIG selects a kernel-side platform configuration backend and exposes a
  normalized device list.
- PDM provides kernel-side peripheral service modules.
- PDM protocol helpers provide kernel-side packet framing owned by PDM.
- PDI provides userspace APIs over the PDM ioctl ABI.
- ACONFIG stores userspace application-facing configuration mappings.

## Layer Responsibilities

### OSAL

OSAL hides Linux kernel or libc/POSIX API differences from framework code. The
kernel and userspace implementations are separate, but public names should match
where the semantics are equivalent.

### HAL

HAL owns LPF hardware-capability APIs in kernel mode. Current HAL support
includes CAN, serial, GPIO, PWM, I2C, and SPI. HAL calls the LPF SoC Adapter
instead of Linux subsystem APIs directly. PDM calls exported HAL symbols;
userspace does not include or call HAL directly.

### LPF Core

LPF Core owns the framework-level device model. Peripheral services register as
LPF drivers, and configured device instances are registered as LPF devices. LPF
Core handles bind, probe, remove ordering, stable device names, and capability
metadata. It does not depend on PCONFIG; callers map configuration backends into
LPF device configs before registration. LPF Core also initializes the default
SoC adapter path used by HAL. Device discovery callers should use the snapshot
APIs (`lpf_device_list()`, `lpf_device_get_info_by_name()`, and capability
queries) instead of retaining internal `lpf_device_t` pointers across lifecycle
events.

### LPF SoC Adapter

The SoC adapter layer owns hardware backend differences below HAL. The current
default backend is `generic-linux`, which calls LPF kernel compat wrappers for
CAN, serial, GPIO, PWM, I2C, and SPI. Future SoC-specific adapters should live
under `kernel/lpf/soc/` and must keep vendor BSP calls out of HAL and PDM.

### LPF Kernel Compat

The compat layer wraps Linux kernel API details that may vary across kernel
versions. GPIO, PWM, I2C, and SPI wrappers currently live under
`kernel/lpf/compat/`, along with CAN and serial wrappers. Peripheral services
and HAL business-facing APIs should not use `LINUX_VERSION_CODE` or vendor BSP
APIs directly.

### PCONFIG

PCONFIG owns kernel-side platform hardware configuration aggregation. It selects
a backend, validates the active platform, and exposes enabled device entries in
one normalized list. Backend selection is controlled by the `backend` module
parameter: `auto` tries Device Tree first and falls back to the built-in static
table, while `dt` and `static` require a specific backend. Future
board-profile or product-selection backends should produce the same
`pconfig_platform_config_t` and `pconfig_device_config_t` model before PDM sees
the data.

### PDM

PDM owns kernel-side peripheral business behavior, ioctl dispatch, procfs debug
nodes, and protocol helpers. Concrete peripheral services such as MCU and LED
live under PDM and register with LPF Core for device lifecycle handling. PDM
also exposes `/dev/pdm_ctl` as the management/discovery ioctl node; business
operations stay on peripheral nodes such as `/dev/pdm_mcu` and `/dev/pdm_led`.

### UAPI

UAPI headers define the stable ioctl ABI shared between PDM and PDI. They must
remain valid for both kernel and userspace compilation and should avoid
kernel-internal types.

### PDI

PDI is the userspace C API layer. It opens the matching `/dev/pdm_*` node,
marshals requests through UAPI ioctls, and hides ioctl details from
applications. Discovery APIs use `/dev/pdm_ctl` to list LPF device snapshots and
look up devices by stable name or capability.

### ACONFIG

ACONFIG provides application-facing configuration mapping. It is separate from
PCONFIG so application function metadata does not leak into hardware tables.

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

## Build And Runtime Boundaries

- Userspace libraries are built by CMake from `user/`.
- Linux kernel modules are built by Kbuild from `kernel/`.
- Shared ABI headers live in `uapi/`.
- Generated configuration and version headers are produced under `include/`.
- Build outputs are written under `_build/`.

## Kernel Module Load Order

Load kernel modules in dependency order:

```text
osal.ko
lpf_core.ko
pconfig.ko
hal.ko
pdm.ko
```

`pdm.ko` consumes PConfig entries and HAL symbols, maps enabled PConfig entries
to LPF devices, and lets LPF Core probe linked peripheral services for each
configured peripheral.

## Adding A Peripheral

Add the following pieces together:

- PCONFIG type and platform config array entry.
- LPF device type and capability mapping for the PCONFIG entry.
- PDM service driver object registered with `lpf_driver_register`.
- Optional PDM character device `/dev/pdm_<peripheral>` when userspace access
  is needed.
- UAPI header `uapi/pdi/pdi_<peripheral>.h` following
  `docs/PDI_UAPI_ABI.md`.
- Userspace wrapper `user/pdi/src/pdi_<peripheral>.c`.
- Kbuild object selection for the new PDM driver.

Feature selection stays at object/list registration boundaries. Kconfig may
select which objects are linked, but business logic should not branch on
feature macros.

Keep kernel/userspace changes aligned. A peripheral exposed to userspace should
add or update PCONFIG, PDM, UAPI, PDI, and Kconfig coverage together so the ABI
and build configuration remain consistent.

## Dependency Rules

- Core modules do not depend on product/application code.
- Dependencies point downward through the layer stack.
- Product-specific behavior belongs outside shared framework module directories.
- Kernel hardware configuration is selected through PCONFIG backends and
  consumed by PDM through the normalized device list, then mapped into LPF Core
  device configs.
- HAL should call LPF SoC Adapter APIs for SoC-backed hardware capabilities.
- Kernel-version conditionals belong in `kernel/lpf/compat/`.
- Userspace code must use PDI/UAPI rather than including kernel-internal HAL,
  PCONFIG, or PDM headers.
