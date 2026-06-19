# PDM

PDM is the Peripheral Driver Module. It contains high-level peripheral services
built on top of LPF Core, PCONFIG, HAL, OSAL, and the PDM-owned internal
protocol helpers.

## Current Scope

The kernel module currently provides:

- module load/unload orchestration in `pdm/src/pdm.c`
- PCONFIG query logic linked into `pdm.ko`
- PDM protocol package/parse helpers linked into `pdm.ko`; peripheral drivers
  pass a device type, message type, and payload to produce or parse standard
  protocol frames
- built-in peripheral-service registration through LPF Core
- configured-device binding and device removal ordering owned by LPF Core
- PDM MCU core, `/dev/pdm_mcu` ioctl dispatch, and CAN/Serial transport glue
  linked into `pdm.ko`
- PDM LED core and `/dev/pdm_led` ioctl dispatch for GPIO/PWM controlled LEDs
  linked into `pdm.ko`
- PDM procfs debug nodes under `/proc/pdm/`

PDM consumes exported `hal.ko` symbols for MCU transport and LED GPIO/PWM
hardware access.

## Configuration

```text
CONFIG_PDM=y
CONFIG_LPF_CORE=y
CONFIG_PCONFIG=y
CONFIG_PDM_MCU_SUPPORT=y
CONFIG_PDM_LED_SUPPORT=y
CONFIG_PDM_PROTOCOL=y
CONFIG_PDM_PROTOCOL_MCU=y
```

## Layout

```text
kernel/pdm/
├── Config.in
├── CMakeLists.txt
├── include/
│   ├── pdm_chrdev.h
│   ├── pdm_driver.h
│   ├── pdm_internal.h
│   ├── pdm_proc.h
│   └── pdm_status.h
└── src/
    ├── base/
    │   ├── pdm_chrdev.c
    │   ├── pdm_driver.c
    │   ├── pdm_proc.c
    │   └── pdm_status.c
    ├── pdm.c
    ├── mcu/
    │   ├── Config.in
    │   ├── pdm_mcu.c
    │   ├── pdm_mcu_chrdev.c
    │   ├── pdm_mcu_proc.c
    │   ├── pdm_mcu_can.c
    │   ├── pdm_mcu_serial.c
    │   └── pdm_mcu_internal.h
    └── led/
        ├── Config.in
        ├── pdm_led.c
        ├── pdm_led_chrdev.c
        ├── pdm_led_proc.c
        └── pdm_led_internal.h

kernel/include/pdm/
├── pdm.h
├── pdm_led.h
└── pdm_mcu.h
```

## Layering

`pdm.ko` owns userspace boundaries per peripheral. Built-in PDM peripheral
services register as LPF drivers through LPF Core; during module initialization
PDM loads PConfig, maps each enabled PConfig entry into an `lpf_device_config_t`,
and registers it with LPF Core. LPF Core then binds the configured device to the
matching service `probe`. On unload, LPF Core removes devices before driver
global resources are released.

Each PDM peripheral exposes
its own character device, such as `/dev/pdm_mcu`, and each PDI peripheral API
uses the matching UAPI ioctl header, such as `pdi_mcu.h`.

Procfs is reserved for debug and observability data. Current nodes:

- `/proc/pdm/mcu`
- `/proc/pdm/led`

Each node supports readback plus simple write commands for kernel-side
functional checks. Write commands return standard errno values and log command
results through the kernel log. For example:

- `echo "status 0" > /proc/pdm/mcu`
- `echo "cmd 0 0x10 0x01 0x02" > /proc/pdm/mcu`
- `echo "set 0 128" > /proc/pdm/led`
- `echo "enable 0" > /proc/pdm/led`

MCU transport APIs are linked into `pdm.ko`, but hardware access remains behind
HAL. `pdm.ko` depends on `lpf_core.ko` and `hal.ko`, and calls the HAL transport
symbols exported by `hal.ko`.

The protocol layer under `src/protocol/` is a common PDM-internal peripheral
communication protocol. It does not own module lifecycle; concrete peripheral
drivers such as MCU call it when they need framed communication.
