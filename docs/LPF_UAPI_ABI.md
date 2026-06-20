# LPF UAPI ABI Rules

LPF UAPI headers under `uapi/lpf/` define the stable ioctl ABI shared by LPF
kernel character devices and userspace PDI wrappers. PDI owns the
application-facing SDK; LPF UAPI owns ioctl commands, fixed-layout payloads,
and ABI version constants.

## Header Rules

- One peripheral owns one UAPI header: `lpf_<peripheral>.h`. Shared management
  or discovery nodes own explicit control headers such as `lpf_ctl.h`.
- The header must compile in both kernel and userspace builds.
- UAPI structures use Linux fixed-width types such as `__u32`, `__s32`, and
  `__u64`.
- UAPI structures must not expose kernel-only LPF HW, LPF runtime config, LPF Core, LPF
  peripheral, or OSAL internal types.
- UAPI headers must not expose PDI context types, helper functions, default
  device paths, or SDK open/discovery policy.
- Each peripheral owns a unique ioctl magic and command-number namespace.
- Ioctl payloads should be plain fixed-layout structs or fixed-width scalar
  types.

## ABI Versioning

- Each peripheral UAPI defines `LPF_<PERIPHERAL>_ABI_VERSION`.
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

- `CONFIG_COMPAT` handling stays local to each LPF peripheral character device.
- A peripheral may forward compat ioctl directly only when all ioctl payloads
  use fixed-width fields and pointer-free structures.
- If a future peripheral needs pointer-bearing or layout-dependent payloads, it
  must add an explicit compat translation path in that peripheral's chrdev
  implementation.

## Adding A Peripheral

Add the following pieces together:

- `uapi/lpf/lpf_<peripheral>.h`
- `user/pdi/include/pdi/<peripheral>.h`
- `user/pdi/src/pdi_<peripheral>.c`
- PDI aggregate include from `user/pdi/include/pdi/pdi.h`
- PDI default-open path constants when the SDK offers a default node.
- LPF peripheral character device implementation for
  `/dev/lpf/<peripheral><index>`
- `LPF_<PERIPHERAL>_IOC_GET_INFO`
- A unique `LPF_<PERIPHERAL>_IOC_MAGIC`

Management APIs follow the same ABI rules but should stay separate from
peripheral business commands. For example, `/dev/pdm_ctl` uses
`uapi/lpf/lpf_ctl.h` only for device discovery snapshots.

LPF v1 does not expose asynchronous userspace device events through the control
ABI. `/dev/pdm_ctl` remains a synchronous snapshot and lookup interface. Do not
add event ioctls, blocking reads, mmap queues, signal delivery, netlink, or
eventfd plumbing without a separate versioned event ABI plan covering buffering,
overflow, replay, filtering, and compatibility behavior.

## ABI Layout Checks

UAPI structure sizes, key field offsets, ABI versions, and ioctl command
encodings are checked by the CTest target under `tests/user/abi/`. Run:

```bash
make tests
```

When a UAPI layout changes intentionally, update the ABI version and the
matching layout assertions in the same change.
