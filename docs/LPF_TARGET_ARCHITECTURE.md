# LPF Target Architecture

## Positioning

LPF is a Linux peripheral business abstraction framework for multi-kernel,
multi-SoC, and multi-product deployments. The framework keeps a stable
upper-layer peripheral model and pushes Linux kernel, SoC, and vendor BSP
differences into lower adapter layers.

LPF does not use the Linux native device model as its only internal center.
Linux device nodes, sysfs, debugfs, procfs, and kernel subsystem APIs are
integration surfaces below or beside the LPF-owned model.

## Target Stack

```text
Application / Product
        ↓
ACONFIG
        ↓
PDI - userspace SDK
        ↓
LPF UAPI / ioctl
        ↓
LPF Peripheral Service Layer
        ↓
LPF Core
        ↓
LPF Transport Layer
        ↓
LPF HW API
        ↓
LPF SoC Adapter Layer
        ↓
LPF Kernel Compat Layer
        ↓
Linux Kernel / Vendor BSP / Hardware
```

## Layer Responsibilities

- ACONFIG owns userspace application-facing product metadata.
- PDI owns userspace C APIs and hides ioctl details from applications.
- UAPI owns fixed ABI structures, ioctl constants, and ABI version constants.
- LPF Peripheral Services own reusable MCU, LED, and future peripheral business
  behavior.
- LPF Core owns the LPF device and driver model, lifecycle, discovery, kernel
  event notification, and shared runtime node helpers.
- LPF Transport owns reusable communication backends such as MCU CAN and UART.
- LPF HW owns framework hardware capability APIs consumed by services and
  transports.
- LPF SoC Adapter owns SoC and vendor BSP backend differences.
- LPF Kernel Compat owns Linux kernel API-version differences.

## Runtime Boundaries

The current module boundary is:

```text
osal.ko
lpf_core.ko
lpf_peripheral_runtime.ko
```

`lpf_core.ko` hosts the LPF device model, discovery control node, shared
chrdev/sysfs/debugfs/proc helpers, protocol helpers, kernel compat wrappers, and
the selected SoC adapter.

`lpf_peripheral_runtime.ko` hosts runtime configuration, LPF HW objects,
peripheral services, transports, and configured-device probing. It should remain
an integrated framework runtime instead of splitting one kernel module per
peripheral service.

## Runtime Access Model

- `/dev/pdm_ctl` is the Core-owned control and discovery node.
- `/dev/pdm_ctl` exposes synchronous device snapshots; LPF v1 keeps device
  event notification kernel-only.
- `/dev/lpf/<type><index>` is the per-instance business ABI.
- Sysfs attributes are read-only runtime inspection data.
- Debugfs files are debug command channels and are not stable product ABI.
- Procfs files are read-only service status snapshots.

## Extension Model

Adding a new peripheral family should add matching pieces together:

- Runtime config type and backend parsing.
- LPF device type and capability mapping.
- Peripheral service driver registered with LPF Core.
- Optional transport implementation under `kernel/lpf/transport/`.
- Optional LPF HW capability if the peripheral needs a reusable hardware API.
- UAPI header when userspace access is needed.
- PDI wrapper when application-facing C APIs are needed.
- Tests covering ABI layout and operation marshaling.
