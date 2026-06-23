# Kernel And Userspace Split

PDM is Linux-only in its current direction. Kernel modules own device binding,
hardware access, and ioctl implementations. Userspace libraries own SDK-facing
APIs and ioctl marshaling.

## Current Layout

```text
kernel/
  include/
    osal/          # kernel OSAL headers
    pdm/           # kernel-internal PDM headers
      compat/      # kernel API feature helpers
      core/        # PDM bus, device, client, backend APIs
  osal/            # builds osal.ko
  pdm/
    bus/           # Linux bus_type and PDM device registration
    core/          # module lifecycle, ctl/client nodes, registries, fs helpers
    peripheral/    # MCU, LED, and future peripheral drivers/backends
    mock/          # synthetic test devices

uapi/
  pdm/             # ioctl ABI shared by kernel nodes and PDI

user/
  osal/            # userspace OSAL library
  aconfig/         # userspace application configuration
  pdi/             # userspace API library for PDM
```

## Responsibilities

- `kernel/osal` wraps kernel APIs and builds `osal.ko`.
- `kernel/pdm` builds `pdm.ko` and owns all in-kernel PDM device model,
  driver, backend, control, and instance-node behavior.
- `kernel/pdm/peripheral` owns reusable kernel peripheral drivers and their
  optional backend files.
- `uapi/pdm` owns fixed ioctl ABI definitions.
- `user/pdi` owns application-facing C APIs and hides open/ioctl details.
- `user/aconfig` remains userspace-only application configuration support.

## Boundary Rules

- Kernel code may include `kernel/include/<module>/` and generated headers.
- Userspace code may include `user/<module>/include/` and `uapi/` headers.
- Userspace code must not include kernel-internal PDM headers.
- UAPI headers must not depend on kernel-only types or private framework
  structures.
- PDI should marshal data and call ioctl; it should not duplicate kernel driver
  or backend behavior.
- Product code should call PDI and ACONFIG instead of reaching into kernel
  framework internals.

## Device Access Flow

```text
Application
    -> PDI userspace API
    -> /dev/pdm_ctl or /dev/pdm/<peripheral><index> ioctl
    -> PDM peripheral driver
    -> PDM backend or Linux subsystem
```

Device creation is kernel-side:

```text
Device Tree or mock config
    -> pdm_device_register()
    -> PDM bus match
    -> pdm_driver probe()
    -> optional /dev/pdm/<peripheral><index> node
```

Each userspace-visible peripheral should have a matching UAPI header and PDI
wrapper. For example, MCU uses `/dev/pdm/mcu0`, `uapi/pdm/pdm_mcu.h`, and
`user/pdi/src/pdi_mcu.c`.

The previous userspace test product was removed. New tests live under `tests/`
with explicit CMake/CTest integration. `make tests` currently covers UAPI layout
and PDI ioctl marshaling through a syscall mock boundary.
