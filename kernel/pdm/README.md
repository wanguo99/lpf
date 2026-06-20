# PDM

PDM is the Peripheral Driver Module. It currently owns module orchestration,
LPF device registration, and the management/discovery node. LPF peripheral
services are layered under `kernel/lpf/peripheral/` but remain integrated
through `pdm.ko` during the current migration stage so runtime deployment does
not fragment into one KO per peripheral.

## Current Scope

The kernel module currently provides:

- module load/unload orchestration in `pdm/src/pdm.c`
- PCONFIG normalized-device query logic linked into `pdm.ko`
- LPF peripheral-service registration through `lpf_peripheral_services_init()`
- configured-device binding and device removal ordering owned by LPF Core
- LPF MCU service, `/dev/lpf/mcuN` ioctl dispatch, and CAN/Serial transport
  glue linked into `pdm.ko`
- LPF LED service and `/dev/lpf/ledN` ioctl dispatch for GPIO/PWM controlled
  LEDs linked into `pdm.ko`
- PDM control node `/dev/pdm_ctl` for LPF device discovery snapshots
- LPF read-only procfs status nodes under `/proc/lpf/`
- LPF debugfs command nodes under `/sys/kernel/debug/lpf/`

PDM consumes exported `hal.ko` symbols for MCU transport and LED GPIO/PWM
hardware access. Runtime character-device, sysfs-attribute, and debugfs-file
lifecycle helpers are provided by `lpf_core.ko`; LPF peripheral services own
the concrete operation handlers. LPF protocol encode/decode helpers are also
provided by `lpf_core.ko` for services that need framed communication.

## Configuration

```text
CONFIG_PDM=y
CONFIG_LPF_CORE=y
CONFIG_PCONFIG=y
CONFIG_LPF_MCU_SERVICE=y
CONFIG_LPF_MCU_MAX_DEVICES=4
CONFIG_LPF_LED_SERVICE=y
CONFIG_LPF_LED_MAX_DEVICES=8
CONFIG_LPF_PROTOCOL=y
CONFIG_LPF_PROTOCOL_MCU=y
```

## Layout

```text
kernel/pdm/
├── Config.in
├── CMakeLists.txt
├── include/
│   └── pdm_ctl.h
└── src/
    ├── base/
    │   └── pdm_ctl_chrdev.c
    └── pdm.c
kernel/lpf/peripheral/
├── lpf_peripheral.c
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
├── lpf_led_service.h
├── lpf_mcu_service.h
├── lpf_mcu_transport.h
├── lpf_proc.h
├── lpf_protocol.h
├── lpf_protocol_mcu.h
└── ...

kernel/include/pdm/
└── pdm.h
```

## Layering

`pdm.ko` links the current framework-hosted LPF peripheral service paths.
LPF peripheral services register as LPF drivers through LPF Core via
`lpf_peripheral_services_init()`. During module initialization PDM loads
PConfig, maps each enabled normalized PConfig device entry into an
`lpf_device_config_t`, and registers it with LPF Core. PDM must not depend on
the concrete PCONFIG backend that produced the entries. LPF Core then binds the
configured device to the matching service `probe`. On unload, LPF Core removes
devices before driver global resources are released.

Each LPF peripheral instance exposes its own character device, such as
`/dev/lpf/mcu0`, and each PDI peripheral API uses the matching UAPI ioctl
header, such as `lpf_mcu.h`. Opening an instance character device acquires an
LPF Core active-device handle, and closing the file releases it. Device removal
therefore stops new opens first, waits for active instance handles to drain,
and then calls the peripheral service `remove` callback. The concrete node
implementation is shared through `lpf_chrdev`.

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

`last_error` and `error_count` are updated from runtime ioctl and debugfs
command failures for the specific instance.

MCU and LED service implementations live under `kernel/lpf/peripheral/`.
They are registered through the LPF peripheral service entry while the
framework module boundary is being cleaned up.

`/dev/pdm_ctl` is the management node for discovery. It exposes LPF Core device
snapshots through `uapi/lpf/lpf_ctl.h`, including stable name, type, state,
driver name, capability flags, `last_error`, and `error_count`. It does not
perform peripheral business operations.

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
`pdm.ko`, but the MCU service no longer directly depends on CAN or UART
implementation symbols. Hardware access remains behind HAL.

The protocol layer under `kernel/lpf/protocol/` is a common LPF-owned
peripheral communication protocol linked into `lpf_core.ko`. It does not own
module lifecycle; concrete peripheral services such as MCU call it when they
need framed communication.
