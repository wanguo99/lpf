# PMC Call-Chain Test

This optional application validates the current PMC runtime configuration path:

```
APP -> PMC_Runtime -> PMC_ACONFIG -> PDL_MCU -> PCONFIG -> HAL
```

It is enabled with `CONFIG_TEST_CALL_CHAIN=y` and builds a `test_call_chain` executable.

## Runtime Setup

The application calls `PMC_Runtime_Init()` before running tests. That runtime entry initializes and registers:

- `PCONFIG`: platform hardware table, selected by `CONFIG_PROJECT_NAME`.
- `ACONFIG`: PMC product function map, from `aconfig_${CONFIG_PROJECT_NAME}.c`.

No test code should directly reference board symbols such as `pconfig_h200_100p_am625`.

## Current Test Cases

The test cases query PMC TC mappings through `PMC_ACONFIG_GetTcConfig()` and then initialize the mapped MCU via `PDL_MCU_init()`.

| Test | TC function | Expected mapping |
| --- | --- | --- |
| `TestCallChain_MCU_CAN` | `PMC_TC_MCU_RESET` | MCU index `0` |
| `TestCallChain_MCU_Serial` | `PMC_TC_POWER_OFF` | MCU index `1` |
| `TestCallChain_PowerControl` | `PMC_TC_POWER_ON` | MCU index `0` |


## Build Verification

```bash
make pmc_h200_100p_am625_debug_defconfig
printf '\nCONFIG_TEST_CALL_CHAIN=y\n' >> .config
make clean
make
```

After verification, regenerate the normal defconfig if the test app should not remain enabled:

```bash
make pmc_h200_100p_am625_debug_defconfig
```
