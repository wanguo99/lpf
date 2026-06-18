# PCONFIG

PCONFIG is the platform hardware-configuration query layer. It reads platform-level hardware tables that are compiled in by the product and exposes typed index-based accessors for enabled core device families.

## Current Responsibility

- Read product-provided platform configurations (`pconfig_platform_config_t`).
- Track the current board through a compile-time `current_index` in `g_pconfig_platform_table`.
- Provide typed accessors for MCU entries.
- Keep hardware configuration data separate from PDL and application logic.

## Public API

```c
const pconfig_platform_config_t *PCONFIG_GetBoard(void);
const pconfig_platform_config_t *PCONFIG_Find(const char *platform,
                                             const char *product,
                                             const char *version);
int32_t PCONFIG_list(const pconfig_platform_config_t **configs, uint32_t *count);
int32_t PCONFIG_validate(const pconfig_platform_config_t *config);
void PCONFIG_print(const pconfig_platform_config_t *config);
```

Products provide the table symbol:

```c
extern const pconfig_platform_table_t g_pconfig_platform_table;
```

## Typed Accessors

The current header provides an inline index-based MCU accessor:

```c
PCONFIG_HW_GetMCU(platform, index);
```

## Layering Rules

- `core/pconfig` defines data structures and read-only query behavior only.
- Product or board integration code owns concrete platform tables.
- PDL consumes `PCONFIG_GetBoard()` and typed accessors; it should not know concrete product table symbols.
- Runtime code must not register, switch, reload, or mutate PCONFIG tables through core APIs.
