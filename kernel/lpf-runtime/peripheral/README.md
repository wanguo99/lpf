# LPF Integrated Runtime

The LPF runtime is integrated into `lpf_core.ko` and hosts reusable LPF
peripheral services. Services stay layered under
`kernel/lpf-runtime/peripheral/` and remain integrated through the Core module
so deployment does not fragment into one KO per peripheral.

## Current Scope

The integrated runtime currently provides:

- runtime load/unload orchestration called from `lpf_core.ko`
- LPF runtime and configuration logic linked into `lpf_core.ko`
- LPF runtime initialization through `lpf_runtime_init()`
- configured-device binding and device removal ordering owned by LPF Core
- LPF MCU service, `/dev/lpf/mcuN` ioctl dispatch, and CAN/Serial transport
  glue linked into `lpf_core.ko`
- LPF LED service and `/dev/lpf/ledN` ioctl dispatch for GPIO/PWM controlled
  LEDs linked into `lpf_core.ko`
- LPF read-only procfs status nodes under `/proc/lpf/`
- LPF debugfs command nodes under `/sys/kernel/debug/lpf/`
- optional `lpf_dummy_service_selftest.ko` for mock-build LPF Core service
  lifecycle coverage

LPF peripheral services consume `lpf_hw_*` APIs for MCU transport and LED
GPIO/PWM hardware access. Those hardware access objects are linked into
`lpf_core.ko`.
Runtime character-device, sysfs-attribute, and debugfs-file lifecycle helpers
are provided by `lpf_core.ko`; LPF peripheral services own the concrete
operation handlers. LPF protocol encode/decode helpers are also provided by
`lpf_core.ko` for services that need framed communication.
LPF device discovery is provided by the LPF Core control node `/dev/lpf_ctl`.

## LPF Device Model

LPF Core is the LPF-owned device model / pseudo bus. It does not expose a separate physical bus layer, and peripheral services should not create one.
The current match rule is intentionally simple: one registered `lpf_driver_t`
owns one `lpf_device_type_t`, and `lpf_device_register()` binds a configured
device to the driver with the same type before calling `probe()`.

Configured-device probing follows this order:

1. `osal.ko` provides kernel OS abstraction services.
2. `lpf_configs.ko` exports the selected static board description.
3. `lpf_core.ko` initializes the LPF device model.
4. The integrated runtime initializes runtime core entries and LPF HW.
5. Peripheral services register their `lpf_driver_t` objects with LPF Core.
6. Runtime config loads the active board description and exposes device nodes.
7. Runtime walks enabled config nodes and dispatches them to peripheral config
   drivers.
8. Each peripheral config driver creates an `lpf_device_config_t` and calls
   `lpf_device_register()`.
9. LPF Core matches by LPF device type and calls the peripheral service
   `probe()`.

## Configuration

```text
CONFIG_LPF_RUNTIME=y
CONFIG_LPF_CONFIGS=m
CONFIG_LPF_CORE=m
CONFIG_LPF_MCU_SERVICE=y
CONFIG_LPF_MCU_MAX_DEVICES=4
CONFIG_LPF_MCU_TRANSPORT_CAN=y
CONFIG_LPF_MCU_TRANSPORT_UART=y
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
│   ├── lpf_mcu_transport.c
│   ├── lpf_mcu_can.c
│   ├── lpf_mcu_uart.c
│   ├── lpf_mcu_transport.h
│   └── lpf_mcu_internal.h
└── led/
    ├── Config.in
    ├── lpf_led_service.c
    ├── lpf_led_gpio.c
    ├── lpf_led_pwm.c
    ├── lpf_led_chrdev.c
    ├── lpf_led_proc.c
    └── lpf_led_internal.h
kernel/lpf-runtime/runtime/
├── lpf_runtime.c
├── lpf_runtime_config.c
├── lpf_runtime_entry_start.c
└── lpf_runtime_entry_end.c
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
└── ...
```

Runtime-private headers such as `lpf_runtime_internal.h` live under
`kernel/lpf-runtime/include/` and must not be included outside the runtime
module implementation.

## Layering

`lpf_core.ko` links the current framework-hosted LPF peripheral service paths.
During module initialization Core calls `lpf_runtime_init()` after Core global
state is ready. The runtime initializes LPF HW, walks the linked LPF peripheral
entry section, and initializes each entry. Feature objects such as MCU, LED, or
runtime selftests are selected with Kbuild `lpf_core-$(CONFIG_...)`; if an
object is not linked, its section entry is absent and the runtime has no
feature-specific branch to maintain. The runtime then loads runtime config,
walks enabled configured-device nodes, and dispatches each node to the matching
peripheral config driver. That config driver parses its typed node payload,
creates an `lpf_device_config_t`, and registers it with LPF Core. If entry
initialization fails, already initialized entries and LPF HW are unwound. If
configured-device probing fails, already registered devices are unregistered
before config and HW cleanup. LPF Core then binds the configured device to the
matching service `probe`. On unload, LPF Core removes devices before driver
global resources are released.

Each LPF peripheral instance exposes its own character device, such as
`/dev/lpf/mcu0`, and each PDI peripheral API uses the matching UAPI ioctl
header, such as `lpf_mcu.h`. Opening an instance character device acquires an
LPF Core active-device handle, and closing the file releases it. Device removal
therefore stops new opens first, waits for active instance handles to drain,
and then calls the peripheral service `remove` callback. The concrete node
implementation is shared through `lpf_chrdev`.

Instance node permissions are set by `CONFIG_LPF_INSTANCE_DEVNODE_MODE`, which
defaults to `0660`. Product builds should keep the framework default
conservative and assign group ownership with udev, devtmpfs policy, or their
init system. Development-only builds can override the mode when broad local
access is intentional.

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
add the object with `lpf_core-$(CONFIG_LPF_FEATURE) += ...` so Kconfig
selects whether the feature is linked into `lpf_core.ko`. Runtime self-tests
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

MCU transport implementations live with the MCU service under
`kernel/lpf-runtime/peripheral/mcu/` and are selected through
`lpf_mcu_transport_get()`. They remain MCU-owned implementation details linked
into `lpf_core.ko`; hardware access stays behind LPF HW.

The protocol layer under `kernel/lpf-core/protocol/` is a common LPF-owned
peripheral communication protocol linked into `lpf_core.ko`. It does not own
module lifecycle; concrete peripheral services such as MCU call it when they
need framed communication.

## Mock Selftest

Mock module builds can enable `CONFIG_LPF_DUMMY_SERVICE_SELFTEST=m` to build
`lpf_dummy_service_selftest.ko`. Loading it registers a temporary dummy driver
and device through LPF Core, then validates lifecycle events, discovery,
capability lookup, error/recovery state transitions, and unregister cleanup.
It is a test module only and is not linked into `lpf_core.ko`.
