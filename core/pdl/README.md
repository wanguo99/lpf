# PDL (Peripheral Driver Layer)

## Overview

PDL provides protocol-aware peripheral drivers above HAL. It keeps application code away from transport details while still using PCONFIG as the source of board-specific hardware configuration.

Current implemented drivers:

- `pdl_mcu`: MCU peripheral driver with CAN and serial transports.
- `pdl_pmc`: PMC communication driver with CAN and Ethernet transports.

## Responsibilities

- Query runtime hardware configuration from PCONFIG.
- Convert PCONFIG entries into HAL transport configuration.
- Encode and decode protocol frames through PRL where required.
- Provide device-level state management, retries, and error handling.
- Expose stable APIs for product applications and higher-level configuration flows.

PDL must remain platform independent. Product-specific board symbols belong in product runtime/configuration code, not in PDL.

## Build Configuration

PDL is enabled through Kconfig:

```kconfig
CONFIG_PDL=y
CONFIG_PDL_MCU_SUPPORT=y
CONFIG_PDL_PMC_SUPPORT=y
```

Typical builds:

```bash
make ctest_x86_pdl_defconfig
make

make pmc_h200_100p_am625_debug_defconfig
make
```

The component builds as part of the core module set and exports `es_middleware::pdl_public_api` for headers plus `es_middleware::pdl` for the implementation library.

## Module Layout

```text
core/pdl/
├── include/pdl/
│   ├── pdl.h
│   ├── pdl_mcu.h
│   └── pdl_pmc.h
└── src/
    ├── pdl_mcu/
    │   ├── pdl_mcu.c
    │   ├── pdl_mcu_can.c
    │   └── pdl_mcu_serial.c
    └── pdl_pmc/
        ├── pdl_pmc.c
        ├── pdl_pmc_can.c
        └── pdl_pmc_eth.c
```

## Dependencies

- OSAL: common types, logging, synchronization, and runtime utilities.
- HAL: hardware transport access such as CAN, serial, and network adapters.
- PCONFIG: runtime board hardware tables.
- PRL: protocol framing for MCU/PMC communication paths.

Applications depend on PDL; PDL must not depend on product application modules.

## Usage

```cmake
target_link_libraries(your_app PUBLIC es_middleware::pdl_public_api)
target_link_libraries(your_app PRIVATE es_middleware::pdl)
```

```c
#include "pdl.h"

pdl_mcu_handle_t mcu = NULL;
if (PDL_MCU_init(0, &mcu) == OSAL_SUCCESS) {
    uint8_t response[32] = {0};
    pdl_mcu_cmd_t cmd = {
        .cmd = 0x01,
        .response = response,
        .response_max = sizeof(response),
    };
    (void)PDL_MCU_send_cmd(mcu, &cmd);
    (void)PDL_MCU_deinit(mcu);
}
```

`PDL_MCU_init()` and `PDL_PMC_init_from_pconfig()` resolve device indexes through `PCONFIG_GetBoard()` and the typed hardware accessors.

## Development Rules

- Use HAL for all hardware access; do not open devices or sockets directly from PDL when a HAL abstraction exists.
- Use PCONFIG for board-specific values; do not hard-code device names, bus IDs, or product symbols.
- Keep protocol handling behind PDL APIs so applications call device-level operations rather than frame builders.
- Add a Kconfig option, source directory, public header, CMake source inclusion, and tests when introducing a new PDL driver.
- Keep public headers independent and include them through `pdl.h` only as a convenience aggregation.

## Testing

Use the ctest product configurations for module-level verification:

```bash
make ctest_x86_pdl_defconfig
make
```

PMC runtime paths that depend on PDL should also be checked with the PMC product defconfig:

```bash
make pmc_h200_100p_am625_debug_defconfig
make
```
