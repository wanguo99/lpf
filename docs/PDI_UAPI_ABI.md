# PDI UAPI ABI Rules

PDI UAPI headers under `uapi/pdi/` define the stable ioctl ABI shared by
kernel PDM character devices and userspace PDI wrappers.

## Header Rules

- One peripheral owns one UAPI header: `pdi_<peripheral>.h`. Shared management
  or discovery nodes own explicit control headers such as `pdi_ctl.h`.
- The header must compile in both kernel and userspace builds.
- UAPI structures use Linux fixed-width types such as `__u32`, `__s32`, and
  `__u64`.
- UAPI structures must not expose kernel-only HAL, PConfig, PDM, or OSAL
  internal types.
- Each peripheral owns a unique ioctl magic and command-number namespace.
- Ioctl payloads should be plain fixed-layout structs or fixed-width scalar
  types.

## ABI Versioning

- Each peripheral UAPI defines `PDI_<PERIPHERAL>_ABI_VERSION`.
- Each peripheral provides a `GET_INFO` ioctl returning the ABI version and
  module version fields.
- ABI version format is `0xMMMMmmmm`, where `MMMM` is the major ABI and `mmmm`
  is the minor ABI.
- Backward-compatible additions increment the minor ABI.
- Incompatible structure layout, command semantics, or ioctl numbering changes
  increment the major ABI.
- Existing ioctl command numbers and structure field meanings are append-only
  once released.

## Compatibility

- `CONFIG_COMPAT` handling stays local to each PDM peripheral character device.
- A peripheral may forward compat ioctl directly only when all ioctl payloads
  use fixed-width fields and pointer-free structures.
- If a future peripheral needs pointer-bearing or layout-dependent payloads, it
  must add an explicit compat translation path in that peripheral's chrdev
  implementation.

## Adding A Peripheral

Add the following pieces together:

- `uapi/pdi/pdi_<peripheral>.h`
- `user/pdi/src/pdi_<peripheral>.c`
- PDI aggregate include from `user/pdi/include/pdi/pdi.h`
- PDM character device implementation for `/dev/pdm_<peripheral>`
- `PDI_<PERIPHERAL>_IOC_GET_INFO`
- A unique `PDI_<PERIPHERAL>_IOC_MAGIC`

Management APIs follow the same ABI rules but should stay separate from
peripheral business commands. For example, `/dev/pdm_ctl` uses
`uapi/pdi/pdi_ctl.h` only for device discovery snapshots.
