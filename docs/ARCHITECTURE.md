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
LPF peripheral services + LPF protocol
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
- LPF peripheral services provide kernel-side peripheral business behavior.
- LPF protocol helpers provide kernel-side packet framing for services that use
  framed peripheral communication.
- LPF Core owns the control/discovery node for device snapshots.
- LPF peripheral runtime owns the integrated framework module entry point.
- PDI provides userspace APIs over LPF UAPI ioctl nodes.
- ACONFIG stores userspace application-facing configuration mappings.

## Layer Responsibilities

### OSAL

OSAL hides Linux kernel or libc/POSIX API differences from framework code. The
kernel and userspace implementations are separate, but public names should match
where the semantics are equivalent.

### HAL

HAL owns LPF hardware-capability APIs in kernel mode. Current HAL support
includes CAN, serial, GPIO, PWM, I2C, and SPI. HAL calls the LPF SoC Adapter
instead of Linux subsystem APIs directly. LPF peripheral services call exported
HAL symbols; userspace does not include or call HAL directly.

### LPF Core

LPF Core owns the framework-level device model. Peripheral services register as
LPF drivers, and configured device instances are registered as LPF devices. LPF
Core handles bind, probe, remove ordering, stable device names, and capability
metadata. It does not depend on PCONFIG; callers map configuration backends into
LPF device configs before registration. LPF Core also initializes the default
SoC adapter path used by HAL. Device discovery callers should use the snapshot
APIs (`lpf_device_list()`, `lpf_device_get_info_by_name()`, and capability
queries) instead of retaining internal `lpf_device_t` pointers across lifecycle
events. Kernel code that needs to keep a device active across operations should
use `lpf_device_get()` or the name/capability variants and release the returned
handle with `lpf_device_put()`. LPF Core emits kernel device events for
registration, bind, state changes, errors, remove start, and remove completion.
It also provides reusable kernel infrastructure helpers for LPF instance
character devices, instance sysfs attributes, and debugfs command files so
peripheral services do not duplicate node lifecycle code.
The LPF control/discovery node `/dev/pdm_ctl` is implemented in LPF Core and
exposes read-only snapshots of the LPF device model through
`uapi/lpf/lpf_ctl.h`.

### LPF SoC Adapter

The SoC adapter layer owns hardware backend differences below HAL. The current
default backend is `generic-linux`, which calls LPF kernel compat wrappers for
CAN, serial, GPIO, PWM, I2C, and SPI. Kconfig selects the default backend built
into `lpf_core.ko`; `kernel/lpf/soc/mock/` provides a deterministic mock backend
for development and framework tests that should not require live hardware. The
mock preset can also build `hal_mock_selftest.ko`, which runs HAL operation-path
checks over the mock backend when loaded. Future SoC-specific adapters should
live under `kernel/lpf/soc/` and must keep vendor BSP calls out of HAL and LPF
peripheral services.

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
`pconfig_platform_config_t` and `pconfig_device_config_t` model before LPF
peripheral configuration sees the data.

### LPF Peripheral Services

LPF peripheral services own kernel-side peripheral business behavior, ioctl
dispatch, state, error handling, and debug command handlers. Services register
with LPF Core for device lifecycle handling and use LPF chrdev/sysfs/debugfs
helpers for runtime nodes. MCU and LED services now live under
`kernel/lpf/peripheral/` and expose instance nodes such as `/dev/lpf/mcu0` and
`/dev/lpf/led0`; they remain integrated through
`lpf_peripheral_runtime.ko` rather than being split into one KO per peripheral.
`kernel/lpf/peripheral/lpf_peripheral.c` owns the unified peripheral runtime
entry used by the integration module.

### LPF Transports

LPF transport implementations own reusable communication backends used by
peripheral services. The current MCU CAN and UART transports live under
`kernel/lpf/transport/mcu/` and are selected by normalized PCONFIG interface
type. MCU service code uses the transport registry instead of depending on
CAN/UART implementation symbols directly.

### LPF Protocol

LPF protocol helpers own reusable kernel-side packet framing for peripheral
services that need a standard message envelope. The current implementation
lives under `kernel/lpf/protocol/`, exports encode/decode entry points from
`lpf_core.ko`, and keeps protocol constants and MCU message definitions under
`kernel/include/lpf/`.

### LPF Peripheral Runtime

`lpf_peripheral_runtime.ko` owns the current integration module load/unload
entry points. It calls the LPF peripheral runtime entry, which owns LPF Core
initialization order, per-service registration, configured-device probing, and
PCONFIG-to-LPF mapping orchestration.
Business operations stay on LPF instance nodes such as `/dev/lpf/mcu0` and
`/dev/lpf/led0`; LPF service status snapshots live under `/proc/lpf/`.

### UAPI

UAPI headers define the stable ioctl ABI shared between LPF kernel nodes and
PDI. They must remain valid for both kernel and userspace compilation and
should avoid kernel-internal types.

### PDI

PDI is the userspace C API layer. It opens the matching `/dev/lpf/*` node,
marshals requests through UAPI ioctls, and hides ioctl details from
applications. Discovery APIs use the LPF control node `/dev/pdm_ctl` to list
LPF device snapshots and look up devices by stable name or capability.

### ACONFIG

ACONFIG provides application-facing configuration mapping. It is separate from
PCONFIG so application function metadata does not leak into hardware tables.

## Current Peripheral Scope

The current framework keeps one concrete peripheral/device family:

- MCU configuration in PCONFIG
- MCU protocol in LPF protocol
- MCU service in LPF peripheral layer
- MCU CAN/UART transport in LPF transport layer
- Userspace access through PDI
- LED configuration in PCONFIG
- LED service in LPF peripheral layer
- Userspace access through PDI

Other peripheral families can be added later by introducing matching PCONFIG
types, LPF peripheral-service implementations, PDI userspace API coverage, UAPI
definitions when needed, and Kconfig entries. Protocol definitions are only
needed for peripherals that use framed LPF protocol communication.

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
lpf_peripheral_runtime.ko
```

`lpf_peripheral_runtime.ko` hosts the current LPF peripheral runtime. The
runtime consumes PConfig entries and HAL symbols, registers the current
framework-hosted peripheral services, maps enabled PConfig entries to LPF
devices, and lets LPF Core probe the matching registered service for each
configured peripheral.

## Adding A Peripheral

Add the following pieces together:

- PCONFIG type and platform config array entry.
- LPF device type and capability mapping for the PCONFIG entry.
- LPF service driver object registered with `lpf_driver_register`.
- Optional LPF instance character device `/dev/lpf/<peripheral><index>` when
  userspace access is needed.
- UAPI header `uapi/lpf/lpf_<peripheral>.h` following
  `docs/LPF_UAPI_ABI.md`.
- Userspace wrapper `user/pdi/src/pdi_<peripheral>.c`.
- Kbuild object selection for the new LPF service inside
  `lpf_peripheral_runtime.ko`.

Feature selection stays at object/list registration boundaries. Kconfig may
select which objects are linked, but business logic should not branch on
feature macros.

Keep kernel/userspace changes aligned. A peripheral exposed to userspace should
add or update PCONFIG, LPF peripheral service, UAPI, PDI, and Kconfig coverage
together so the ABI and build configuration remain consistent.

## Runtime Interfaces

- `/dev/pdm_ctl` is the LPF Core-owned management/discovery node.
- `/dev/lpf/<peripheral><index>` nodes are the stable per-instance business ABI.
- `/sys/class/misc/<device>/` attributes are read-only per-instance sysfs
  inspection data, including runtime `last_error` and `error_count`.
- LPF discovery snapshots report the same runtime `last_error` and
  `error_count` values for management clients.
- `/proc/lpf/*` nodes are read-only LPF service status snapshots.
- `/sys/kernel/debug/lpf/*` nodes are debug-only command entry points and must
  not be treated as stable product ABI.

## Dependency Rules

- Core modules do not depend on product/application code.
- Dependencies point downward through the layer stack.
- Product-specific behavior belongs outside shared framework module directories.
- Kernel hardware configuration is selected through PCONFIG backends and
  consumed by LPF peripheral configuration through the normalized device list,
  then mapped into LPF Core device configs.
- HAL should call LPF SoC Adapter APIs for SoC-backed hardware capabilities.
- Kernel-version conditionals belong in `kernel/lpf/compat/`.
- Userspace code must use PDI/UAPI rather than including kernel-internal HAL,
  PCONFIG, LPF Core, or LPF peripheral headers.
