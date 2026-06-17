# PCONFIG

PCONFIG is the platform hardware-configuration registry. It stores platform-level hardware tables and exposes typed index-based accessors for enabled core device families.

## Current Responsibility

- Register platform configurations (`pconfig_platform_config_t`).
- Track the current board configuration.
- Provide typed accessors for MCU, FPGA, Switch, and PMC entries.
- Keep hardware configuration data separate from PDL and application logic.

## Public API

```c
int32_t PCONFIG_init(void);
void PCONFIG_cleanup(void);
int32_t PCONFIG_register(const pconfig_platform_config_t *config);
const pconfig_platform_config_t *PCONFIG_GetBoard(void);
int32_t PCONFIG_SetBoard(const pconfig_platform_config_t *config);
const pconfig_platform_config_t *PCONFIG_Find(const char *platform,
                                             const char *product,
                                             const char *version);
int32_t PCONFIG_list(const pconfig_platform_config_t **configs, uint32_t *count);
int32_t PCONFIG_validate(const pconfig_platform_config_t *config);
void PCONFIG_print(const pconfig_platform_config_t *config);
```

## Typed Accessors

The current header provides inline index-based accessors:

```c
PCONFIG_HW_GetMCU(platform, index);
PCONFIG_HW_GetFPGA(platform, index);
PCONFIG_HW_GetSwitch(platform, index);
PCONFIG_HW_GetPMC(platform, index);
```


## PMC Runtime Flow

PMC applications should use `PMC_Runtime_Init()` rather than directly calling `PCONFIG_init()` and `PCONFIG_register()`.

`pmc_runtime` performs:

1. `PCONFIG_init()`
2. `PCONFIG_register(&pconfig_${CONFIG_PROJECT_NAME})`
3. `PCONFIG_SetBoard(&pconfig_${CONFIG_PROJECT_NAME})`

This keeps board symbol knowledge in the product runtime layer.

## Platform Config Location

PMC platform PCONFIG data lives under:

```text
products/pmc/configs/projects/<CONFIG_PROJECT_NAME>/pconfig/
```

For H200-100P-AM625, the active table is:

```text
products/pmc/configs/projects/h200_100p_am625/pconfig/pconfig_h200_100p_am625.c
```

## Layering Rules

- `core/pconfig` defines data structures and registry behavior only.
- Product CMake chooses the platform table based on `CONFIG_PROJECT_NAME`.
- PDL consumes `PCONFIG_GetBoard()` and typed accessors; it should not know concrete product table symbols.
- Applications should go through product runtime initialization, not direct board registration.
