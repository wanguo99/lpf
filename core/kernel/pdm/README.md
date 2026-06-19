# PDM

PDM is the Peripheral Driver Module. It contains high-level peripheral drivers
built on top of PCONFIG, HAL, OSAL, and the PDM-owned internal protocol helpers.

## Current Scope

The kernel module currently provides:

- ioctl device-node dispatch in `pdm/src/pdm.c`
- PCONFIG query logic linked into `pdm.ko`
- PDM protocol encode/decode logic linked into `pdm.ko`
- PDM MCU core and CAN/Serial transport glue linked into `pdm.ko`

PDM consumes the exported `hal.ko` CAN and Serial symbols for MCU transport
hardware access.

## Configuration

```text
CONFIG_PDM=y
CONFIG_PCONFIG=y
CONFIG_PDM_MCU_SUPPORT=y
CONFIG_PDM_PROTOCOL=y
CONFIG_PDM_PROTOCOL_MCU=y
```

## Layout

```text
core/kernel/pdm/
├── Config.in
├── CMakeLists.txt
└── src/
    ├── pdm.c
    └── pdm_mcu/
        ├── Config.in
        ├── pdm_mcu.c
        ├── pdm_mcu_can.c
        ├── pdm_mcu_serial.c
        └── pdm_mcu_internal.h

core/kernel/include/pdm/
├── pdm.h
└── pdm_mcu.h
```

## Layering

`pdm.ko` owns the userspace boundary. PDM may consume PCONFIG and its internal
protocol helpers, but userspace must call through PDI/ioctl rather than
including kernel headers.

MCU transport APIs are linked into `pdm.ko`, but hardware access remains behind
HAL. `pdm.ko` depends on `hal.ko` and calls the HAL transport symbols exported
by that module.
