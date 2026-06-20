# LPF Module Boundaries

## Dependency Direction

Dependencies should point downward through the LPF stack:

```text
Product → ACONFIG/PDI → UAPI → Peripheral Service → Core/Transport
        → LPF HW → SoC Adapter → Kernel Compat → Linux Kernel
```

Cross-layer shortcuts should be treated as architecture debt unless they are
explicitly documented as a transitional compatibility path.

## Allowed Dependencies

### PDI

- May include UAPI headers from `uapi/lpf/`.
- May use libc/POSIX and userspace OSAL helpers.
- Must not include kernel-internal LPF headers.

### UAPI

- May use Linux UAPI scalar types such as `__u32`.
- Must contain only fixed-layout structures, ABI constants, ioctl constants, and
  enum values that are part of the ABI.
- Must not expose SDK context types, helper functions, default-open paths, or
  kernel-private framework structures.

### LPF Peripheral Services

- May use LPF Core device/driver APIs.
- May use LPF HW APIs and LPF transport APIs.
- May use LPF runtime config typed entries as configuration input.
- May use shared LPF chrdev, sysfs, procfs, and debugfs helpers.
- Must not call SoC adapter, compat, vendor BSP, or Linux hardware subsystem APIs
  directly.

### LPF Transport

- May use LPF HW APIs.
- May consume normalized LPF runtime config transport fields.
- Must not call SoC adapter, compat, vendor BSP, or Linux hardware subsystem APIs
  directly.

### LPF Core

- May own LPF device/driver lifecycle, discovery, events, and shared runtime node
  helpers.
- May own reusable infrastructure wrappers for chrdev, sysfs, procfs, and
  debugfs.
- Must not depend on concrete runtime config backends or product tables.

### LPF HW

- May call LPF SoC Adapter APIs.
- May hold hardware capability state that is meaningful above a concrete SoC
  backend.
- Must not call Linux subsystem or vendor BSP APIs directly.

### LPF SoC Adapter

- May call LPF Kernel Compat APIs or vendor BSP APIs.
- Must keep SoC-specific code under `kernel/lpf/soc/`.
- Must not contain peripheral business behavior.

### LPF Kernel Compat

- May include Linux kernel headers and wrap kernel API differences.
- Should be the default location for `LINUX_VERSION_CODE` and equivalent
  version/feature handling.
- Must not contain peripheral business behavior.

### Runtime Config

- Owns backend selection, validation, and normalized device list generation.
- Backends may read static tables, Device Tree, module parameters, or future
  board-profile sources.
- LPF Core and peripheral services must consume normalized config output, not
  concrete backend symbols.

## Forbidden Patterns

- Peripheral service code calling `lpf_soc_*`, `lpf_compat_*`, `gpio_*`,
  `pwm_*`, `i2c_*`, `spi_*`, CAN socket helpers, TTY helpers, or vendor BSP APIs.
- Userspace code including `kernel/include/lpf/` headers other than installed
  UAPI copies.
- Product-specific business logic under shared framework service directories.
- New per-peripheral kernel modules when the integrated runtime boundary can
  host the reusable service.
- New global fixed-size registries when LPF Core device lifecycle or dynamic
  service-owned registries are sufficient.

## Transitional Allowances

- Instance character device arrays may retain configured maxima while UAPI info
  still reports `max_devices`.
- ABI or protocol constants may remain fixed-size when changing them would break
  userspace or wire compatibility.
- Procfs/debugfs/sysfs helpers may live in LPF Core while compat wrappers for
  their API-version differences are still being defined.
