# PDM

PDM is the Peripheral Driver Module. It contains high-level peripheral drivers built on top of PCONFIG, PRL, HAL, and OSAL.

## Current Scope

The framework currently keeps one concrete peripheral type:

- `pdm_mcu`: MCU peripheral driver with CAN and serial transports.

The layer is intentionally still organized by peripheral type so future peripherals can be added under `core/kernel/pdm/src/<peripheral>/` with matching public headers and Kconfig entries.

## Configuration

```text
CONFIG_PDM=y
CONFIG_PDM_MCU_SUPPORT=y
CONFIG_PDM_MCU_CAN_SUPPORT=y
CONFIG_PDM_MCU_UART_SUPPORT=y
```

## Layout

```text
core/kernel/pdm/
├── Config.in
├── CMakeLists.txt
├── include/pdm/
│   ├── pdm.h
│   └── pdm_mcu.h
└── src/pdm_mcu/
    ├── Config.in
    ├── pdm_mcu.c
    ├── pdm_mcu_can.c
    ├── pdm_mcu_serial.c
    └── pdm_mcu_internal.h
```

## Layering

`PDM_MCU_init()` resolves MCU indexes through `PCONFIG_GetBoard()` and `PCONFIG_HW_GetMCU()`. PDM consumes typed platform configuration and should not know concrete product table symbols.
