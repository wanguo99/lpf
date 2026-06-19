# PDM

PDM is the Peripheral Driver Module. It contains high-level peripheral drivers
built on top of PCONFIG, HAL, OSAL, and the PDM-owned internal protocol helpers.

## Current Scope

The kernel module currently provides:

- module load/unload orchestration in `pdm/src/pdm.c`
- PCONFIG query logic linked into `pdm.ko`
- PDM protocol package/parse helpers linked into `pdm.ko`; peripheral drivers
  pass a device type, message type, and payload to produce or parse standard
  protocol frames
- PDM-local bus logic for built-in driver registration, configured device
  binding, and device removal ordering
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
│   ├── pdm_bus.h
│   ├── pdm_driver.h
│   ├── pdm_internal.h
│   ├── pdm_proc.h
│   └── pdm_status.h
└── src/
    ├── base/
    │   ├── pdm_bus.c
    │   ├── pdm_chrdev.c
    │   ├── pdm_driver.c
    │   ├── pdm_proc.c
    │   ├── pdm_driver_start.c
    │   └── pdm_driver_end.c
    ├── pdm.c
    ├── mcu/
    │   ├── Config.in
    │   ├── pdm_mcu.c
    │   ├── pdm_mcu_chrdev.c
    │   ├── pdm_mcu_can.c
    │   ├── pdm_mcu_serial.c
    │   └── pdm_mcu_internal.h
    └── led/
        ├── Config.in
        ├── pdm_led.c
        ├── pdm_led_chrdev.c
        └── pdm_led_internal.h

kernel/include/pdm/
├── pdm.h
├── pdm_led.h
└── pdm_mcu.h
```

## Layering

`pdm.ko` owns userspace boundaries per peripheral. Built-in PDM drivers register
through `pdm_driver_register`; during module initialization PDM loads PConfig,
registers each configured device on its internal bus, and the bus binds each
device to the matching driver `probe`. On unload, bus devices are removed before
driver global resources are released.

Each PDM peripheral exposes
its own character device, such as `/dev/pdm_mcu`, and each PDI peripheral API
uses the matching UAPI ioctl header, such as `pdi_mcu.h`.

Procfs is reserved for read-only debug and observability data. Current nodes:

- `/proc/pdm/mcu`
- `/proc/pdm/led`

MCU transport APIs are linked into `pdm.ko`, but hardware access remains behind
HAL. `pdm.ko` depends on `hal.ko` and calls the HAL transport symbols exported
by that module.

The protocol layer under `src/protocol/` is a common PDM-internal peripheral
communication protocol. It does not own module lifecycle; concrete peripheral
drivers such as MCU call it when they need framed communication.
