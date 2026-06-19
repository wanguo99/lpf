# PDM

PDM is the Peripheral Driver Module. It contains high-level peripheral drivers
built on top of PCONFIG, HAL, OSAL, and the PDM-owned internal protocol helpers.

## Current Scope

The kernel module currently provides:

- ioctl device-node dispatch in `pdm/src/pdm_main.c`
- PCONFIG query logic linked into `pdm.ko`
- PDM protocol encode/decode logic linked into `pdm.ko`
- PDM MCU core and CAN/Serial transport glue linked into `pdm.ko`

The kernel HAL CAN/Serial implementations are currently stubs that return
`OSAL_ERR_NOT_SUPPORTED`. They provide a kernel-safe link boundary for PDM MCU
while the real in-kernel transport implementations are developed.

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
    ├── pdm_main.c
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
HAL. Until the kernel HAL CAN/Serial backends are implemented, MCU init fails
with `OSAL_ERR_NOT_SUPPORTED` from the HAL transport stub.
