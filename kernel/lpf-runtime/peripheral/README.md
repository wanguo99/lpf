# LPF Runtime

The LPF runtime is the integrated kernel module that hosts reusable
LPF peripheral services. Services stay layered under `kernel/lpf-runtime/peripheral/`
and remain integrated through one runtime module so deployment does not
fragment into one KO per peripheral.

## Current Scope

The runtime module currently provides:

- module load/unload orchestration in `kernel/lpf-runtime/runtime/lpf_runtime_module.c`
- LPF runtime and configuration logic linked into
  `lpf_runtime.ko`
- LPF runtime initialization through `lpf_runtime_init()`
- configured-device binding and device removal ordering owned by LPF Core
- LPF MCU service, `/dev/lpf/mcuN` ioctl dispatch, and CAN/Serial transport
  glue linked into `lpf_runtime.ko`
- LPF LED service and `/dev/lpf/ledN` ioctl dispatch for GPIO/PWM controlled
  LEDs linked into `lpf_runtime.ko`
- LPF read-only procfs status nodes under `/proc/lpf/`
- LPF debugfs command nodes under `/sys/kernel/debug/lpf/`
- optional `lpf_dummy_service_selftest.ko` for mock-build LPF Core service
  lifecycle coverage

LPF peripheral services consume `lpf_hw_*` APIs for MCU transport and LED
GPIO/PWM hardware access. Those hardware access objects are linked into
`lpf_runtime.ko`.
Runtime character-device, sysfs-attribute, and debugfs-file lifecycle helpers
are provided by `lpf_core.ko`; LPF peripheral services own the concrete
operation handlers. LPF protocol encode/decode helpers are also provided by
`lpf_core.ko` for services that need framed communication.
LPF device discovery is provided by the LPF Core control node `/dev/lpf_ctl`.

## Configuration

```text
CONFIG_LPF_RUNTIME=m
CONFIG_LPF_CORE=m
CONFIG_LPF_MCU_SERVICE=y
CONFIG_LPF_MCU_MAX_DEVICES=4
CONFIG_LPF_LED_SERVICE=y
CONFIG_LPF_LED_MAX_DEVICES=8
CONFIG_LPF_PROTOCOL=y
CONFIG_LPF_PROTOCOL_MCU=y
```

## Layout

```text
kernel/lpf-core/core/
├── lpf_ctl.c
├── lpf_ctl_internal.h
└── ...
kernel/lpf-runtime/peripheral/
├── Config.in
├── selftest/
│   └── lpf_dummy_service_selftest.c
├── mcu/
│   ├── Config.in
│   ├── lpf_mcu_service.c
│   ├── lpf_mcu_chrdev.c
│   ├── lpf_mcu_proc.c
│   └── lpf_mcu_internal.h
└── led/
    ├── Config.in
    ├── lpf_led_service.c
    ├── lpf_led_chrdev.c
    ├── lpf_led_proc.c
    └── lpf_led_internal.h
kernel/lpf-runtime/runtime/
├── lpf_runtime.c
├── lpf_runtime_config.c
├── lpf_runtime_entry_start.c
├── lpf_runtime_entry_end.c
└── lpf_runtime_module.c
kernel/lpf-runtime/transport/
└── mcu/
    ├── Config.in
    ├── lpf_mcu_transport.c
    ├── lpf_mcu_transport_can.c
    └── lpf_mcu_transport_uart.c
kernel/lpf-core/protocol/
├── lpf_protocol.c
├── lpf_protocol_common.c
└── lpf_protocol_internal.h

kernel/include/lpf/
├── lpf_errno.h
├── core/
│   ├── lpf_core.h
│   ├── lpf_device.h
│   ├── lpf_driver.h
│   ├── lpf_chrdev.h
│   ├── lpf_proc.h
│   └── lpf_debugfs.h
├── config/
│   └── lpf_config*.h
├── runtime/
│   └── lpf_runtime.h
├── peripheral/
│   ├── led/
│   │   └── lpf_led_service.h
│   └── mcu/
│       └── lpf_mcu_service.h
├── protocol/
│   ├── lpf_protocol.h
│   └── lpf_protocol_mcu.h
├── transport/
│   └── mcu/
│       └── lpf_mcu_transport.h
└── ...
```

Runtime-private headers such as `lpf_runtime_internal.h` live under
`kernel/lpf-runtime/include/` and must not be included outside the runtime
module implementation.

## Layering

`lpf_runtime.ko` links the current framework-hosted LPF peripheral
service paths. During module initialization the runtime module calls
`lpf_runtime_init()`. The LPF
runtime initializes LPF Core, walks the linked LPF peripheral entry
section, and initializes each entry. Feature objects such as MCU, LED, or
runtime selftests are selected with Kbuild `obj-$(CONFIG_...)`; if an object is
not linked, its section entry is absent and the runtime has no feature-specific
branch to maintain. The runtime then loads runtime config, maps each enabled
normalized runtime config device entry into an `lpf_device_config_t`, and
registers it with LPF Core. The module entry does not depend on the concrete
runtime config backend, service registration order, or per-device capability
mapping. LPF Core then binds the configured device to the matching service
`probe`. On unload, LPF Core removes devices before driver global resources are
released.

Each LPF peripheral instance exposes its own character device, such as
`/dev/lpf/mcu0`, and each PDI peripheral API uses the matching UAPI ioctl
header, such as `lpf_mcu.h`. Opening an instance character device acquires an
LPF Core active-device handle, and closing the file releases it. Device removal
therefore stops new opens first, waits for active instance handles to drain,
and then calls the peripheral service `remove` callback. The concrete node
implementation is shared through `lpf_chrdev`.

MCU and LED services keep their runtime operation contexts in service-owned
dynamic registries keyed by LPF device index. The current
`CONFIG_LPF_MCU_MAX_DEVICES` and `CONFIG_LPF_LED_MAX_DEVICES` limits still size
the userspace-visible instance-node tables and the legacy `max_devices` info
fields; they should not be treated as the primary LPF device model. Service
probe paths do not enforce these node limits directly; instance-node
registration owns that boundary and cleans up the service context if a
configured device cannot be exposed as a character device.

Instance character devices expose read-only sysfs attributes for inspection:

- `name`
- `type`
- `index`
- `state`
- `capabilities`
- `driver`
- `soc`
- `last_error`
- `error_count`
- `open_count`

`state`, `last_error`, and `error_count` are updated from runtime ioctl and
debugfs operation failures for the specific instance through LPF Core. Caller
ABI failures such as unsupported commands or malformed arguments are returned to
the caller and are not treated as runtime health changes. Successful
runtime operations recover an instance from `ERROR` to `BOUND`; `last_error`
and `error_count` remain as historical diagnostics. MCU status queries also map
reported `OFFLINE` and `ERROR` MCU states into LPF Core runtime health.

MCU and LED service implementations live under `kernel/lpf-runtime/peripheral/`.
They contribute classed entries to the LPF runtime section. New
framework-hosted services should declare a file-local runtime entry with
`lpf_runtime_service_register(feature_name, feature_init, feature_exit)` and
add the object with `lpf_runtime-$(CONFIG_LPF_FEATURE) += ...` so Kconfig
selects whether the feature is linked into `lpf_runtime.ko`. Runtime self-tests
should use `lpf_runtime_selftest_register(...)` so they run after services.

`/dev/lpf_ctl` is the management node for discovery. It is implemented by LPF
Core and exposes LPF Core device snapshots through `uapi/lpf/lpf_ctl.h`,
including stable name, type, state, driver name, capability flags,
`last_error`, and `error_count`. It does not perform peripheral business
operations.

Procfs is reserved for read-only observability data. Current nodes:

- `/proc/lpf/mcu`
- `/proc/lpf/led`

Debug-only operations are exposed through debugfs so they are separate from
stable userspace ABI and read-only status files. Current command nodes:

- `/sys/kernel/debug/lpf/mcu`
- `/sys/kernel/debug/lpf/led`

Debugfs write commands return standard errno values and log command results
through the kernel log. Debugfs file creation and root reference counting are
shared through `lpf_debugfs`; LPF services supply the `mcu` and `led` command
handlers.
For example:

- `echo "status 0" > /sys/kernel/debug/lpf/mcu`
- `echo "cmd 0 0x10 0x01 0x02" > /sys/kernel/debug/lpf/mcu`
- `echo "set 0 128" > /sys/kernel/debug/lpf/led`
- `echo "enable 0" > /sys/kernel/debug/lpf/led`

MCU transport implementations live under `kernel/lpf-runtime/transport/mcu/` and are
selected through `lpf_mcu_transport_get()`. They are still linked into
`lpf_runtime.ko`, but the MCU service no longer directly depends on
CAN or UART implementation symbols. Hardware access remains behind LPF HW.

The protocol layer under `kernel/lpf-core/protocol/` is a common LPF-owned
peripheral communication protocol linked into `lpf_core.ko`. It does not own
module lifecycle; concrete peripheral services such as MCU call it when they
need framed communication.

## Mock Selftest

Mock module builds can enable `CONFIG_LPF_DUMMY_SERVICE_SELFTEST=m` to build
`lpf_dummy_service_selftest.ko`. Loading it registers a temporary dummy driver
and device through LPF Core, then validates lifecycle events, discovery,
capability lookup, error/recovery state transitions, and unregister cleanup.
It is a test module only and is not linked into `lpf_runtime.ko`.
