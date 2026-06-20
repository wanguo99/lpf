# LPF Peripheral Runtime

The LPF peripheral runtime is the integrated kernel module that hosts reusable
LPF peripheral services. Services stay layered under `kernel/lpf/peripheral/`
and remain integrated through one runtime module so deployment does not
fragment into one KO per peripheral.

## Current Scope

The runtime module currently provides:

- module load/unload orchestration in `lpf_peripheral_module.c`
- LPF peripheral runtime and configuration logic linked into
  `lpf_peripheral_runtime.ko`
- LPF peripheral runtime initialization through `lpf_peripheral_runtime_init()`
- configured-device binding and device removal ordering owned by LPF Core
- LPF MCU service, `/dev/lpf/mcuN` ioctl dispatch, and CAN/Serial transport
  glue linked into `lpf_peripheral_runtime.ko`
- LPF LED service and `/dev/lpf/ledN` ioctl dispatch for GPIO/PWM controlled
  LEDs linked into `lpf_peripheral_runtime.ko`
- LPF read-only procfs status nodes under `/proc/lpf/`
- LPF debugfs command nodes under `/sys/kernel/debug/lpf/`

LPF peripheral services consume `lpf_hw_*` APIs for MCU transport and LED
GPIO/PWM hardware access. Those hardware access objects are linked into
`lpf_peripheral_runtime.ko`.
Runtime character-device, sysfs-attribute, and debugfs-file lifecycle helpers
are provided by `lpf_core.ko`; LPF peripheral services own the concrete
operation handlers. LPF protocol encode/decode helpers are also provided by
`lpf_core.ko` for services that need framed communication.
LPF device discovery is provided by the LPF Core control node `/dev/lpf_ctl`.

## Configuration

```text
CONFIG_LPF_PERIPHERAL_RUNTIME=y
CONFIG_LPF_CORE=y
CONFIG_LPF_MCU_SERVICE=y
CONFIG_LPF_MCU_MAX_DEVICES=4
CONFIG_LPF_LED_SERVICE=y
CONFIG_LPF_LED_MAX_DEVICES=8
CONFIG_LPF_PROTOCOL=y
CONFIG_LPF_PROTOCOL_MCU=y
```

## Layout

```text
kernel/lpf/core/
├── lpf_ctl.c
├── lpf_ctl_internal.h
└── ...
kernel/lpf/peripheral/
├── Config.in
├── lpf_peripheral.c
├── lpf_peripheral_config.c
├── lpf_peripheral_internal.h
├── lpf_peripheral_module.c
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
kernel/lpf/transport/
└── mcu/
    ├── Config.in
    ├── lpf_mcu_transport.c
    ├── lpf_mcu_transport_can.c
    └── lpf_mcu_transport_uart.c
kernel/lpf/protocol/
├── lpf_protocol.c
├── lpf_protocol_common.c
└── lpf_protocol_internal.h

kernel/include/lpf/
├── lpf_errno.h
├── lpf_peripheral.h
├── lpf_led_service.h
├── lpf_mcu_service.h
├── lpf_proc.h
├── config/
│   └── lpf_config*.h
├── protocol/
│   ├── lpf_protocol.h
│   └── lpf_protocol_mcu.h
├── transport/
│   └── mcu/
│       └── lpf_mcu_transport.h
└── ...
```

## Layering

`lpf_peripheral_runtime.ko` links the current framework-hosted LPF peripheral
service paths. During module initialization the runtime module calls
`lpf_peripheral_runtime_init()`. The LPF
peripheral runtime initializes LPF Core, registers peripheral services, loads
runtime config, maps each enabled normalized runtime config device entry into an
`lpf_device_config_t`, and registers it with LPF Core. The module entry does not
depend on the concrete runtime config backend, service registration order, or
per-device capability mapping. LPF Core then binds the configured device to the
matching service `probe`. On unload, LPF Core removes devices before driver
global resources are released.

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
fields; they should not be treated as the primary LPF device model.

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
the caller and are not treated as peripheral runtime health changes. Successful
runtime operations recover an instance from `ERROR` to `BOUND`; `last_error`
and `error_count` remain as historical diagnostics. MCU status queries also map
reported `OFFLINE` and `ERROR` MCU states into LPF Core runtime health.

MCU and LED service implementations live under `kernel/lpf/peripheral/`.
They are registered through the LPF peripheral runtime while the framework
module boundary is being cleaned up.

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

MCU transport implementations live under `kernel/lpf/transport/mcu/` and are
selected through `lpf_mcu_transport_get()`. They are still linked into
`lpf_peripheral_runtime.ko`, but the MCU service no longer directly depends on
CAN or UART implementation symbols. Hardware access remains behind LPF HW.

The protocol layer under `kernel/lpf/protocol/` is a common LPF-owned
peripheral communication protocol linked into `lpf_core.ko`. It does not own
module lifecycle; concrete peripheral services such as MCU call it when they
need framed communication.
